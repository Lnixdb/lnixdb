#include <iostream>
#include "requestandreply.h"
#include "db.h"
#include "client.h"
#include "dbm.h"

int Dbm::ExecuteCommand(Client *cli){
	if(!cli->GetRequestPtr()->GetFunctionName().compare("open"))
		this->open(cli->GetRequestPtr());
		
	else if(!cli->GetRequestPtr()->GetFunctionName().compare("store"))
		this->store(cli->GetRequestPtr());
		
	else if(!cli->GetRequestPtr()->GetFunctionName().compare("fetch"))
		this->fetch(cli->GetRequestPtr());

	else if(!cli->GetRequestPtr()->GetFunctionName().compare("delet"))
		this->delet(cli->GetRequestPtr());

	else if(!cli->GetRequestPtr()->GetFunctionName().compare("close"))
		this->close(cli->GetRequestPtr());

	else if(!cli->GetRequestPtr()->GetFunctionName().compare("rewind"))
		this->rewind(cli->GetRequestPtr());

	else if(!cli->GetRequestPtr()->GetFunctionName().compare("nextrec"))
		this->nextrec(cli->GetRequestPtr());

	else if(!cli->GetRequestPtr()->GetFunctionName().compare("masterinfo"))
		this->MasterInfo(cli);

	else if(!cli->GetRequestPtr()->GetFunctionName().compare("PONG"))
		this->pong(cli);
}

void Dbm::pong(Client* cli){
	cli->SetMaster();
	cli->ClearTimer();
}

void Dbm::MasterInfo(Client* cli){

	RequestAndReply *rp = cli->GetRequestPtr();
	std::string ip;
	short port;
	
	cli->SetMaster();
	for(auto vec_it = rp->GetFunctionArgs().begin();
		 vec_it != rp->GetFunctionArgs().end(); vec_it++){
		size_t i = 0;
		for(auto c : *vec_it){
			if(c != ':'){
				ip += c;
				i++;
				continue;
			}
			else{
				port = std::stoi(std::string(*vec_it, i+1));
				break;
			}
		}
		cli->MakeSockaddr(ip, port);
	}

	rp->SetRequestReply("masterinfo ok");
}

int Dbm::open(RequestAndReply* rp)
{
	if(rp->GetFunctionArgs().size() != 1){
                return -1;
        }
        db_handle_ = (DB*)db_open(rp->GetFunctionArgs()[0].c_str(),\
				  O_RDWR | O_CREAT | O_TRUNC, FILE_MODE);
        
	rp->SetRequestReply("open ok");
	return 0;
}

int Dbm::fetch(RequestAndReply* rp)
{
	if(rp->GetFunctionArgs().size() != 1){
                return -1;
        }
	if(db_handle_ == NULL){
		rp->SetRequestReply("the database is not open");
		return 0;
	}
        const char *ret = db_fetch(db_handle_, rp->GetFunctionArgs()[0].c_str());
	if(ret != NULL)
		rp->SetRequestReply(ret);
	else	
		rp->SetRequestReply("(NULL)");
	
	return 0;
}

int Dbm::store(RequestAndReply* rp)
{
        if(rp->GetFunctionArgs().size() != 3)
                return -1;
        
	if(db_handle_ == NULL){
		rp->SetRequestReply("the database is not open");
		return 0;
	}
	int ret = db_store(db_handle_, rp->GetFunctionArgs()[0].c_str(), \
			   rp->GetFunctionArgs()[1].c_str(), 
			   std::stoi(rp->GetFunctionArgs()[2], NULL));
	
	rp->SetRequestReply("store ok");
	return 0;	
}

int Dbm::delet(RequestAndReply* rp)
{
	if(rp->GetFunctionArgs().size() != 1){
                return -1;
        }
	if(db_handle_ == NULL){
		rp->SetRequestReply("the database is not open");
		return 0;
	}
        int ret = db_delete(db_handle_, rp->GetFunctionArgs()[0].c_str());
	rp->SetRequestReply("delete ok");
	
	return 0;
}

int Dbm::close(RequestAndReply* rp)
{
	if(rp->GetFunctionArgs().size() != 0){
                return -1;
        }
	if(db_handle_ == NULL){
		rp->SetRequestReply("the database is not open");
		return 0;
	}
        db_close(db_handle_);
	rp->SetRequestReply("close ok");
	
	return 0;
}
int Dbm::rewind(RequestAndReply* rp)
{
	if(rp->GetFunctionArgs().size() != 0){
                return -1;
        }
	if(db_handle_ == NULL){
		rp->SetRequestReply("the database is not open");
		return 0;
	}
        db_rewind(db_handle_);
	rp->SetRequestReply("rewind ok");
	
	return 0;
}


int Dbm::nextrec(RequestAndReply* rp)
{
	if(rp->GetFunctionArgs().size() != 1){
                return -1;
        }
	if(db_handle_ == NULL){
		rp->SetRequestReply("the database is not open");
		return 0;
	}
        const char *ret = db_nextrec(db_handle_, (char *)(rp->GetFunctionArgs()[0].c_str()));
	if(ret != NULL)
		rp->SetRequestReply(ret);
	else	
		rp->SetRequestReply("(NULL)");
	
	return 0;
}

