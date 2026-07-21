#ifndef EULERIANFLUIDSIMULATION_CUDAUTIL_H
#define EULERIANFLUIDSIMULATION_CUDAUTIL_H
#ifdef HAS_CUDA

__device__ float cuda_clamp(float x, float min, float max);
__device__ unsigned int cuda_clamp(unsigned int x, unsigned int minVal, unsigned int maxVal);
__device__ unsigned int cuda_idx_2D(unsigned int x, unsigned int y, unsigned int w);
__device__ unsigned int cuda_idx_3D(unsigned int x, unsigned int y, unsigned int z, unsigned int width, unsigned int height);

#endif
#endif //EULERIANFLUIDSIMULATION_CUDAUTIL_H