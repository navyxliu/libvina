#include "oclHelper.h"

// return executed time in milisecond
float executionTime(cl_event event)
{
  cl_ulong start, end;
    
  clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
  clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    
  return (end - start) * 1e-6f;  /* milisecond */
}
