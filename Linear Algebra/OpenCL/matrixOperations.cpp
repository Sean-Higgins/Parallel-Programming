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
    if (status != CL_SUCCESS)
        fprintf(stderr, "clCreateCommandQueue failed\n");
    

    // 5. Allocate the device memory buffers for the A, B, and C matrices
    size_t aSize = MATW * MATW * sizeof(float);
    size_t bSize = MATW * MATW * sizeof(float);
    int mw = MATW;
    size_t mwSize = sizeof(mw);
    size_t cSize = MATW * MATW * sizeof(float);

    // Allocating device memory for the A matrix
    cl_mem dA = clCreateBuffer( Context, CL_MEM_READ_ONLY, aSize, NULL, &status);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clCreateBuffer failed for dA (1)\n");

    // Allocating device memory for the B matrix
    cl_mem dB = clCreateBuffer( Context, CL_MEM_READ_ONLY, bSize, NULL, &status);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clCreateBuffer failed for dB (1)\n");
    
    // Allocating device memory for the width of the matrices
    cl_mem dMW = clCreateBuffer( Context, CL_MEM_READ_ONLY, mwSize, NULL, &status);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clCreateBuffer failed for dMW (1)\n");

    // Allocating device memory for the C matrix
    cl_mem dC = clCreateBuffer( Context, CL_MEM_WRITE_ONLY, cSize, NULL, &status);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clCreateBuffer failed for dC (1)\n");
    

    // 6. Enqueue the commands to write data from the host buffers to the device
    //    buffers:

    // Enqueue the data from matrix A to the device.
    status = clEnqueueWriteBuffer(CmdQueue, dA, CL_FALSE, 0, aSize, hA, 0, NULL, NULL);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clEnqueueWriteBuffer failed for matrix A (1)\n");

    // Enqueue the data from matrix B to the device.
    status = clEnqueueWriteBuffer(CmdQueue, dB, CL_FALSE, 0, bSize, hB, 0, NULL, NULL);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clEnqueueWriteBuffer failed for matrix B (1)\n");

    // Enqueue the matrix width data dMW to the device.
    status = clEnqueueWriteBuffer(CmdQueue, dMW, CL_FALSE, 0, mwSize, &mw, 0, NULL, NULL);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clEnqueueWriteBuffer failed for matrix width mw (1)\n");
    
    Wait( CmdQueue );


    // 7. Read the kernel code from a file...
    fseek( fp, 0, SEEK_END);
    size_t fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *clProgramText = new char[ fileSize+1 ];   // Leave room for '\0'
    size_t n = fread(clProgramText, 1, fileSize, fp);
    clProgramText[fileSize] = '\0';
    fclose(fp);

    if (n != fileSize)
        fprintf(stderr, "Expected to read %d bytes from '%s' -- actually read %d.\n",
                fileSize, CL_FILE_NAME, n);
    
    // ... and create the kernel program:
    char *strings[1];
    strings[0] = clProgramText;
    Program = clCreateProgramWithSource(Context, 1, (const char **)strings, NULL, &status);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clCreateProgramWithSource failed\n");
    
    delete [ ] clProgramText;


    // 8. Compile and link the kernel code:
    char *options = { (char *)"" };
    status = clBuildProgram(Program, 1, &Device, options, NULL, NULL);
    if (status != CL_SUCCESS) {
        size_t size;
        clGetProgramBuildInfo(Program, Device, CL_PROGRAM_BUILD_LOG, 0, NULL, &size);   // Get the size of the log
        cl_char *log = new cl_char[size];   // Create a buffer with enough space to store the log
        clGetProgramBuildInfo(Program, Device, CL_PROGRAM_BUILD_LOG, size, log, NULL);  // Copy the log to read.
        fprintf(stderr, "clBuildProgram failed:\n%s\n", log);   // Print the log
        delete [ ] log;
    }


    // 9. Create the kernel object:
    Kernel = clCreateKernel( Program, "MatrixMult", &status);
        fprintf(stderr, "clCreateKernel failed\n");
    

    // 10. Set up the arguments to the kernel object:
    
    // Passing the dA matrix argument to the kernel object
    status = clSetKernelArg( Kernel, 0, sizeof(cl_mem), &dA);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clSetKernelArg failed for dA (%d)\n", status);
    
    // Passing the dB matrix argument to the kernel object
    status = clSetKernelArg( Kernel, 1, sizeof(cl_mem), &dB);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clSetKernelArg failed for dB (%d)\n", status); 

    // Passing the dMW matrix width argument to the kernel object
    status = clSetKernelArg( Kernel, 2, sizeof(cl_mem), &dMW);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clSetKernelArg failed for dMW (%d)\n", status);

    // Passing the dC matrix argument to the kernel object
    status = clSetKernelArg( Kernel, 3, sizeof(cl_mem), &dC);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clSetKernelArg failed for dC (%d)\n", status);
    

    // 11. Enqueue the kernel object for execution:

    size_t globalWorkSize[3] = { MATW,      MATW,       1 };
    size_t localWorkSize[3]  = { LOCALSIZE, LOCALSIZE,  1 };

#ifndef CSV
    fprintf(stdout, "Number of Work Groups = %5d x %5d\n", MATW/LOCALSIZE, MATW/LOCALSIZE);
#endif

    Wait( CmdQueue );

    double time0 = omp_get_wtime();

    status = clEnqueueNDRangeKernel(CmdQueue, Kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clEnqueueNDRangeKernel failed: %d\n", status);
    
    Wait( CmdQueue );
    double time1 = omp_get_wtime();


    // 12. Read the results buffer back from the device to the host:
    status = clEnqueueReadBuffer( CmdQueue, dC, CL_FALSE, 0, cSize, hC, 0, NULL, NULL);
    if (status != CL_SUCCESS)
        fprintf(stderr, "clEnqueueReadBuffer failed\n");

    Wait( CmdQueue );

#ifdef CSV
    fprintf(stdout, "%8d, %6d, %10.2lf\n",
            MATW*MATW, LOCALSIZE*LOCALSIZE,
            (double)MATW*(double)MATW*(double)MATW/(time1-time0)/1000000000.);
#else
    fprintf(stdout, "Matrix Size: %6d x %6d, Work Elements: %4d x %4d, GigaMultsPerSecond: %10.2lf\n",
            MATW, MATW, LOCALSIZE, LOCALSIZE,
            (double)MATW*(double)MATW*(double)MATW/(time1-time0)/1000000000.);
#endif


    // 13. Clean everything up:
    clReleaseKernel(Kernel);
    clReleaseProgram(Program);
    clReleaseCommandQueue(CmdQueue);
    clReleaseMemObject(dA);
    clReleaseMemObject(dB);
    clReleaseMemObject(dMW);
    clReleaseMemObject(dC);

    return 0;
}


// Wait until all queued tasks have taken place
void Wait(cl_command_queue queue) {
    cl_event wait;
    cl_int status;

    status = clEnqueueMarker(queue, &wait);
    if (status != CL_SUCCESS)
        fprintf(stderr, "Wait: clEnqueueMarker failed\n");
    
    status = clWaitForEvents(1, &wait);
    if (status != CL_SUCCESS)
        fprintf(stderr, "Wait: clWaitForEvents failed\n");
}


// Vendor IDs:
#define ID_AMD      0x1002
#define ID_INTEL    0x8086
#define ID_NVIDIA   0x10de

void SelectOpenclDevice() {
    // Select which OpenCL device to use:
    // Priority order:
    // 1. A GPU
    // 2. An NVidia or AMD GPU
    // 3. An Intel GPU
    // 4. An Intel CPU

    int bestPlatform = -1;
    int bestDevice = -1;
    cl_device_type bestDeviceType;
    cl_uint bestDeviceVendor;
    cl_int status;  // Returned status from OpenCL calls. Test against CL_SUCCESS

    // Find out how many platforms are attached here and get their IDs.
    cl_uint numPlatforms;
    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    if (status != CL_SUCCESS)
        fprintf(stderr, "selectOpenclDevice: clGetPlatformIDs failed (1)\n");
    
    cl_platform_id* platforms = new cl_platform_id[numPlatforms];
    status = clGetPlatformIDs(numPlatforms, platforms, NULL);
    if (status != CL_SUCCESS)
        fprintf(stderr, "selectOpenclDevice: clGetPlatformIDs failed (2)\n");

    for (int p = 0; p < (int)numPlatforms; p++) {
        // Find out how many devices are attached to each platform and get their IDs:
        cl_uint numDevices;

        status = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
        if (status != CL_SUCCESS)
            fprintf(stderr, "selectOpenclDevice: clGetDeviceIDs failed (1)\n");
        
        cl_device_id* devices = new cl_device_id[numDevices];
        status = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);
        if (status != CL_SUCCESS)
            fprintf(stderr, "selectOpenclDevice: clGetDeviceIDs failed (2)\n");
        
        for (int d = 0; d < (int)numDevices; d++) {
            cl_device_type type;
            cl_uint vendor;
            size_t sizes[3] = {0, 0, 0};

            clGetDeviceInfo(devices[d], CL_DEVICE_TYPE, sizeof(type), &type, NULL);

            clGetDeviceInfo(devices[d], CL_DEVICE_VENDOR_ID, sizeof(vendor), &vendor, NULL);

            // Select:
            if (bestPlatform < 0)   // Not yet holding anything -- we'll accept anything
            {
                bestPlatform = p;
                bestDevice = d;
                Platform = platforms[bestPlatform];
                Device = devices[bestDevice];
                bestDeviceType = type;
                bestDeviceVendor = vendor;
            } else {    // A bestPlatform has been found, but can we do better?
                if (bestDeviceType == CL_DEVICE_TYPE_CPU) { // Holding a CPU already. Switch to GPU if possible
                    if (type == CL_DEVICE_TYPE_GPU) {   // Found a GPU. Switch to the GPU that was found
                        bestPlatform = p;
                        bestDevice = d;
                        Platform = platforms[bestPlatform];
                        Device = devices[bestDevice];
                        bestDeviceType = type;
                        bestDeviceVendor = vendor;
                    }
                } else {    // Holding a GPU. Is a better GPU available?
                    if (bestDeviceVendor == ID_INTEL ) {    // Currently holding an Intel GPU
                        // We are assuming we just found a bigger, better NVidia or AMD GPU
                        bestPlatform = p;
                        bestDevice = d;
                        Platform = platforms[bestPlatform];
                        Device = devices[bestDevice];
                        bestDeviceType = type;
                        bestDeviceVendor = vendor;
                    }

                }
            }
        }

        delete [ ] devices;
    }
    
    delete [ ] platforms;

    if (bestPlatform < 0) {
        fprintf(stderr, "No OpenCL devices were located!\n");
        exit(1);
    } else {
#ifndef CSV
        fprintf(stdout, "I have selected Platform #%d, Device #%d: ", bestPlatform, bestDevice);
        fprintf(stdout, "Vendor = %s, Type = %s\n", Vendor(bestDeviceVendor), Type(bestDeviceType));
#endif
    }
}


char * Vendor( cl_uint v ) {
    switch (v)
    {
    case ID_AMD:
        return (char*)"AMD";
    case ID_INTEL:
        return (char*)"Intel";
    case ID_NVIDIA:
        return (char*)"NVIDIA";
    }
    return (char*)"Unknown";
}


char * Type( cl_device_type t) {
    switch (t)
    {
    case CL_DEVICE_TYPE_CPU:
        return (char*)"CL_DEVICE_TYPE_CPU";
    case CL_DEVICE_TYPE_GPU:
        return (char*)"CL_DEVICE_TYPE_GPU";
    case CL_DEVICE_TYPE_ACCELERATOR:
        return (char*)"CL_DEVICE_TYPE_ACCELERATOR";
    }

    return (char*)"Unknown";
}