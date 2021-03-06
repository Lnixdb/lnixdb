#include "dbm.h"
#include "requestandreply.h"

Client::Client(bufferevent *bev) : bev_(bev),\
				   rap_(new RequestAndReply()){ 
	SetMaster();
	rap_->SetClient(this);
}

Dbm* Client::GetDbmPtr(int dbid){
	if(dbms_.find(dbid) == dbms_.end()){
		dbms_.insert({dbid, new Dbm});
	}
	return dbms_[dbid];
}
