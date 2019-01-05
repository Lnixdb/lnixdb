#ifndef _DBM_H
#define _DBM_H
#include "db.h"
#include "client.h"
#include "requestandreply.h"
class Client;

class Dbm{
 public :
	Dbm() = default;
	~Dbm() { }
	Dbm(const Dbm&) = delete;
	const Dbm& operator=(const Dbm&) = delete;

	int ExecuteCommand(Client *cli);	
	int store(RequestAndReply* rap);
	int open(RequestAndReply* rap);
	int close(RequestAndReply* rap);
	int fetch(RequestAndReply* rap);
	int delet(RequestAndReply* rap);   	//Conflicts with the c++ delete keyword, named delet
	int rewind(RequestAndReply* rap);
	int nextrec(RequestAndReply* rap);

	~Dbm(){
		db_close(db_handle_);
	}

 private:
	DB *db_handle_;
};

#endif
