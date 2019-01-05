#ifndef _CLIENT_H
#define _CLIENT_H
#include <map>
#include "dbm.h"
#include "requestandreply.h"
#include<event2/bufferevent.h>
#include<event2/buffer.h>

#define MASTER 1
#define SLAVE  0

class Dbm;
class DbmServer;
class RequestAndReply;

class Client{
 public :
	Client(bufferevent *bev);

	Dbm* GetDbmPtr(int dbid);

	int SetMaster() { flag_ = MASTER; }

	int SetSlave() { flag_ = SLAVE; }

	bool IsSlave() { return flag_ == SLAVE; }

	void SetDbname(const std::string name) { dbname_ = name; }

	const std::string& GetDbname() const { return dbname_; }

	bufferevent* GetEvent() { return bev_; }

	RequestAndReply* GetRequestPtr() { return rap_; }

	DbmServer* GetServer() { return server_; }

	void SetServer(DbmServer* server) { server_ = server; } 

 	bufferevent* GetBev() { return bev_; }

	void AddSlaveAddr(const sockaddr_in& addr){ 
		slaves_addr_.push_back(addr); 
	}

	~Client(){  
		for(auto map_it = dbms_.begin(); map_it != dbms_end(); map_it++){
			delete map_it->second;
		}	
	}
	
 private:
	std::vector<sockaddr_in> slaves_addr_;
	// database name
	std::string dbname_;

	// Marks whether the client is slave or normal read and write client
	// 0 : slave
	// 1 : master
	int flag_;

	// Readable or writable
	bufferevent *bev_;
	// Different databases open by the same client
	std::map<int, Dbm*> dbms_;

	// Request message from client
	RequestAndReply *rap_;

	//Refers back to the server object
	DbmServer *server_;
};

#endif 
