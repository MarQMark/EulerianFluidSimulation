#include "util/CudaUtil.h"

__device__ float cuda_clamp(const float x, const float min, const float max) {
    return fmaxf(min, fminf(max, x));
}
__device__ unsigned int cuda_clamp(const unsigned int x, const unsigned int minVal, const unsigned int maxVal) {
    return max(minVal, min(maxVal, x));
}

__device__ unsigned int cuda_idx_2D(const unsigned int x, const unsigned int y, const unsigned int w) {
    return y * w + x;
}
__device__ unsigned int cuda_idx_3D(
    const unsigned int x, const unsigned int y, const unsigned int z,
    const unsigned int width, const unsigned int height)
{
    return z * width * height + y * width + x;
}

