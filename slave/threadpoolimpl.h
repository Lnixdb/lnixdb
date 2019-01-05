#ifndef _THREAD_POOL_IMPL
#define _THREAD_POOL_IMPL

#include "threadpool.h"

/* Implements a thread class that specifies event processing logic
 * override ProcessFdEvent(bufferevent *bev) 
*/
class ThreadPoolImpl : public ThreadPool{
  public:
	ThreadPoolImpl() : ThreadPool() {}
	ThreadPoolImpl(int num_thread) : ThreadPool(num_thread) {}
	virtual int ProcessFdEvent(bufferevent *bev) override;
};
#endif
