#ifndef _THREAD_POOL
#define _THREAD_POOL
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

class ThreadPool{
 public :	 
	ThreadPool();
	ThreadPool(int thread_num);
	ThreadPool(const ThreadPool &) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	bool QueueEmpty(){ return queue_work_.empty(); }
	/* 
 	 * When a readable event occurs, libevent returns a bev pointer and 
 	 * queues for processing by the thread
 	*/
	void AddQueue(bufferevent* bev, void *arg = NULL);
	/*
 	 * free thread to process bev, return the index of the thread in vector
 	 * return -1 means there is no free thread
 	*/
	int FindFreeThread();
	bool HasFreeThread();
	/*
 	 * A readable event has occurred notifying a thread of processing*
 	*/
	void NotifyOne();
	/* 
 	 * The user should implement the processing logic when a readable event occurs
 	*/
	virtual int ProcessFdEvent(bufferevent *bev) = 0;

	/* Each thread has a ThreadLock structure and a private mu_, cv_*/
	struct ThreadLock{
		int flag_;		//only for debug
		bool idle_;		//thread free flag
		std::mutex	mutex_;
		std::condition_variable	cv_;
		void Wait(){
			std::unique_lock<std::mutex>	ul(mutex_);
			cv_.wait(ul);
		}
		void Singnal(){
			cv_.notify_one();
		}
		bool IsBusy() { return !idle_; }
		void SetFree() { idle_ = true; }
		void SetBusy() { idle_ = false; }
		ThreadLock(int flag) : flag_(flag), idle_(true) {} ;
	};
 private:
	static void WorkThreadEntry(ThreadPool *tp, int index){
		tp->WorkThreadMain(tp->thread_lock_pool_[index]);
	}
	void WorkThreadMain(ThreadLock *tl);
	int	thread_num_;
	int thread_load_balance_;
	std::vector<ThreadLock*> thread_lock_pool_;
	std::queue<bufferevent*> queue_work_;
	std::mutex	queue_mutex_;
};

#endif
