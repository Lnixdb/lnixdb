#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include<event.h>
#include<event2/bufferevent.h>
#include<event2/buffer.h>
#include<event2/util.h>

#include "connect.h"

Connection::Connection(const char *ip, int port) : ip_(ip), port_(port)
{
        sockfd_ = tcp_connect_server(ip, port);
}

int Connection::tcp_connect_server(const char* server_ip, int port)
{
    int sockfd, status, save_errno;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr) );

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    status = inet_aton(server_ip, &server_addr.sin_addr);

    if( status == 0 ) //the server_ip is not valid value
    {
        errno = EINVAL;
        return -1;
    }

    sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
    if( sockfd == -1 )
        return sockfd;


    status = ::connect(sockfd, (SA*)&server_addr, sizeof(server_addr) );

    if( status == -1 )
    {
        save_errno = errno;
        ::close(sockfd);
        errno = save_errno; //the close may be error
        return -1;
    }
   // evutil_make_socket_nonblocking(sockfd);
        return sockfd;
}

void Connection::WriteMsg(RequestAndReply *rap){
	std::cout << rap->GetMsgLen() << " debug here" << std::endl;
	::write(sockfd_, rap->GetMsgPtr(), rap->GetMsgLen());	
	char msg[1024];
	int len = ::read(sockfd_, msg, sizeof(msg));
	std::string reply(msg, len);
	rap->SetRequestReply(reply);	
}



