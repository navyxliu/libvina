/*
 * Copyright 1993-2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and 
 * proprietary rights in and to this software and related documentation. 
 * Any use, reproduction, disclosure, or distribution of this software 
 * and related documentation without an express license agreement from
 * NVIDIA Corporation is strictly prohibited.
 *
 * Please refer to the applicable NVIDIA end user license agreement (EULA) 
 * associated with this source code for terms and conditions that govern 
 * your use of this NVIDIA software.
 * 
 */

//Standard utilities and systems includes
#include <oclUtils.h>
#include "oclDCT8x8_common.h"

////////////////////////////////////////////////////////////////////////////////
// Main program
////////////////////////////////////////////////////////////////////////////////
int main(int argc, const char **argv){
    cl_context       cxGPUContext; //OpenCL context
    cl_command_queue cqCommandQue; //OpenCL command que
    cl_mem      d_Input, d_Output; //OpenCL memory buffer objects

    size_t dataBytes;
    cl_int ciErrNum;

    float *h_Input, *h_OutputCPU, *h_OutputGPU;

    const uint
        imageW = 2048,
        imageH = 2048,
        stride = 2048;

    const int dir = DCT_FORWARD;

    // set logfile name and start logs
    shrSetLogFileName ("oclDCT8x8.txt");
    shrLog(LOGBOTH, 0.0, "%s Starting...\n\n", argv[0]); 

    shrLog(LOGBOTH, 0.0, "Allocating and initializing host memory...\n");
        h_Input     = (float *)malloc(imageH * stride * sizeof(float));
        h_OutputCPU = (float *)malloc(imageH * stride * sizeof(float));
        h_OutputGPU = (float *)malloc(imageH * stride * sizeof(float));
        srand(2009);
        for(uint i = 0; i < imageH; i++)
            for(uint j = 0; j < imageW; j++)
                h_Input[i * stride + j] = (float)rand() / (float)RAND_MAX;

    shrLog(LOGBOTH, 0.0, "Initializing OpenCL...\n");
        cxGPUContext = clCreateContextFromType(0, CL_DEVICE_TYPE_GPU, NULL, NULL, &ciErrNum);
        shrCheckError (ciErrNum, CL_SUCCESS);

        //Get the list of GPU devices associated with context
        ciErrNum = clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, 0, NULL, &dataBytes);
        cl_device_id *cdDevices = (cl_device_id *)malloc(dataBytes);
        ciErrNum |= clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, dataBytes, cdDevices, NULL);
        shrCheckError (ciErrNum, CL_SUCCESS);

        //Create a command-queue
        cqCommandQue = clCreateCommandQueue(cxGPUContext, cdDevices[0], 0, &ciErrNum);
        shrCheckError (ciErrNum, CL_SUCCESS);

        //Discard temp storage
        free(cdDevices);

    shrLog(LOGBOTH, 0.0, "Initializing OpenCL DCT 8x8...\n");
        initDCT8x8(cxGPUContext, cqCommandQue, argv);

    shrLog(LOGBOTH, 0.0, "Creating OpenCL memory objects...\n");
        d_Input = clCreateBuffer(cxGPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageH * stride *  sizeof(cl_float), h_Input, &ciErrNum);
        shrCheckError (ciErrNum, CL_SUCCESS);
        d_Output = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, imageH * stride * sizeof(cl_float), NULL, &ciErrNum);
        shrCheckError (ciErrNum, CL_SUCCESS);

    shrLog(LOGBOTH, 0.0, "Performing DCT8x8 of %u x %u image...\n", imageH, imageW);
        DCT8x8(
            NULL,
            d_Output,
            d_Input,
            stride,
            imageH,
            imageW,
            dir
        );

    shrLog(LOGBOTH, 0.0, "Reading back OpenCL results...\n\n");
        ciErrNum = clEnqueueReadBuffer(cqCommandQue, d_Output, CL_TRUE, 0, imageH * stride * sizeof(cl_float), h_OutputGPU, 0, NULL, NULL);
        shrCheckError (ciErrNum, CL_SUCCESS);

    shrLog(LOGBOTH, 0.0, "Comparing against Host/C++ computation...\n"); 
        DCT8x8CPU(h_OutputCPU, h_Input, stride, imageH, imageW, dir);
        double sum = 0, delta = 0;
        double L2norm;
        for(uint i = 0; i < imageH; i++)
            for(uint j = 0; j < imageW; j++){
                sum += h_OutputCPU[i * stride + j] * h_OutputCPU[i * stride + j];
                delta += (h_OutputGPU[i * stride + j] - h_OutputCPU[i * stride + j]) * (h_OutputGPU[i * stride + j] - h_OutputCPU[i * stride + j]);
            }
        L2norm = sqrt(delta / sum);
        shrLog(LOGBOTH, 0.0, "Relative L2 norm: %.3e\n\n", L2norm);
        shrLog(LOGBOTH, 0.0, "TEST %s\n\n", (L2norm < 1E-6) ? "PASSED" : "FAILED !!!");

    shrLog(LOGBOTH, 0.0, "Shutting down...\n");
        //Release kernels and program
        closeDCT8x8();

        //Release other OpenCL objects
        ciErrNum  = clReleaseMemObject(d_Output);
        ciErrNum |= clReleaseMemObject(d_Input);
        ciErrNum |= clReleaseCommandQueue(cqCommandQue);
        ciErrNum |= clReleaseContext(cxGPUContext);
        shrCheckError (ciErrNum, CL_SUCCESS);

        //Release host buffers
        free(h_OutputGPU);
        free(h_OutputCPU);
        free(h_Input);

        //Finish
        shrEXIT(argc, argv);
}
