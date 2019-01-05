#ifndef _CONNECTION_H
#define _CONNECTION_H

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>
#include <string>
#include "requestandreply.h"

class Connection{
 typedef struct sockaddr SA;
 public :
        Connection(const char *ip, int port);
        int GetSockfd() { return sockfd_; }
        int tcp_connect_server(const char *server_ip, int port = 9999);
	void WriteMsg(RequestAndReply *rap);
 private:
        int     port_;
        int     sockfd_;
        const std::string ip_;
};

#endif
