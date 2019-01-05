#ifndef _DBM_SERVER_
#define _DBM_SERVER_
#include <map>
#include <iostream>
#include "client.h"
#include "threadpool.h"
#include "threadpoolimpl.h"
#include<event2/bufferevent.h>
#include<event2/buffer.h>
#include "connect.h"
class Connection;

class DbmServer{
 public :
	DbmServer(){	
		tp_ = new ThreadPoolImpl(1);	//Creates a thread by default
	}
	DbmServer(int thread_num){
		tp_ = new ThreadPoolImpl(thread_num);
	}
	ThreadPool* GetThreadPoolPtr() { return tp_; }
	void ForwardingMsg(RequestAndReply *rap);
	std::vector<Connection*> nodes_;
	Client* LookupClient(bufferevent *bev);
	std::map<bufferevent*, Client*> clients_;
	// bev -> fd
	std::map<bufferevent*, int> fds_;
	
	std::vector<Client*> vec_clients_;
 private:
	ThreadPool *tp_;
};
#endif
