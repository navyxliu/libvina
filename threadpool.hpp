//author: ydf
///HISTORY:
// 2009.7.15  fix a bug when buffer size smaller than thread number, dead lock happen.

#ifndef __BST_THREAD_POOL__
#define __BST_THREAD_POOL__
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <queue>
#include <map>
namespace vina{
  namespace mt{
    boost::mutex __io_mutex;
    
    template <typename T>
    class task_buffer 
    { 
      typedef boost::mutex::scoped_lock scoped_lock; 
      typedef T value_type;
    public:  
      task_buffer(size_t size, boost::condition& cond)
		  : _MAX_BUFFER(size)
        , empty_cond(cond)
      {}
      
      bool empty() const
      {
        return buf.empty();
      }
      
      void put(const T &m) 
      { 
        scoped_lock lock(mutex);        
        
        while( buf.size() == _MAX_BUFFER )
          cond.wait(lock); 
        buf.push(m);                            
        cond.notify_one();                      
      }

      T get()
      { 
        scoped_lock lock(mutex);
        
        while(buf.empty())              
     
          cond.wait(lock); 
        T t = buf.front();                      
        buf.pop();
        cond.notify_one();                      
        
        if(buf.empty())
          empty_cond.notify_one();
        return t; 
      } 
      
    private:
	const size_t _MAX_BUFFER;
      boost::mutex mutex; 
      boost::condition cond;
      boost::condition& empty_cond;
      std::queue<T> buf;
    };
    
    class thread_pool
    {
    public:
      thread_pool(int max_thread, int max_task = 10)
        : _max_thread_num(max_thread)
        , _stop(true)
		, _task_buffer(max_task > max_thread ? max_task : max_thread, cond)
      {}
      
    public:
        void run(const boost::function<void (void)> &fun)
      {
        _task_buffer.put(fun);
        if(_threads.size() < _max_thread_num)
          {
            boost::shared_ptr<boost::thread> p(new boost::thread(boost::bind(&thread_pool::thread_proc, this)));
            _threads.push_back(p);
            }
      }
      
      ~thread_pool()
      {

        boost::mutex::scoped_lock lock(mutex);
        while(!_task_buffer.empty())
          cond.wait(lock);
        
        
        _stop = false;
        for(int i = 0, j = _threads.size(); i < j; ++i)
          {
            _task_buffer.put(boost::bind(&thread_pool::stop_proc, this));
          }
        for(int i = 0, j = _threads.size(); i < j; ++i)
          {
            _threads[i]->join();
          }
      }

    private:
      void thread_proc()
      {
        while(_stop)
          {
            boost::function<void (void)> task = _task_buffer.get();
            task();
          }
      }
      
      void stop_proc(void)
      {}
    private:
      boost::mutex mutex;
      boost::condition cond;
      bool _stop;
      std::size_t _max_thread_num;
      std::vector<boost::shared_ptr<boost::thread>> _threads;
      task_buffer<boost::function<void (void)>> _task_buffer;
    };
  }//endof NS
}//endof NS
#endif //__BST_THREAD_POOL__
