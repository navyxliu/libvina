#include "mtsupport.hpp"
namespace vina {
  namespace mt{
    barrier_t trivial_barrier(new boost::barrier(1));
    barrier_t null_barrier;

    class  signal::impl{
      bool ready_;
      cond_variable_t cond_;
      mutex_t * mutex_;

      impl(const impl&);
      impl& operator=(const impl&);
    public:
      impl() : ready_(false), cond_(new boost::condition_variable),
        mutex_(new mutex_t){}

      void set() {
        {
          boost::lock_guard<boost::mutex>(*mutex_);
          ready_ = true;
        }       
        cond_->notify_all();
      }
      void wait() const {
        boost::unique_lock<boost::mutex> lock(*mutex_);
        while ( !ready_ ) {
          cond_->wait(lock); // unlock internally
        }
      }
    };
    signal::signal() : pimpl_(new impl) {}
    void signal::set() { pimpl_->set(); }
    void signal::wait() { pimpl_->wait(); }
  }// end of NS mt
}// end of NS
