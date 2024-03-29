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

#include <oclUtils.h>
#include "oclDCT8x8_common.h"

////////////////////////////////////////////////////////////////////////////////
// OpenCL launcher for DCT8x8 / IDCT8x8 kernels
////////////////////////////////////////////////////////////////////////////////
//OpenCL DCT8x8 program
static cl_program
    cpDCT8x8;

//OpenCL DCT8x8 kernels
static cl_kernel
    ckDCT8x8, ckIDCT8x8;

//Default command queue for DCT8x8 kernels
static cl_command_queue cqDefaultCommandQue;

extern "C" void initDCT8x8(cl_context cxGPUContext, cl_command_queue cqParamCommandQue, const char **argv){
    cl_int ciErrNum;
    size_t kernelLength;

    shrLog(LOGBOTH, 0.0, "Loading OpenCL DCT8x8...\n");
        char *cPathAndName = shrFindFilePath("DCT8x8.cl", argv[0]);
        shrCheckError(cPathAndName != NULL, shrTRUE);
        char *cDCT8x8 = oclLoadProgSource(cPathAndName, "// My comment\n", &kernelLength);
        shrCheckError(cDCT8x8 != NULL, shrTRUE);

    shrLog(LOGBOTH, 0.0, "Creating DCT8x8 program...\n");
        cpDCT8x8 = clCreateProgramWithSource(cxGPUContext, 1, (const char **)&cDCT8x8, &kernelLength, &ciErrNum);
        shrCheckError (ciErrNum, CL_SUCCESS);

    shrLog(LOGBOTH, 0.0, "Building DCT8x8 program...\n");
        ciErrNum = clBuildProgram(cpDCT8x8, 0, NULL, NULL, NULL, NULL);
        shrCheckError (ciErrNum, CL_SUCCESS);

    shrLog(LOGBOTH, 0.0, "Creating DCT8x8 kernels...\n");
        ckDCT8x8 = clCreateKernel(cpDCT8x8, "DCT8x8", &ciErrNum);
        shrCheckError (ciErrNum, CL_SUCCESS);
        ckIDCT8x8= clCreateKernel(cpDCT8x8, "IDCT8x8", &ciErrNum);
        shrCheckError (ciErrNum, CL_SUCCESS);

    //Save default command queue
    cqDefaultCommandQue = cqParamCommandQue;

    //Discard temp storage
    free(cDCT8x8);
}

extern "C" void closeDCT8x8(void){
    cl_int ciErrNum;

    ciErrNum  = clReleaseKernel(ckIDCT8x8);
    ciErrNum |= clReleaseKernel(ckDCT8x8);
    ciErrNum |= clReleaseProgram(cpDCT8x8);
}

inline uint iDivUp(uint dividend, uint divisor){
    return dividend / divisor + (dividend % divisor != 0);
}

extern "C" void DCT8x8(
    cl_command_queue cqCommandQue,
    cl_mem d_Dst,
    cl_mem d_Src,
    cl_uint stride,
    cl_uint imageH,
    cl_uint imageW,
    cl_int dir
){
    if(!cqCommandQue)
        cqCommandQue = cqDefaultCommandQue;

    shrCheckError((dir == DCT_FORWARD) || (dir == DCT_INVERSE), CL_TRUE);
    cl_kernel ckDCT = (dir == DCT_FORWARD) ? ckDCT8x8 : ckIDCT8x8;

    const uint BLOCK_X = 32;
    const uint BLOCK_Y = 16;

    size_t localWorkSize[2], globalWorkSize[2];
    cl_uint ciErrNum;

    ciErrNum  = clSetKernelArg(ckDCT, 0, sizeof(cl_mem),  (void*)&d_Dst);
    ciErrNum |= clSetKernelArg(ckDCT, 1, sizeof(cl_mem),  (void*)&d_Src);
    ciErrNum |= clSetKernelArg(ckDCT, 2, sizeof(cl_uint), (void*)&stride);
    ciErrNum |= clSetKernelArg(ckDCT, 3, sizeof(cl_uint), (void*)&imageH);
    ciErrNum |= clSetKernelArg(ckDCT, 4, sizeof(cl_uint), (void*)&imageW);
    shrCheckError(ciErrNum, CL_SUCCESS);

    localWorkSize[0] = BLOCK_X;
    localWorkSize[1] = BLOCK_Y / BLOCK_SIZE;
    globalWorkSize[0] = iDivUp(imageW, BLOCK_X) * localWorkSize[0];
    globalWorkSize[1] = iDivUp(imageH, BLOCK_Y) * localWorkSize[1];

    ciErrNum = clEnqueueNDRangeKernel(cqCommandQue, ckDCT, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
    shrCheckError (ciErrNum, CL_SUCCESS);
}
