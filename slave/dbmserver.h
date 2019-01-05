#ifndef _DBM_SERVER_
#define _DBM_SERVER_
#include <map>
#include <iostream>
#include "client.h"
#include "threadpool.h"
#include "threadpoolimpl.h"
#include<event2/bufferevent.h>
#include<event2/buffer.h>

class DbmServer{
 public :
	DbmServer(){	
		tp_ = new ThreadPoolImpl(1);	//Creates a thread by default
	}
	DbmServer(int thread_num){
		tp_ = new ThreadPoolImpl(thread_num);
	}

	~DbmServer() {
		for(auto map_it = clients_.begin(); map_it != clients_end(); map_it++)
			delete map_it->second;
	}

	ThreadPool* GetThreadPoolPtr() { return tp_; }
	Client* LookupClient(bufferevent *bev);
	// event -> client object
	std::map<bufferevent*, Client*> clients_;
 private:
	ThreadPool *tp_;
};
#endif
