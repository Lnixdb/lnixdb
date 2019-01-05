#ifndef _CLIENT_H
#define _CLIENT_H
#include <map>
#include <arpa/inet.h>
#include "dbm.h"
#include "requestandreply.h"
#include<event2/bufferevent.h>
#include<event2/buffer.h>

#define MASTER 1
#define CLIENT 0

class Dbm;
class RequestAndReply;
class DbmServer;

class Client{
 public :
	Client(bufferevent *bev);
	Dbm* GetDbmPtr(int dbid);

	struct addr{
	 std::string ip_;
	 short port_;	
	};
	void SetMaster() { flag_ = MASTER; }
	
	bool IsMaster() { return flag_ == MASTER; }	

	void SetServerPtr(DbmServer *server) { server_ = server;}

	DbmServer* GetServerPtr() { return server_; }

	RequestAndReply* GetRequestPtr() { return rap_; }

	std::vector<addr>& GetSlaves() { return  slaves_; }

	bufferevent* GetBev() { return bev_; }

	void SetAlive() { alive_ = true; }
	void SetNotAlive() { alive_ = false; }

	void ClearTimer() { timer_ = 0; }

	void IncTimer() { timer_++; }

	size_t GetTimer() { return timer_; } 

	void MakeSockaddr(std::string& ip, short port){
                struct addr ad;
		ad.ip_ = ip;
		ad.port_ = port;
                slaves_.push_back(ad);
        }
	
 private:
	int flag_;
	bool alive_;
	size_t timer_;
	bufferevent *bev_;
	std::map<int, Dbm*> dbms_;
	RequestAndReply *rap_;
	DbmServer *server_;
	std::vector<addr> slaves_;
};

#endif 
