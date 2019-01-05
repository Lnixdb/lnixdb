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
	if(len == 0){
		std::cout << "client closed connect" << std::endl;
		return 0;
	}
	Client *cli = server->LookupClient(bev);
	assert(cli != NULL);
	int processedlen = 0;
	while(len > 0){
		cli->GetRequestPtr()->SetMsg(msgp);
		processedlen = cli->GetRequestPtr()->ProcessMsg();
		
		std::cout << cli->GetRequestPtr()->GetFunctionName() << std::endl;
		for(auto it : cli->GetRequestPtr()->GetFunctionArgs())
			std::cout << it << std::endl;

		if((cli->GetRequestPtr()->GetFunctionName() == "masterinfo") || 
		   (cli->GetRequestPtr()->GetFunctionName() == "PONG") ){	
			int dbid = cli->GetRequestPtr()->GetId();
			cli->GetDbmPtr(dbid)->ExecuteCommand(cli);
		}else{
			cli->GetServerPtr()->ForwardingMsg(cli->GetRequestPtr());
		}
		len -= processedlen;
		msgp += processedlen;
		
		const std::string &reply = cli->GetRequestPtr()->GetRequestReply(); 
		if(!reply.empty())	
			bufferevent_write(bev, reply.c_str(), reply.size());
		
		cli->GetRequestPtr()->Clear();
	}
	return 0;
}
