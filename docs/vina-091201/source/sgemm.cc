/* two matrices of size [M,P], [P,N] and type T */
template <class T, int M, int P, int N,
     /* predicate deciding if recursion is taken */ 
     template <class, class> class PRED,
     int K /*param to divide task*/>
struct SGEMM {
  typedef ReadView<T, M, P>  ARG0;
  typedef ReadView<T, P, N>  ARG1;
  typedef WriteView<T, M, N> RESULT;

  typedef SGEMM<T, M, P, N, PRED, K> SELF;
  typedef TF_hierarchy<SELF, PRED> TF;

  //interface for programmer
  void operator()(const Matrix<T, M, P>& A, 
                const Matrix<T, P, N>& B,
                Matrix<T, M, N>& C) {
    TF::doit(A, B, C.SubViewW());
  }

  //static entry for TF
  static void inner(ARG0 A, ARG1 B, RESULT C) {
    //define SubTask here
    typedef SGEMM<T, M/K, P/K, N/K, PRED, K> SubTask;
    typedef typename SubTask::TF SubTF;
    typedef Matrix<T, M/K, N/K> SubMatrix; 

    //lambda for iteration, row i, column j
    auto subtask = [&](int i, int j) {
      SubMatrix tmps[K];
      //lambda for division
      auto m = [&](int n) {
        tmps[n].zero(); //initialize
        SubTF::doit(
           A.template SubViewR<M/K,P/K>(i, n), 
           B.template SubViewR<P/K,N/K>(n, j),
           tmps[n].SubViewW());
      };
      //perform K operations
      par<_tail, K, decltype(m)&> ::apply(m);
      //sum up temporaries to submatrix of C. 
      reduce<K, plus<SubMatrix>>
        (tmps, C.template<M/K, N/K>SubViewW(i, j));
    };
    //calculate each submatrix in parallel 
    typedef decltype(subtask)& closure_t;
    par<par<_tail, K>, K, closure_t>
      ::apply(subtask);  
  }/*end func*/

  //static entry for TF
  static void leaf(ARG0 A, ARG1 B, RESULT C) {
    // compute matrix product directly
    for (int i=0; i<M; ++i)
      for (int j=0; j<N; ++j)
        for (int k=0; k<P; ++k)
          C[i][j]+=A[i][k]*B[k][j];
  }
};
