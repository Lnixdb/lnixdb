#ifndef _CLIENT_H
#define _CLIENT_H
#include <map>
#include "dbm.h"
#include "requestandreply.h"
#include<event2/bufferevent.h>
#include<event2/buffer.h>

class Dbm;
class RequestAndReply;

class Client{
 public :
	Client(bufferevent *bev);
	~Client() {
		for(auto map_it = dbms_.begin(); map_it != dbms_.end(); map_it++){
			delete map_it->second;
		}
	}
	Dbm* GetDbmPtr(int dbid);

	RequestAndReply* GetRequestPtr() { return rap_; }
 private:
	// bev_ represents client readable events
	bufferevent *bev_;

	// Different databases opened by the same client are identified by id, 
	// which is generated by the client.
	std::map<int, Dbm*> dbms_;

	//The client requests the message.
	RequestAndReply *rap_;
};

#endif 