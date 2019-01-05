#include "dbmserver.h"
#include "client.h"

/* Get the client object
 * If not, assign one
 */
Client* DbmServer::LookupClient(bufferevent *bev){
	Client *cli;
	if(clients_.find(bev) == clients_.end()){
		cli = new Client(bev);
		this->vec_clients_.push_back(cli);
		cli->SetServerPtr(this);
		clients_.insert({bev, cli});
	}
	return clients_[bev];
}

void DbmServer::ForwardingMsg(RequestAndReply *rap){
	
	if(!rap->GetFunctionName().compare("open")){
		for(size_t i=0; i<nodes_.size(); i++){
			nodes_[i]->WriteMsg(rap);
		}
	}
        else if(!rap->GetFunctionName().compare("store")){
		long hashval = rap->DbHash(); 
		//int idx = hashval % nodes_.size();
		size_t idx = 0;
		if(!nodes_.empty())
			nodes_[idx]->WriteMsg(rap);
        }
	else if(!rap->GetFunctionName().compare("fetch")){
		long hashval = rap->DbHash(); 
		//size_t idx = hashval % nodes_.size();
		size_t idx = 0;

		if(!nodes_.empty())
			nodes_[idx]->WriteMsg(rap);
	}
        else if(!rap->GetFunctionName().compare("delet")){
		long hashval = rap->DbHash(); 
		//int idx = hashval % nodes_.size();
		size_t idx = 0;

		if(!nodes_.empty())
			nodes_[idx]->WriteMsg(rap);
	}
        else if(!rap->GetFunctionName().compare("close")){
		for(int i=0; i<nodes_.size(); i++){
			nodes_[i]->WriteMsg(rap);
		}
	}
        else if(!rap->GetFunctionName().compare("rewind")){
	}
        else if(!rap->GetFunctionName().compare("nextrec")){
	}
}
