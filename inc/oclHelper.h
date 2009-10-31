// Helper procedures in using opencl API.
// This is a simple complement for oclUtils.h
// I shamelessly add the procedures to liboclUtil.a file to extend its functionalities.


#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#include <OpenGL/opengl.h>
#include <OpenCL/cl_gl.h>
#else
#include <CL/cl.h>
#include <GL/gl.h>
#include <CL/cl_gl.h>
#endif // !__APPLE__

#ifdef __cplusplus
extern "C"{
#endif

extern float executionTime(cl_event e);

#ifdef __cplusplus
}
#endif
