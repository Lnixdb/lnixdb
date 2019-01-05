#include "dbmserver.h"
#include "client.h"

/* Get the client object
 * If not, assign one
 */
Client* DbmServer::LookupClient(bufferevent *bev){
	Client *cli;
	if(clients_.find(bev) == clients_.end()){
		cli = new Client(bev);
		cli->SetServer(this);
		cli->GetServer()->clientptrs_.push_back(cli);
		clients_.insert({bev, cli});
	}
	return clients_[bev];
}
