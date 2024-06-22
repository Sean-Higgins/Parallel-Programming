// System Includes
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <assert.h>
#include <malloc.h>

// CUDA runtime
#include <cuda_runtime.h>

// Helper functions and utilities to work with CUDA
#include "helper_functions.h"
#include "helper_cuda.h"

// CudaCheckError: Prints a detailed message abount the most recent
//                 error to have occurred.
void CudaCheckError(int which) {
    // "which" is which error number has been assigned:
    // It is not a line number, just an arbitrary integer
    cudaError_t e = cudaGetLastError();

    if (e != cudaSuccess)
        fprintf(stderr, "CUDA Failure: %s:%d: '%s'\n", __FILE__, which, cudaGetErrorString(e));
}


#define IN
#define OUT

__global__ void MatrixMult(IN float *dMatrixA, IN float *dMatrixB, IN int *dMW, OUT float *dMatrixC) {
    //unsigned int numItems = blockDim.x;
    //unsigned int wgNum    = blockIdx.x;   // Work-Group Number
    //unsigned int tnum     = threadIdx.x;
    unsigned int gid = blockIdx.x*blockDim.x + threadIdx.x;

    dMatrixC[gid] = 0.0;

    // 
}

int main(int argc, char *argv[]) {
    
}