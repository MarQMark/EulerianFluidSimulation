#include "solver/JacobiSolver.h"

#include <GL/glew.h>

#include "simulation/Grid.h"
#include "simulation/Properties.h"

JacobiSolver::JacobiSolver(const int width, const int height) : ISolver(width, height) {
    _tmp = new ScalField(width, height);
#ifdef USE_GPU
    _shader_jp  = new ComputeShader(ComputeShader::LoadTextFile("shaders/jacobi_pass.comp"));
    _gpu_tmp = new StorageBuffer(width * height * sizeof(float));
#endif
}

JacobiSolver::~JacobiSolver() {
    delete _tmp;
#ifdef USE_GPU
    delete _shader_jp;
    delete _gpu_tmp;
#endif
}

float JacobiSolver::solve(Grid *grid) {
    const int iter = PROPERTY_I(MAX_STEPS);

#ifdef USE_GPU

    const int gx2d = (grid->width() + 15) / 16;
    const int gy2d = (grid->height() + 15) / 16;

    auto gpu_jacobi_pass = [&](const StorageBuffer* x1, const StorageBuffer* x0) {
        _shader_jp->bind();
        x1->bind(0);
        x0->bind(1);
        grid->gpuDivergence->bind(2);
        grid->gpuRho->bind(3);
        grid->gpuBoundary->bind(4);
        _shader_jp->setInt("uWidth", grid->width());
        _shader_jp->setInt("uHeight", grid->height());
        _shader_jp->dispatch(gx2d, gy2d);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    };

    grid->gpuRho->upload(grid->rho().data());
    grid->gpuBoundary->upload(grid->boundary().data());
    grid->gpuDivergence->upload(grid->divergence().data());
    grid->gpuPressure->upload(grid->pressure().data());

    for (int i = 0; i < iter; i++) {
        gpu_jacobi_pass(_gpu_tmp, grid->gpuPressure);
        gpu_jacobi_pass(grid->gpuPressure, _gpu_tmp);
    }

    grid->gpuPressure->download(grid->pressure().data());
    return 0;

#else
    ScalField       &p        = grid->pressure();
    const ScalField &b        = grid->divergence();  // RHS: ∇·u / dt
    const ScalField &rho      = grid->rho();
    const ScalField &boundary = grid->boundary();

    ScalField& tmp = *_tmp;

    for (int i = 0; i < iter; i++) {
        for (int y = 0; y < _height; y++) {
            for (int x_ = 0; x_ < _width; x_++) {
                if (rho(x_, y) <= 0.5f || boundary(x_, y) >= 0.5f) {
                    tmp(x_, y) = p(x_, y);
                    continue;
                }

                float sum    = 0.0f;
                int   nCount = 0;

                auto sampleNeighbor = [&](int nx, int ny) {
                    if (nx < 0 || nx >= _width || ny < 0 || ny >= _height) return;
                    if (nx == 0 || nx == _width - 1 || ny == 0 || ny == _height - 1) return;
                    if (boundary(nx, ny) > 0.5f) return;
                    nCount++;
                    if (rho(nx, ny) > 0.5f)
                        sum += p(nx, ny);
                };

                sampleNeighbor(x_ - 1, y);
                sampleNeighbor(x_ + 1, y);
                sampleNeighbor(x_, y - 1);
                sampleNeighbor(x_, y + 1);

                // Jacobi update: p = (Σ p_neighbors − b) / nCount
                tmp(x_, y) = nCount > 0 ? (sum - b(x_, y)) / float(nCount) : 0.0f;
            }
        }
        std::swap(p, tmp);
    }

    // Final residual: ||b − Ap||∞
    float residual = 0.0f;
    impl_mat_mul(tmp, p, rho, boundary);
    for (int y = 0; y < _height; y++)
        for (int x_ = 0; x_ < _width; x_++)
            residual = std::max(residual, std::abs(b(x_, y) - tmp(x_, y)));

    return 0;
#endif
}

void JacobiSolver::impl_mat_mul(ScalField &result, const ScalField &x, const ScalField &rho, const ScalField &boundary) const {
    for (int y = 0; y < _height; y++) {
        for (int x_ = 0; x_ < _width; x_++) {
            if (rho(x_, y) <= 0.5f || boundary(x_, y) >= 0.5f) {  // was == 1
                result(x_, y) = x(x_, y);
                continue;
            }

            float sum    = 0.0f;
            int   nCount = 0;

            auto sampleNeighbor = [&](int nx, int ny) {
                if (nx < 0 || nx >= _width || ny < 0 || ny >= _height) return;
                if (nx == 0 || nx == _width - 1 || ny == 0 || ny == _height - 1) return;
                if (boundary(nx, ny) > 0.5f) return;
                nCount++;
                if (rho(nx, ny) > 0.5f)
                    sum += x(nx, ny);
            };

            sampleNeighbor(x_ - 1, y);
            sampleNeighbor(x_ + 1, y);
            sampleNeighbor(x_, y - 1);
            sampleNeighbor(x_, y + 1);

            // (Ap)_i = Σ x_neighbors − nCount * x_i  (discrete Laplacian row)
            result(x_, y) = nCount > 0 ? sum - float(nCount) * x(x_, y) : 0.0f;
        }
    }
}