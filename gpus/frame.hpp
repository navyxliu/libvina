//This file is not supposed to be distribued.
#include "../cl.hpp"
#include <vector>
#include <cstdlib>
using namespace std;

namespace vina{
  template <class Instance>
  struct mappar{
    static void doit(const typename Instance::Arg0& arg0, 
		     const typename Instance::Arg1& arg1, 
		     typename Instance::Result& result) {
      cl_int err;
      try {
      cl::Context ctx(CL_DEVICE_TYPE_GPU, 0, NULL, NULL, &err);
      vector<cl::Device> dev = ctx.getInfo<CL_CONTEXT_DEVICES>();
      auto code = Instance::computation();
      cl::Program::Sources src(1, std::make_pair(code, strlen(code)));
      cl::Program prog = cl::Program(ctx, src);
      prog.build(dev);
      
      cl::CommandQueue queue(ctx, dev[0], 0, &err);
      cl::Buffer arg0(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
		      sizeof(view_trait<Instance::Arg0>::value_type 
			     * view_trait<Instance::Arg0>::READER_SIZE ), 
		      arg0.data(),  &err);
      cl::Buffer arg1(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		      sizeof(view_trait<Instance::Arg1>::value_type 
			     * view_trait<Instance::Arg1>::READER_SIZE), 
		      arg1.data(), &err);

      cl::Buffer result(ctx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
		      sizeof(view_trait<Instace::Result>::value_type 
			     * view_trait<Instance::Result>::WRITER_SIZE),
			result.data(), &err);

      cl::Kernel kerl(prog, "square", &err);
      int global = view_trait<Instance::Result>::WRITER_SIZE;
      int local = kerl.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(dev[0]);
      auto func = kerl.bind(queue, 
			    cl::NDRange(global),
			    cl::NDRange(local));
      
      aut evt = func(arg0(), arg1(), result());
      
      evt = evt.wait();

      }
      catch ( cl::Error & e) {
	cerr << "ERROR: "
	     << e.what()
	     << "("
	     << e.err()
	     << ")"
	     << endl;
      }
    }
  };
}
