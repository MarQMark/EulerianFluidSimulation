#include "simulation/solver/JacobiSolver.h"

#include "simulation/data/Grid2D.h"
#include "simulation/Properties.h"
#include "simulation/data/OpenGLGrid2D.h"
#include "simulation/data/OpenGLGrid3D.h"
#include "simulation/data/ScalField3D.h"

JacobiSolverCPU2D::JacobiSolverCPU2D(const unsigned int width, const unsigned int height)
    : ISolverCPU2D(width, height) {
    _tmp = new ScalField2D(width, height);
}

JacobiSolverCPU2D::~JacobiSolverCPU2D() {
    delete _tmp;
}

float JacobiSolverCPU2D::solve(IGrid2D *_grid) {
    auto* grid = dynamic_cast<Grid2D*>(_grid);
    if (!grid) return 0;

    const int iter = PROPERTY_I(MAX_STEPS) / 2;

    ScalField2D       &p        = grid->pressure();
    const ScalField2D &b        = grid->divergence();  // RHS: ∇·u / dt
    const ScalField2D &rho      = grid->rho();
    const ScalField2D &boundary = grid->boundary();

    ScalField2D& tmp = *_tmp;

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
    //float residual = 0.0f;
    //impl_mat_mul(tmp, p, rho, boundary);
    //for (int y = 0; y < _height; y++)
    //    for (int x_ = 0; x_ < _width; x_++)
    //        residual = std::max(residual, std::abs(b(x_, y) - tmp(x_, y)));

    return 0;
}

void JacobiSolverCPU2D::impl_mat_mul(ScalField2D &result, const ScalField2D &x, const ScalField2D &rho, const ScalField2D &boundary) const {
    for (int y = 0; y < _height; y++) {
        for (int x_ = 0; x_ < _width; x_++) {
            if (rho(x_, y) <= 0.5f || boundary(x_, y) >= 0.5f) {  // was == 1
                result(x_, y) = x(x_, y);
                continue;
            }

            float sum    = 0.0f;
            int   nCount = 0;

            auto sampleNeighbor = [&](const int nx, const int ny) {
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

JacobiSolverOpenGL2D::JacobiSolverOpenGL2D(const unsigned int width, const unsigned int height)
    : ISolverOpenGL2D(width, height){
    _shader_jp  = new Shader({{"assets/shaders/jacobi_pass_2D.comp", Shader::COMPUTE }});
    _gpu_tmp = new StorageBuffer(_size * sizeof(float));
}

JacobiSolverOpenGL2D::~JacobiSolverOpenGL2D() {
    delete _shader_jp;
    delete _gpu_tmp;
}

float JacobiSolverOpenGL2D::solve(IGrid2D *_grid) {
    const auto* grid = dynamic_cast<OpenGLGrid2D*>(_grid);
    if (!grid) return 0;

    const int iter = PROPERTY_I(MAX_STEPS);

    const int gx2d = (grid->width() + 15) / 16;
    const int gy2d = (grid->height() + 15) / 16;

    auto gpu_jacobi_pass = [&](const StorageBuffer& x1, const StorageBuffer& x0) {
        _shader_jp->bind();
        x1.bind(0);
        x0.bind(1);
        grid->gpuDivergence().bind(2);
        grid->gpuRho().bind(3);
        grid->gpuBoundary().bind(4);
        _shader_jp->setInt("uWidth", grid->width());
        _shader_jp->setInt("uHeight", grid->height());
        _shader_jp->dispatch(gx2d, gy2d);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    };

    for (int i = 0; i < iter; i++) {
        gpu_jacobi_pass(*_gpu_tmp, grid->gpuPressure());
        gpu_jacobi_pass(grid->gpuPressure(), *_gpu_tmp);
    }

    return 0;
}

JacobiSolverCPU3D::JacobiSolverCPU3D(const unsigned int width, const unsigned int height, const unsigned int depth)
    : ISolverCPU3D(width, height, depth) {
    _tmp = new ScalField3D(width, height, depth);
}

JacobiSolverCPU3D::~JacobiSolverCPU3D() {
    delete _tmp;
}

float JacobiSolverCPU3D::solve(IGrid3D *grid) {
    if (!grid) return 0;

    const int iter = PROPERTY_I(MAX_STEPS) / 2;

    ScalField3D       &p        = grid->pressure();
    const ScalField3D &b        = grid->divergence();  // RHS: ∇·u / dt
    const ScalField3D &rho      = grid->rho();
    const ScalField3D &boundary = grid->boundary();

    ScalField3D& tmp = *_tmp;

    for (int i = 0; i < iter; i++) {
        for (int z = 0; z < _depth; z++)
        for (int y = 0; y < _height; y++)
        for (int x_ = 0; x_ < _width; x_++) {
            if (rho(x_, y, z) <= 0.5f || boundary(x_, y, z) >= 0.5f) {
                tmp(x_, y, z) = p(x_, y, z);
                continue;
            }

            float sum    = 0.0f;
            int   nCount = 0;

            auto sampleNeighbor = [&](const int nx, const int ny , const int nz) {
                if (nx < 0 || nx >= _width || ny < 0 || ny >= _height || nz < 0 || nz >= _depth)
                    return;
                if (nx == 0 || nx == _width - 1 || ny == 0 || ny == _height - 1 || nz == 0 || nz == _depth - 1)
                    return;
                if (boundary(nx, ny, nz) > 0.5f) return;
                nCount++;
                if (rho(nx, ny, nz) > 0.5f)
                    sum += p(nx, ny, nz);
            };

            sampleNeighbor(x_ - 1, y, z);
            sampleNeighbor(x_ + 1, y, z);
            sampleNeighbor(x_, y - 1, z);
            sampleNeighbor(x_, y + 1, z);
            sampleNeighbor(x_, y, z - 1);
            sampleNeighbor(x_, y, z + 1);

            // Jacobi update: p = (Σ p_neighbors − b) / nCount
            tmp(x_, y, z) = nCount > 0 ? (sum - b(x_, y, z)) / (float)nCount : 0.0f;
        }
        std::swap(p, tmp);
    }

    // Final residual: ||b − Ap||∞
    /*float residual = 0.0f;
    impl_mat_mul(tmp, p, rho, boundary);
    for (int z = 0; z < _depth; z++)
    for (int y = 0; y < _height; y++)
    for (int x_ = 0; x_ < _width; x_++)
        residual = std::max(residual, std::abs(b(x_, y, z) - tmp(x_, y, z)));
    return residual;*/

    return 0;
}

void JacobiSolverCPU3D::impl_mat_mul(ScalField3D &result, const ScalField3D &x, const ScalField3D &rho, const ScalField3D &boundary) const {
    for (int z = 0; z < _depth; z++)
    for (int y = 0; y < _height; y++)
    for (int x_ = 0; x_ < _width; x_++) {
        if (rho(x_, y, z) <= 0.5f || boundary(x_, y, z) >= 0.5f) {  // was == 1
            result(x_, y, z) = x(x_, y, z);
            continue;
        }

        float sum    = 0.0f;
        int   nCount = 0;

        auto sampleNeighbor = [&](const int nx, const int ny , const int nz) {
            if (nx < 0 || nx >= _width || ny < 0 || ny >= _height || nz < 0 || nz >= _depth)
                return;
            if (nx == 0 || nx == _width - 1 || ny == 0 || ny == _height - 1 || nz == 0 || nz == _depth - 1)
                return;
            if (boundary(nx, ny, nz) > 0.5f) return;
            nCount++;
            if (rho(nx, ny, nz) > 0.5f)
                sum += x(nx, ny, nz);
        };

        sampleNeighbor(x_ - 1, y, z);
        sampleNeighbor(x_ + 1, y, z);
        sampleNeighbor(x_, y - 1, z);
        sampleNeighbor(x_, y + 1, z);
        sampleNeighbor(x_, y, z - 1);
        sampleNeighbor(x_, y, z + 1);

        // (Ap)_i = Σ x_neighbors − nCount * x_i  (discrete Laplacian row)
        result(x_, y, z) = nCount > 0 ? sum - (float)nCount * x(x_, y, z) : 0.0f;
    }
}

JacobiSolverOpenGL3D::JacobiSolverOpenGL3D(const unsigned int width, const unsigned int height, const unsigned int depth)
    : ISolverOpenGL3D(width, height, depth) {
    _shader_jp  = new Shader({{"assets/shaders/jacobi_pass_3D.comp", Shader::COMPUTE }});
    _gpu_tmp = new StorageBuffer(_size * sizeof(float));
}

JacobiSolverOpenGL3D::~JacobiSolverOpenGL3D() {
    delete _shader_jp;
    delete _gpu_tmp;
}

float JacobiSolverOpenGL3D::solve(IGrid3D *_grid) {
    const auto* grid = dynamic_cast<OpenGLGrid3D*>(_grid);
    if (!grid) return 0;

    const int iter = PROPERTY_I(MAX_STEPS);

    const unsigned int gx2d = (grid->width()  + 7) / 8;
    const unsigned int gy2d = (grid->height() + 7) / 8;
    const unsigned int gz2d = (grid->depth()  + 7) / 8;

    auto gpu_jacobi_pass = [&](const StorageBuffer& x1, const StorageBuffer& x0) {
        _shader_jp->bind();
        x1.bind(0);
        x0.bind(1);
        grid->gpuDivergence().bind(2);
        grid->gpuRho().bind(3);
        grid->gpuBoundary().bind(4);
        _shader_jp->setInt("uWidth",  (int)grid->width());
        _shader_jp->setInt("uHeight", (int)grid->height());
        _shader_jp->setInt("uDepth",  (int)grid->depth());
        _shader_jp->dispatch(gx2d, gy2d, gz2d);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    };

    for (int i = 0; i < iter; i++) {
        gpu_jacobi_pass(*_gpu_tmp, grid->gpuPressure());
        gpu_jacobi_pass(grid->gpuPressure(), *_gpu_tmp);
    }

    return 0;
}
