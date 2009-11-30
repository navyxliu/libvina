  template <class T, int M, int P, int N
          template <class, class> 
          class PRED/*predicate*/
          int K,/*param to divide task*/>
  struct SGEMM {
  typedef ReadView<T, M, P>  ARG0;
  typedef ReadView<T, P, N>  ARG1;
  typedef WriteView<T, M, N> RESULT;

  typedef SGEMM<T, M, P, N, PRED, K> SELF;
  typedef TF_hierarchy<SELF, PRED> TF;

  void //interface for programmer
  operator()(const Matrix<T, M, P>& A, 
             const Matrix<T, P, N>& B,
             Matrix<T, M, N>& C)
  {
     TF::doit(A, B, C.SubViewW());
  }

  static void //static entry for TF
  inner(ARG0 A, ARG1 B, RESULT C) {
     //lambda for iteration
     auto subtask = [&](int i, int j) 
     {
       Matrix<T, M/K, N/K> tmps[K];
       //lambda for map
       auto m = [&](int k) {
         TF::doit(
            A.SubViewR<M/K,P/K>(i, k), 
            B.SubViewR<P/K,N/K>(k, j),
            tmps[k].SubViewW(i, j));
       };
       par<par_tail, K, decltype(m)&>
       ::apply(m);
       reduce<K>(tmps, C[i][j]);
    };
      
     typedef decltype(subtask)& closure_t;
     par< par<par_tail, K>, K, closure_t>
     ::apply(par_lv_handler2(subtask));  
   }/*end func*/

   static void //static entry for TF
   leaf(ARG0 A, ARG1 B, RESULT C)
   {
     // compute matrix product directly
     for (int i=0; i<M; ++i)
       for (int j=0; j<N; ++j)
         for (int k=0; k<P; ++k)
           C[i][j]+=A[i][k]*B[k][j];
   }
  };
