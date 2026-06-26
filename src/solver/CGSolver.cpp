#include "solver/CGSolver.h"

#include <GL/glew.h>

#include "simulation/Properties.h"
#include "util/Profiler.h"


static constexpr int SCALAR_GAMMA     = 0;
static constexpr int SCALAR_DELTA     = 1;
static constexpr int SCALAR_ALPHA     = 2;
static constexpr int SCALAR_BETA      = 3;
static constexpr int SCALAR_GAMMA_OLD = 4;
static constexpr int SCALAR_COUNT     = 5;

CGSolver::CGSolver(const int width, const int height) : ISolver(width, height) {
    _t   = new ScalField(width, height);
    _tmp = new ScalField(width, height);
    _r   = new ScalField(width, height);
    _d   = new ScalField(width, height);

#ifdef USE_GPU
    _shader_as  = new ComputeShader(ComputeShader::LoadTextFile("shaders/cg_add_scaled.comp"));
    _shader_mm  = new ComputeShader(ComputeShader::LoadTextFile("shaders/cg_mat_mul.comp"));
    _shader_ss  = new ComputeShader(ComputeShader::LoadTextFile("shaders/cg_set_subtract.comp"));
    _shader_sa  = new ComputeShader(ComputeShader::LoadTextFile("shaders/cg_scale_add.comp"));

    _shader_asg = new ComputeShader(ComputeShader::LoadTextFile("shaders/cg_add_scaled_gpu.comp"));
    _shader_sag = new ComputeShader(ComputeShader::LoadTextFile("shaders/cg_scale_add_gpu.comp"));

    _shader_dp2 = new ComputeShader(ComputeShader::LoadTextFile("shaders/cg_dot2_pass.comp"));
    _shader_r2  = new ComputeShader(ComputeShader::LoadTextFile("shaders/cg_reduce2.comp"));

    _shader_us  = new ComputeShader(ComputeShader::LoadTextFile("shaders/cg_update_scalars.comp"));

    const int n = width * height;
    _gpu_r   = new StorageBuffer(n * sizeof(float));
    _gpu_d   = new StorageBuffer(n * sizeof(float));
    _gpu_t   = new StorageBuffer(n * sizeof(float));   // semantic: z  (direction image A*d)
    _gpu_tmp = new StorageBuffer(n * sizeof(float));   // semantic: w  (w = A*r each iter)

    // GPU-resident scalar state: [gamma, delta, alpha, beta, gamma_old]
    _gpu_scalars = new StorageBuffer(SCALAR_COUNT * sizeof(float));
    _scalars_cpu.resize(SCALAR_COUNT, 0.0f);

    // Build the reduction chain.
    // _reduceChain[i] is the output pair-count of the i-th reduce2 pass.
    // The final entry is always 1 (a single pair written to _gpu_scalars).
    _numGroups = (n + 255) / 256;
    _gpu_dot2  = new StorageBuffer(2 * _numGroups * sizeof(float));

    int count = _numGroups;
    while (count > 1) {
        count = (count + 255) / 256;
        _reduceChain.push_back(count);
    }

    // Allocate one intermediate buffer if the chain needs more than one reduce2 pass.
    // For grids up to 2048×2048 (numGroups ≤ 16384), a single intermediate pass
    // produces ≤ 64 pairs → one more pass reaches 1.  Two intermediate buffers would
    // only be needed at 4096×4096 (numGroups = 65536), which is exceptional.
    if (_reduceChain.size() > 1) {
        // Only the first intermediate output needs a buffer; subsequent passes may
        // also need one if the chain is longer, but that would take a grid of
        // 4096×4096 or larger.  Allocate for each stage except the last (→ scalars).
        for (int i = 0; i + 1 < (int)_reduceChain.size(); i++) {
            _gpu_reduce_chain.push_back(new StorageBuffer(2 * _reduceChain[i] * sizeof(float)));
        }
    }
#endif
}

CGSolver::~CGSolver() {
    delete _t;
    delete _tmp;
    delete _r;
    delete _d;

#ifdef USE_GPU
    delete _shader_as;
    delete _shader_mm;
    delete _shader_ss;
    delete _shader_sa;

    delete _shader_asg;
    delete _shader_sag;
    delete _shader_dp2;
    delete _shader_r2;
    delete _shader_us;

    delete _gpu_t;
    delete _gpu_tmp;
    delete _gpu_r;
    delete _gpu_d;

    delete _gpu_scalars;
    delete _gpu_dot2;
    for (const auto* buf : _gpu_reduce_chain)
        delete buf;
#endif
}

float CGSolver::solve(Grid* grid) {
    const auto& divergence = grid->divergence();
    const auto& rho        = grid->rho();
    const auto& boundary   = grid->boundary();
    auto&       pressure   = grid->pressure();

#ifdef USE_GPU
    Profiler* profiler = Profiler::Get();

    profiler->start("6.1 Solver Start");
    const int   steps          = PROPERTY_I(MAX_STEPS);
    const int   convCheck      = PROPERTY_I(SOLVER_CHECK_INTERVAL);
    const float errorThreshold = PROPERTY_F(SOLVER_THRESHOLD);

    const int w    = grid->width();
    const int h    = grid->height();
    const int n    = grid->size();
    const int gx1d = (n + 255) / 256;
    const int gx2d = (w + 15) / 16;
    const int gy2d = (h + 15) / 16;

    auto barrier = []() {
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    };

    // result = A * x  (Laplacian mat-vec)
    auto gpu_mat_mul = [&](const StorageBuffer* result, const StorageBuffer* x) {
        _shader_mm->bind();
        result->bind(0);  x->bind(1);
        grid->gpuRho->bind(2);  grid->gpuBoundary->bind(3);
        _shader_mm->setInt("uWidth", w);
        _shader_mm->setInt("uHeight", h);
        _shader_mm->dispatch(gx2d, gy2d);
        barrier();
    };

    // result[i] = x[i] - d[i]
    auto gpu_set_subtract = [&](const StorageBuffer* result,
                                const StorageBuffer* x,
                                const StorageBuffer* d) {
        _shader_ss->bind();
        result->bind(0);  x->bind(1);  d->bind(2);
        _shader_ss->setInt("uCount", n);
        _shader_ss->dispatch(gx1d);
        barrier();
    };

    // result[i] = factor * result[i] + x[i]   (CPU-scalar, used for init only)
    auto gpu_scale_add_cpu = [&](const StorageBuffer* result,
                                 const StorageBuffer* x, float factor) {
        _shader_sa->bind();
        result->bind(0);  x->bind(1);
        _shader_sa->setInt("uCount", n);
        _shader_sa->setFloat("uFactor", factor);
        _shader_sa->dispatch(gx1d);
        barrier();
    };

    // ── Full reduction: dot(r,r) and dot(r,w) → _gpu_scalars[0..1] ────────
    //
    // Runs the dot2 pass followed by as many reduce2 passes as the chain
    // requires.  No CPU download; the final pair lands in _gpu_scalars.
    // Dispatches are fire-and-forget separated only by SSBO barriers.
    //
    auto gpu_full_reduce = [&]() {
        _shader_dp2->bind();
        _gpu_dot2->bind(0);
        _gpu_r->bind(1);
        _gpu_tmp->bind(2);   // w = A*r
        _shader_dp2->setInt("uCount", n);
        _shader_dp2->dispatch(_numGroups);
        barrier();

        StorageBuffer* src      = _gpu_dot2;
        int            inCount  = _numGroups;

        for (int stage = 0; stage < (int)_reduceChain.size(); stage++) {
            const int outCount = _reduceChain[stage];

            StorageBuffer* dst = (stage < (int)_gpu_reduce_chain.size())
                               ? _gpu_reduce_chain[stage]   // intermediate
                               : _gpu_scalars;              // final → scalars[0..1]

            _shader_r2->bind();
            dst->bind(0);
            src->bind(1);
            _shader_r2->setInt("uCount", inCount);
            _shader_r2->dispatch(outCount);
            barrier();

            src     = dst;
            inCount = outCount;
        }
    };

    // result[i] += sign * scalars[idx] * x[i]
    auto gpu_add_scaled_gpu = [&](const StorageBuffer* result,
                                  const StorageBuffer* x,
                                  float sign, int idx) {
        _shader_asg->bind();
        result->bind(0);  x->bind(1);  _gpu_scalars->bind(2);
        _shader_asg->setInt("uCount", n);
        _shader_asg->setInt("uScalarIdx", idx);
        _shader_asg->setFloat("uSign", sign);
        _shader_asg->dispatch(gx1d);
        barrier();
    };

    // result[i] = scalars[idx] * result[i] + x[i]
    auto gpu_scale_add_gpu = [&](const StorageBuffer* result,
                                 const StorageBuffer* x, int idx) {
        _shader_sag->bind();
        result->bind(0);  x->bind(1);  _gpu_scalars->bind(2);
        _shader_sag->setInt("uCount", n);
        _shader_sag->setInt("uScalarIdx", idx);
        _shader_sag->dispatch(gx1d);
        barrier();
    };

    // Single-invocation scalar update on GPU: reads gamma/delta from scalars[0..1],
    // writes alpha/beta to scalars[2..3], saves gamma_old to scalars[4].
    auto gpu_update_scalars = [&]() {
        _shader_us->bind();
        _gpu_scalars->bind(0);
        _shader_us->dispatch(1);
        barrier();
    };

    grid->gpuRho->upload(rho.data());
    grid->gpuBoundary->upload(boundary.data());
    grid->gpuDivergence->upload(divergence.data());
    grid->gpuPressure->upload(pressure.data());

    // r = b − A*x₀  Reuse _gpu_tmp (the w slot) as a temporary to hold A*x₀, then overwrite.
    gpu_mat_mul(_gpu_tmp, grid->gpuPressure);                         // tmp  = A*x₀
    gpu_set_subtract(_gpu_r, grid->gpuDivergence, _gpu_tmp);         // r    = b − A*x₀

    // w = A*r  (first w, needed for both the initial dot and d/z init)
    gpu_mat_mul(_gpu_tmp, _gpu_r);

    // Full reduction → _gpu_scalars[0] = γ₀,  _gpu_scalars[1] = δ₀
    gpu_full_reduce();

    // This is the only mandatory CPU stall per frame.  We need γ₀ to check the
    // initial residual and compute α₀ = γ₀/δ₀ to seed the GPU scalar state.
    _gpu_scalars->download(_scalars_cpu.data(), 2 * sizeof(float));
    const float gamma0 = _scalars_cpu[SCALAR_GAMMA];
    const float delta0 = _scalars_cpu[SCALAR_DELTA];

    if (gamma0 < errorThreshold) {
        grid->gpuPressure->download(pressure.data());
        return gamma0;
    }

    // Seed the full GPU scalar state.
    _scalars_cpu[SCALAR_GAMMA]     = gamma0;
    _scalars_cpu[SCALAR_DELTA]     = delta0;
    _scalars_cpu[SCALAR_ALPHA]     = gamma0 / delta0;   // α₀ = γ₀/δ₀
    _scalars_cpu[SCALAR_BETA]      = 0.0f;              // β₀ = 0 (first iter uses β=0)
    _scalars_cpu[SCALAR_GAMMA_OLD] = gamma0;            // for next β = γ_new/γ_old
    _gpu_scalars->upload(_scalars_cpu.data());

    // d = r,  z = w  (scale_add with 0: result = 0*result + src = src)
    gpu_scale_add_cpu(_gpu_d, _gpu_r,   0.0f);   // d = r
    gpu_scale_add_cpu(_gpu_t, _gpu_tmp, 0.0f);   // z = w  (_gpu_t is the z slot)

    float gamma_last = gamma0;

    profiler->stop();
    profiler->start("6.2 Solver Loop");
    int i = 0;
    for (i = 0; i < steps; i++)
    {
        // x += alpha * d           (alpha = scalars[SCALAR_ALPHA])
        profiler->start("6.2.1 | x += alpha * d");
        gpu_add_scaled_gpu(grid->gpuPressure, _gpu_d,  +1.0f, SCALAR_ALPHA);
        profiler->stop();

        // r -= alpha * z
        profiler->start("6.2.2 | r -= alpha * z");
        gpu_add_scaled_gpu(_gpu_r, _gpu_t, -1.0f, SCALAR_ALPHA);
        profiler->stop();

        // ── Residual replacement every 64 iterations ──────────────────────
        // Resets the accumulated floating-point error in r by recomputing
        // r = b − A*x directly.  Reuses _gpu_tmp (w slot) as a temp.
        profiler->start("6.2.3 | r = b − A*x");
        if ((i & 0x3F) == 0x3F) {
            gpu_mat_mul(_gpu_tmp, grid->gpuPressure);                 // tmp  = A*x
            gpu_set_subtract(_gpu_r, grid->gpuDivergence, _gpu_tmp); // r    = b − A*x
        }
        profiler->stop();

        // w = A*r  (always needed for the reduction)
        profiler->start("6.2.4 | w = A*r");
        gpu_mat_mul(_gpu_tmp, _gpu_r);
        profiler->stop();

        // Full reduction: dot(r,r), dot(r,w) → scalars[0..1]
        profiler->start("6.2.5 | dot(r,r), dot(r,w)");
        gpu_full_reduce();
        profiler->stop();

        // alpha, beta update on GPU: scalars[2..4] ← f(scalars[0..4])
        profiler->start("6.2.6 | update scalars");
        gpu_update_scalars();
        profiler->stop();

        // d = beta*d + r   (beta = scalars[SCALAR_BETA])
        profiler->start("6.2.7 | d = beta*d + r");
        gpu_scale_add_gpu(_gpu_d, _gpu_r,   SCALAR_BETA);
        profiler->stop();

        // z = beta*z + w
        profiler->start("6.2.8 | z = beta*z + w");
        gpu_scale_add_gpu(_gpu_t, _gpu_tmp, SCALAR_BETA);
        profiler->stop();

        profiler->start("6.2.9 | gamma download");
        if ((i % convCheck) == (convCheck - 1))
        {
            _gpu_scalars->download(&gamma_last, sizeof(float));
            if (gamma_last < errorThreshold) {
                profiler->stop();
                break;
            }
        }
        profiler->stop();
    }
    profiler->stop();
    profiler->start("6.3 Solver End");

    SET_PROPERTY_I(SOLVE_STEPS, i + 1);
    grid->gpuPressure->download(pressure.data());
    profiler->stop();
    return gamma_last;

#else
    const int   steps          = PROPERTY_I(MAX_STEPS);
    const float errorThreshold = PROPERTY_F(SOLVER_THRESHOLD);

    auto& t   = *_t;   t.clear();
    auto& tmp = *_tmp; tmp.clear();
    auto& r   = *_r;   r = divergence;

    pressure = divergence;
    impl_mat_mul(t, pressure, rho, boundary);
    r -= t;
    auto& d = *_d; d = r;

    float sqrtLen = r.sqLen();
    if (sqrtLen < errorThreshold)
        return sqrtLen;

    for (int i = 0; i < steps; i++) {
        impl_mat_mul(t, d, rho, boundary);
        const float alpha = sqrtLen / d.dot(t);
        pressure.addScaled(d, alpha);

        if (i & 0x3F) {
            r.addScaled(t, -alpha);
        }
        else {
            impl_mat_mul(t, pressure, rho, boundary);
            r = divergence;
            r -= t;
        }

        const float oldSqrtLen = sqrtLen;
        sqrtLen = r.sqLen();
        if (sqrtLen < errorThreshold)
            return sqrtLen;

        const float beta = sqrtLen / oldSqrtLen;
        d *= beta;
        d += r;
    }

    return sqrtLen;
#endif
}

void CGSolver::impl_mat_mul(ScalField &result, const ScalField &x,
                             const ScalField &rho, const ScalField &boundary) const
{
    for (int y = 0; y < _height; y++) {
        for (int x_ = 0; x_ < _width; x_++) {
            if (rho(x_, y) <= 0.5f || boundary(x_, y) == 1) {
                result(x_, y) = x(x_, y);
                continue;
            }
            float sum    = 0.0f;
            int   nCount = 0;

            auto sampleNeighbor = [&](const int nx, const int ny) {
                if (nx < 0 || nx >= _width || ny < 0 || ny >= _height) return;
                if (nx == 0 || nx == _width-1 || ny == 0 || ny == _height-1) return;
                if (boundary(nx, ny) > 0.5f) return;
                nCount++;
                if (rho(nx, ny) > 0.5f)
                    sum += x(nx, ny);
            };

            sampleNeighbor(x_-1, y); sampleNeighbor(x_+1, y);
            sampleNeighbor(x_, y-1); sampleNeighbor(x_, y+1);
            result(x_, y) = sum - (float)nCount * x(x_, y);
        }
    }
}