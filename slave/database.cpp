#include <vector>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <event2/bufferevent.h>

#include "requestandreply.h"
#include "dbm.h"
#include "db.h"
#include "dbmserver.h"

int tcp_connect_server(const char* server_ip, int port);
void socket_read_cb(bufferevent* bev, void* arg);
void event_cb(struct bufferevent *bev, short event, void *arg);
void send_slaveof(int fd);

void EncodeString(const std::string &str, std::string &ret);
void EncodeInt(int val, std::string &ret);

DbmServer *server = new DbmServer(4);

int main(int argc, char** argv)
{
    if(argc < 3)
	return 0;
    int connfd = tcp_connect_server(argv[1], atoi(argv[2]));
    
    evutil_make_socket_nonblocking(connfd);
    struct event_base* base = event_base_new();
   
    bufferevent* bev = bufferevent_socket_new(base, connfd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, socket_read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_ET);
    // Send "slave" commands to master, copy master 
    send_slaveof(connfd);

    event_base_dispatch(base);
    event_base_free(base);
 
    return 0;
}

void send_slaveof(int fd){
	std::string send("*2\r\n $7\r\nslaveof\r\n ");
	EncodeInt(1, send);
	::write(fd, send.c_str(), send.size());
}

void socket_read_cb(bufferevent* bev, void* arg)
{
	server->GetThreadPoolPtr()->AddQueue(bev);	
}
 
void event_cb(struct bufferevent *bev, short event, void *arg)
{
 
    if (event & BEV_EVENT_EOF){
	// disconnec client
	delete (server->clients_[bev]);
	//remove it
	server->clients_.erase(bev);

        printf("connection closed\n");
    }
    else if (event & BEV_EVENT_ERROR)
        printf("some other error\n");
 
    //这将自动close套接字和free读写缓冲区
    bufferevent_free(bev);
    //server->clients_.erase(bev);
}
 
typedef struct sockaddr SA;
int tcp_connect_server(const char* server_ip, int port)
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

    //evutil_make_socket_nonblocking(sockfd);

    return sockfd;
}

void EncodeString(const std::string &str, std::string &ret){
        int strlen = str.size();
        ret.append("$");
        ret.append(std::to_string(strlen));
        ret.append("\r\n");
        ret.append(str);
        ret.append("\r\n ");
}
void EncodeInt(int val, std::string &ret){
        std::string str = std::to_string(val);
        ret.append("$");
        ret.append(std::to_string(str.size()));
        ret.append("\r\n");
        ret.append(str);
        ret.append("\r\n ");
}

