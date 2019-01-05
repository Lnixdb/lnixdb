#include <unistd.h>
#include <iostream>
#include "coding.h"
#include "requestandreply.h"
#include "db.h"
#include "client.h"
#include "dbm.h"
#include "dbmserver.h"
#include "coding.h"

class DbmServer;

int Dbm::ExecuteCommand(Client *cli){
	if(!cli->GetRequestPtr()->GetFunctionName().compare("open")){
		cli->SetDbname(cli->GetRequestPtr()->GetFunctionArgs()[0]);
		this->open(cli->GetRequestPtr());
	}
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

	else if(!cli->GetRequestPtr()->GetFunctionName().compare("slaveof"))
		this->slaveof(cli);

	else if(!cli->GetRequestPtr()->GetFunctionName().compare("PING"))
		this->ping(cli);
}

int Dbm::ping(Client* cli){
	std::cout << "recved ping" << std::endl;
	std::string reply("*2\r\n $4\r\nPONG\r\n ");
	EncodeInt(1, reply);
	int fd = cli->GetServer()->fds_[cli->GetBev()];
	::write(fd, reply.c_str(), reply.size());
}

/* Handling replication requests */
int Dbm::slaveof(Client* cli)
{
	// save slave addr
	int fd = cli->GetServer()->fds_[cli->GetBev()];
	cli->AddSlaveAddr(cli->GetServer()->remote_addr_[fd]);

	cli->SetSlave();	// The client belongs to the slave.
	slave_open(cli);	// Open the database on slave
	slave_store(cli, 1);	// All k-v pairs sent to slave
}

/* open database */
int Dbm::open(RequestAndReply* rp)
{
	char retbuf[128];
	DbmServer* server = rp->GetClient()->GetServer();
	Client* cli = rp->GetClient();
	
	if(rp->GetFunctionArgs().size() != 1){
                return -1;
        }
	// all databases set
	server->dbs_.insert(rp->GetFunctionArgs()[0]);

        db_handle_ = (DB*)db_open(rp->GetFunctionArgs()[0].c_str(),\
				  O_RDWR | O_CREAT | O_TRUNC, FILE_MODE);
	rp->GetClient()->GetServer()->handles_.insert(db_handle_);        
	rp->SetRequestReply("open ok");

	/* "open" command forwarding to slaves. 
 	 * There may be multiple slaves, 
 	 * find them and forward the request
 	 */
	for(auto map_it = server->clients_.begin(); 
		 map_it != server->clients_.end(); map_it++){
		 if((map_it->second)->IsSlave()){
			::write(server->fds_[map_it->second->GetBev()], 
			        rp->GetMsgPtr(), rp->GetProcessedLen());	
			// only synchronous
			::read(server->fds_[map_it->second->GetBev()], 
					    retbuf, sizeof(retbuf));
		 }
	}
	return 0;
}

/* Reads the value specified by the key */
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

 /* Store key-value pairs */
int Dbm::store(RequestAndReply* rp)
{
	char key[512];
	char* value = NULL;
	char retbuf[128];

	DbmServer* server = rp->GetClient()->GetServer();
	Client* cli = rp->GetClient();

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
	
	/* "store" command forwarding to slaves. 
 	 * There may be multiple slaves, 
 	 * find them and forward the request
 	 */
	for(auto map_it = server->clients_.begin(); 
		 map_it != server->clients_.end(); map_it++){
		 if((map_it->second)->IsSlave()){
			::write(server->fds_[map_it->second->GetBev()], 
			        rp->GetMsgPtr(), rp->GetProcessedLen());	
			// only for synchronous
			int retlen = ::read(server->fds_[map_it->second->GetBev()], 
					    retbuf, sizeof(retbuf));
		 }
	}
	
	return 0;	
}

/* Delete key-value pairs */
int Dbm::delet(RequestAndReply* rp)
{
	char key[512];
	char* value = NULL;
	char retbuf[128];

	DbmServer* server = rp->GetClient()->GetServer();
	Client* cli = rp->GetClient();

	if(rp->GetFunctionArgs().size() != 1){
                return -1;
        }
	if(db_handle_ == NULL){
		rp->SetRequestReply("the database is not open");
		return 0;
	}
        int ret = db_delete(db_handle_, rp->GetFunctionArgs()[0].c_str());
	rp->SetRequestReply("delete ok");

	/* "delete" command forwarding to slaves. 
 	 * There may be multiple slaves, 
 	 * find them and forward the request
 	 */
	for(auto map_it = server->clients_.begin(); 
		 map_it != server->clients_.end(); map_it++){
		 if((map_it->second)->IsSlave()){
			::write(server->fds_[map_it->second->GetBev()], 
			        rp->GetMsgPtr(), rp->GetProcessedLen());	
			// only for synchronous
			int retlen = ::read(server->fds_[map_it->second->GetBev()], 
					    retbuf, sizeof(retbuf));
		 }
	}

	return 0;
}

/* close database */
int Dbm::close(RequestAndReply* rp)
{
	char key[512];
	char* value = NULL;
	char retbuf[128];

	DbmServer* server = rp->GetClient()->GetServer();
	Client* cli = rp->GetClient();

	if(rp->GetFunctionArgs().size() != 0){
                return -1;
        }
	if(db_handle_ == NULL){
		rp->SetRequestReply("the database is not open");
		return 0;
	}
        db_close(db_handle_);
	rp->SetRequestReply("close ok");

	/* "close" command forwarding to slaves. 
 	 * There may be multiple slaves, 
 	 * find them and forward the request
 	 */
	for(auto map_it = server->clients_.begin(); 
		 map_it != server->clients_.end(); map_it++){
		 if((map_it->second)->IsSlave()){
			::write(server->fds_[map_it->second->GetBev()], 
			        rp->GetMsgPtr(), rp->GetProcessedLen());	
			// only for synchronous
			int retlen = ::read(server->fds_[map_it->second->GetBev()], 
					    retbuf, sizeof(retbuf));
		 }
	}

	return 0;
}

/* Move to the first record */
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

/* The loop calls this method to read all key-value pairs in the database */
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

/* Send master all data to slaves */
int Dbm::slave_store(Client *cli, int flag)
{
        char key[512];
	char* value = NULL;
	char retbuf[256];

	DbmServer* server = cli->GetServer();

	for(auto set_it = server->handles_.begin(); set_it != server->handles_.end(); set_it++){
		db_rewind(*set_it);
		std::string sndbuf;
		while((value = db_nextrec(*set_it, key)) != NULL){
			std::cout << "key = " << key << "value = " << value << std::endl;	
			sndbuf = "*5\r\n $5\r\nstore\r\n ";
			EncodeString(key, sndbuf);
			EncodeString(value, sndbuf);
			EncodeInt(flag, sndbuf);
			EncodeInt(cli->GetRequestPtr()->GetId(), sndbuf);
			
			::write(server->fds_[cli->GetBev()], sndbuf.c_str(), sndbuf.size());
			sndbuf.clear();
			// for synchronization
			::read(server->fds_[cli->GetBev()], retbuf, sizeof(retbuf));
		}
	}	       
 	return 0;
}


int Dbm::slave_open(Client *cli)
{
        char retbuf[256];

	DbmServer* server = cli->GetServer();
	std::string sndbuf;

	// first open slave database
	for(auto set_it = server->dbs_.begin(); set_it != server->dbs_.end(); set_it++){
		sndbuf = "*3\r\n $4\r\nopen\r\n ";
		EncodeString(*set_it, sndbuf);
		EncodeInt(cli->GetRequestPtr()->GetId(), sndbuf);
		
		::write(server->fds_[cli->GetBev()], sndbuf.c_str(), sndbuf.size());
		sndbuf.clear();
		int retlen = ::read(server->fds_[cli->GetBev()], retbuf, sizeof(retbuf));
	}

	return 0;
}

