#include "dbmserver.h"
#include "client.h"

/* Get the client object
 * If not, assign one
 */
Client* DbmServer::LookupClient(bufferevent *bev){
	Client *cli;
	if(clients_.find(bev) == clients_.end()){
		cli = new Client(bev);
		clients_.insert({bev, cli});
	}
	return clients_[bev];
}
