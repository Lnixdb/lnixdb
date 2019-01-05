#include <stdlib.h>
#include <cassert>
#include <unistd.h>
#include "client.h"
#include "dbmserver.h"

extern DbmServer *server;
int ThreadPoolImpl::ProcessFdEvent(bufferevent *bev){
	if(bev == NULL)
		return 0;

	char	msg[4096];
	char	*msgp = msg;
	int	len = bufferevent_read(bev, msg, sizeof(msg));
	
	std::cout << "len = " << len << std::endl;
	if(len == 0){
		std::cout << "client closed connect" << std::endl;
		return 0;
	}

	// get client 
	Client *cli = server->LookupClient(bev);
	assert(cli != NULL);
	int processedlen = 0;
	while(len > 0){
		cli->GetRequestPtr()->SetMsg(msgp);
		processedlen = cli->GetRequestPtr()->ProcessMsg();
		// for debug
		//std::cout << cli->GetRequestPtr()->GetFunctionName() << std::endl;
		//for(auto it : cli->GetRequestPtr()->GetFunctionArgs())
		//	std::cout << it << std::endl;
		
		// get database id
		int dbid = cli->GetRequestPtr()->GetId();

		// execute user command
		cli->GetDbmPtr(dbid)->ExecuteCommand(cli);
		len -= processedlen;
		msgp += processedlen;
		
		const std::string &reply = cli->GetRequestPtr()->GetRequestReply(); 
		if(!reply.empty())	
			bufferevent_write(bev, reply.c_str(), reply.size());
		// clear message, for process next command
		cli->GetRequestPtr()->Clear();
	}
	return 0;
}
