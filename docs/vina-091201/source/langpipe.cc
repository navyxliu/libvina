//template full specialization
template<>
struct TF_pipeline<>
{
  //last stage definitions
  static void impl(ReadViewMT *input) {
    //omitted...
  }

  static void doit(ReadViewMT *input) {
    std::tr1::function<void (ReadViewMT*)> 
      func(&impl);

    mt::thread_t thr(func, input);
  } 
};

//customized TF_pipeline class
typedef TF_pipeline<stage1, stage2,
                    stage3, stage4> MYPIPE;

MYPIPE::doit(&input);
