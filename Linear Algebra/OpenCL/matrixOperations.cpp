// 1. Program header

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <omp.h>

#include "cl.h"
#include "cl_platform.h"

// The matrix-width and the number of work-items per work-group:
// Note: The matrices are actually MATWxMATW (square matrices) and the work group
//       sizes are LOCALSIZExLOCALSIZE:

#ifndef MATW
#define MATW        1024
#endif

#ifndef LOCALSIZE
#define LOCALSIZE   8
#endif

// OpenCL Objects
cl_platform_id      Platform;
cl_device_id        Device;
cl_kernel           Kernel;
cl_program          Program;
cl_context          Context;
cl_command_queue    CmdQueue;

// Do we want to print in csv file format?
#define CSV


float       hA[MATW][MATW];
float       hB[MATW][MATW];
float       hC[MATW][MATW];

const char *    CL_FILE_NAME = { "linearAlgebraOps.cl" };

// Function prototypes
void        SelectOpenclDevice();
char *      Vendor( cl_uint );
char *      Type( cl_device_type );
void        Wait( cl_command_queue );


int main( int argc, char *argv[] ) {
#ifndef _OPENMP
    fprintf(stderr, "OpenMP is not enabled!\n");
    return 1;
#endif

    // See if we can even open the OpenCL kernel program
    // There's no point in continuing if we can't.
    FILE *fp;
#ifdef WIN32
    errno_t err = fopen_s( &fp, CL_FILE_NAME, "r" );
    if (err != 0)
#else
    fp = fopen( CL_FILE_NAME, "r" );
    if (fp == NULL)
#endif
    {
        fprintf(stderr, "Cannot open OpenCL source file '%s'\n", CL_FILE_NAME);
        return 1;
    }

    cl_int status;      // Returned status from OpenCL calls. Test against CL_SUCCESS
                        // for error checking and handling.
    
    // Get the platform ID and the device ID
    SelectOpenclDevice();   // Sets the global variables Platform and Device


    // 2. Allocate the host memory buffers:
    // This has already been done, as we allocated the buffers as global variables
    // instead of on the heap. This allowed us to allocate the matrices as 2D arrays.

    // Initialize the input matrices. Matrix
    for (int i = 0; i < MATW; i++) {
        for (int j = 0; j < MATW; j++) {
            hA[i][j] = 2.0;
            hB[i][j] = 3.0;
        }
    }


    // 3. Create an OpenCL context:
    
    Context = clCreateContext( NULL, 1, &Device, NULL, NULL, &status);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clCreateContext failed\n");

    
    // 4. Create an OpenCL command queue:
    CmdQueue = clCreateCommandQueue(Context, Device, 0, &status);
}