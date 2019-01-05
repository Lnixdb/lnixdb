#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>
 
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include<event.h>
#include<event2/util.h>
#include "threadpool.h"

/* thread_load_balance_ is not currently in use,
 * Assign a ThreadLock structure to each thread. 
*/
ThreadPool::ThreadPool() : thread_load_balance_(0), thread_num_(1){
	
	for(int i = 0; i< thread_num_; i++){
		ThreadLock *tl = new ThreadLock(i);
		thread_lock_pool_.push_back(tl);
	}
	/* Specify thread entry */
	for(int i = 0; i < thread_num_; i++){
		std::thread t(WorkThreadEntry, this, i);
		t.detach();
	}

}
ThreadPool::ThreadPool(int thread_num) : thread_load_balance_(0),thread_num_(thread_num){
	int i;
	for(i = 0; i< thread_num_; i++){

		ThreadLock *tl = new ThreadLock(i);
		thread_lock_pool_.push_back(tl);
	}
	for(i = 0; i < thread_num_; i++){
		std::thread t(WorkThreadEntry, this, i);
		t.detach();
	}
}

int ThreadPool::FindFreeThread(){
	int i;
	for(i=0; i< thread_num_; i++){
		if(thread_lock_pool_[i]->IsBusy()) continue;
		else break;
	}
	return (i != thread_num_) ? i : -1;
}

bool ThreadPool::HasFreeThread(){ 
	return FindFreeThread() == -1 ? false : true; 
}

/* Wakes up the specified free thread */
void ThreadPool::NotifyOne(){
	int i = FindFreeThread();
	if(i != -1)
		thread_lock_pool_[i]->Singnal();
}


void ThreadPool::AddQueue(bufferevent *bev, void *arg){

	queue_mutex_.lock();
	if(HasFreeThread())	
		NotifyOne();
	queue_work_.emplace(bev);
	queue_mutex_.unlock();

}

void ThreadPool::WorkThreadMain(ThreadLock *tp){
	while(true){
		if(queue_work_.empty()){		
			//std::cout << tp->flag_ << std::endl; // only for debug 
			tp->Wait();
		}
		/* The thread starts running and sets the busy flag */
		tp->SetBusy();
		queue_mutex_.lock();
		bufferevent *bev = queue_work_.front();
		queue_work_.pop();	
		queue_mutex_.unlock();
		/* Event handling method */
		ProcessFdEvent(bev);

		/* The thread finishes processing and 
                 * sets the idle flag to wait for scheduling 
                */
		tp->SetFree();
	}
}

