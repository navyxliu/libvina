// This file is not supposed to be distributed.
#ifndef VINA_SPMD_HEADER
#define VINA_SPMD_HEADER

#ifdef __cplusplus
extern "C" {
#endif
  /*libspmd intialization
   *return the maximal number of SPMD hardware support.
   *-1 when failed
   */
  extern int spmd_initialize();
  /*kill threads managed by spmd RT.
   *reclaim memory
   *no error is returned.
   */
  extern void spmd_cleanup();
  /*build a new warp. this function may block when no
   *sufficient physical resource is available
   *
   *param:
   *  nr     -- the number of tasks
   *  fn     -- function ptr of task
   *  stk_sz -- specify stack size of task, size==0 uses default value,
   *            depend on impl.
   *  hook   -- hook function to be called when all task completed
   *            give  NULL to omit it.
   *return:
   *  -1 when failed, otherwise,
   *  warp_id
   */
  extern int spmd_create_warp(int nr, void * fn, unsigned int stk_sz/*=0*/,
			      void *hook/*=NULL*/);
  /*next NR calls of specific warp is to mount task
   *in to native threads. when all tasks are ready,
   *runtime automatically fire the warp.
   *param:
   *  id -- warp id
   *  argc    -- the number of variable-length arguments
   *  variable arguments store in task evironment
   *return:
   *  -1 when failed, otherwise,
   *  task_id
   */
  //  extern int spmd_create_thread(int warp_id, int argc, ...);
  extern int spmd_create_thread(int warp_id, void * ret, void * arg0, void * arg1);
  /*check out avaialable threads in runtime.
   *return the maximal number of available PEs. libSPMD RT can not guarantee
   *that spawn threads continuously in returned number is non-blocking. it depends on
   *implementation.
   */
  extern int smpd_available_thread();
  /*
   */
  extern int spmd_all_complete();
#ifdef __cplusplus
}
#endif
#endif/*VINA_SPMD_HEADER*/
