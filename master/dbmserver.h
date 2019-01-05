#ifndef _DBM_SERVER_
#define _DBM_SERVER_
#include <map>
#include <set>
#include <iostream>
#include "db.h"
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
	ThreadPool* GetThreadPoolPtr() { return tp_; }
	
	Client* LookupClient(bufferevent *bev);
	// All handles to open the database
	std::set<DB*>	handles_;

	// Each bufferevent* represents a client
	std::map<bufferevent*, Client*> clients_;

	// Each bufferevent* represents a file descriptor
	std::map<bufferevent*, int> fds_;

	std::set<std::string> dbs_;

	// fd -> sockaddr
	std::map<int, sockaddr_in> remote_addr_;

	// all clients
	std::vector<Client*> clientptrs_;
 private:
	// thradpool
	ThreadPool *tp_;
};
#endif
