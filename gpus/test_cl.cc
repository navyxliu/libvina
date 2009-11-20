#include <cstdlib> // for rand()

#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <boost/lambda/lambda.hpp>

#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"


#include "toolkits.hpp"

using namespace std;
using namespace boost;
using namespace boost::lambda;

#define DATA_SIZE 1024

const char * helloStr = "__kernel void "
                        "hello(void) "
                        "{ "
                        "  "
                        "} ";

// Simple compute kernel which computes the square of an input array 
//
const char *SquareThem = "\n" \
"__kernel square(                                                       \n" \
"   __global float* input,                                              \n" \
"   __global float* output,                                             \n" \
"   const unsigned int count)                                           \n" \
"{                                                                      \n" \
"   int i = get_global_id(0);                                           \n" \
"   output[i] = input[i] * input[i];                                    \n" \
"}                                                                      \n" \
"\n";

int
main(int argc, char * argv[])
{
  vector<cl::Platform> pfs;
  cl::Platform::get(&pfs);
  cout << "the number of platform: " << pfs.size() << endl;

  int i = 0;
  cl_int err = CL_SUCCESS;

  for (auto I = pfs.begin();
       I != pfs.end(); 
       ++I, ++i) {

    try {
      auto ret = I->getInfo<CL_PLATFORM_NAME>(NULL);
      auto ver = I->getInfo<CL_PLATFORM_VERSION>(NULL);
      auto prof = I->getInfo<CL_PLATFORM_PROFILE>(NULL);
      cout << "platform #" << i << "\t" << ret 
	   << "\n version: " << ver 
	   << "\n profile: " << prof
	   << endl;
      
      vector<cl::Device> dev;
      int j = 0;
      I->getDevices(CL_DEVICE_TYPE_GPU, &dev);
      for (auto J = dev.begin(); 
	   J != dev.end();
	   ++J) {
	auto id = J->getInfo<CL_DEVICE_VENDOR_ID>(NULL);
	auto unit = J->getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(NULL);
	auto max_mem = J->getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>(NULL);
	auto local_mem = J->getInfo<CL_DEVICE_LOCAL_MEM_SIZE>(NULL);
	cout << "GPU Device # " << j << "\t" << id
	     << "\n maximal computation unit: " << unit
	     << "\n maximal allocated memory: " << (max_mem >> 20) << "M"
	     << "\n local memory size: " << (local_mem >> 10) << "K"
	     << endl;
      }
    }
    catch ( cl::Error& e)
    {
      cerr << e.what();
      exit(2);
    }
  }
  
  cout << "test helleStr" << endl;
  try {
    cl::Context ctx(CL_DEVICE_TYPE_GPU, 0, NULL, NULL, &err);
    vector<cl::Device> devices = ctx.getInfo<CL_CONTEXT_DEVICES>();
    cl::Program::Sources source(1, std::make_pair(helloStr, strlen(helloStr)));
    cl::Program program_ = cl::Program(ctx, source);
    program_.build(devices);
    cl::Kernel kernel(program_, "hello", &err);
    cl::CommandQueue queue(ctx, devices[0], 0, &err);
    cl::KernelFunctor func = kernel.bind(queue, 
					 cl::NDRange(4, 4),
					 cl::NDRange(2, 2));
    func().wait();
  }
  catch( cl::Error e )
  {
    cerr << "ERROR: "
	 << e.what()
	 << "("
	 << e.err()
	 << ")"
	 << endl;
  }
  

  cout << "test SequareThem" << endl;
  try {
    float data[DATA_SIZE], result[DATA_SIZE];
    unsigned int count = DATA_SIZE;
    int local, global;

    std::generate(data, data+DATA_SIZE, vina::NumRandGen<float>());

    cl::Context ctx(CL_DEVICE_TYPE_GPU, 0, NULL, NULL, &err);
    vector<cl::Device> dev = ctx.getInfo<CL_CONTEXT_DEVICES>();
    cl::Program::Sources src(1, std::make_pair(SquareThem, strlen(SquareThem)));
    cl::Program prog = cl::Program(ctx, src);
    prog.build(dev);

    cl::Kernel kerl(prog, "square", &err);
    cl::CommandQueue queue(ctx, dev[0], 0, &err);
    cl::Buffer input(ctx, CL_MEM_READ_ONLY, sizeof(float)*count, NULL, &err);
    cl::Buffer output(ctx, CL_MEM_WRITE_ONLY, sizeof(float)*count, NULL, &err);
    queue.enqueueWriteBuffer(input, CL_TRUE, 0, sizeof(float)*count, data);
    
    global = count;
    local = kerl.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(dev[0]);
    cout << "global " << global << "\n"
	 << "local "  << local << endl;

    //kerl.getWorkGroupInfo<int>(dev[0], CL_KERNEL_WORK_GROUP_SIZE, &local);

    auto func = kerl.bind(queue, 
			  cl::NDRange(global),
			  cl::NDRange(local));
    auto evt = func(input(), output(), count);
    
    evt.wait();
    
    queue.enqueueReadBuffer(output, CL_TRUE, 0, sizeof(float)*count, result);

    for (int i=0; i<count; ++i) if ( fabs(result[i] - data[i] * data[i]) > 1e-6 ) 
				  {
				    cerr << "WRONG ANSWER!" << endl;
				    break;
				  }

    cl_ulong beg, end;
    clGetEventProfilingInfo(evt(), CL_PROFILING_COMMAND_END, 
			    sizeof(cl_ulong), &end, NULL);
    clGetEventProfilingInfo(evt(), CL_PROFILING_COMMAND_START,
			    sizeof(cl_ulong), &beg, NULL);
    float execTime = (end - beg) * 1.0e-6f;
    cout << "execution time in milliseconds " << execTime << endl;
  }
  catch( cl::Error e) {
    cerr << "ERROR: "
	 << e.what()
	 << "("
	 << e.err()
	 << ")"
	 << endl;
  }
  
  cout << "Test Passed.\n";
  return 0;
}
