#include <vector>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <event.h>
#include <event2/bufferevent.h>
#include <unistd.h>
#include "requestandreply.h"
#include "dbm.h"
#include "db.h"
#include "dbmserver.h"
#include "coding.h"

void accept_cb(int fd, short events, void* arg);
void socket_read_cb(bufferevent* bev, void* arg);
void event_cb(struct bufferevent *bev, short event, void *arg);
int tcp_server_init(int port, int listen_num);

void
timeout_cb(evutil_socket_t fd, short event, void *arg);
DbmServer *server = new DbmServer(4);

int main(int argc, char** argv)
{
   
    int listener = tcp_server_init(6479, 10);
    if( listener == -1 )
    {
        perror(" tcp_server_init error ");
        return -1;
    }
 
    struct event_base* base = event_base_new();

    // add time event
    struct event timeout;
    struct timeval tv;
    event_assign(&timeout, base, -1, EV_PERSIST, timeout_cb, (void*) &timeout);
    evutil_timerclear(&tv);
    tv.tv_sec = 2;
    event_add(&timeout, &tv);

    //添加监听客户端请求连接事件
    struct event* ev_listen = event_new(base, listener, EV_READ | EV_PERSIST,
                                        accept_cb, base);
    event_add(ev_listen, NULL); 
    event_base_dispatch(base);
    event_base_free(base);
 
    return 0;
}
 
void accept_cb(int fd, short events, void* arg)
{
    evutil_socket_t sockfd;
 
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
 
    sockfd = ::accept(fd, (struct sockaddr*)&client, &len );
    evutil_make_socket_nonblocking(sockfd);
 
    printf("accept a client %d\n", sockfd);
 
    struct event_base* base = (event_base*)arg;
 
    bufferevent* bev = bufferevent_socket_new(base, sockfd, BEV_OPT_CLOSE_ON_FREE);
    server->fds_.insert({bev, sockfd});
    server->remote_addr_.insert({sockfd, client});
    bufferevent_setcb(bev, socket_read_cb, NULL, event_cb, arg);
 
    bufferevent_enable(bev, EV_READ | EV_ET);
}
 
void socket_read_cb(bufferevent* bev, void* arg)
{
	server->GetThreadPoolPtr()->AddQueue(bev);	
}
 
void event_cb(struct bufferevent *bev, short event, void *arg)
{
 
    if (event & BEV_EVENT_EOF)
        printf("connection closed\n");
    else if (event & BEV_EVENT_ERROR)
        printf("some other error\n");
 
    //这将自动close套接字和free读写缓冲区
    bufferevent_free(bev);
    //server->clients_.erase(bev);
}
 
typedef struct sockaddr SA;
int tcp_server_init(int port, int listen_num)
{
    int errno_save;
    evutil_socket_t listener;
 
    listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if( listener == -1 )
        return -1;
 
    //允许多次绑定同一个地址。要用在socket和bind之间
    evutil_make_listen_socket_reuseable(listener);
 
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(port);
 
    if( ::bind(listener, (SA*)&sin, sizeof(sin)) < 0 )
        goto error;
 
    if( ::listen(listener, listen_num) < 0)
        goto error;
  
    //跨平台统一接口，将套接字设置为非阻塞状态
    evutil_make_socket_nonblocking(listener);
 
    return listener;
 
    error:
        errno_save = errno;
        evutil_closesocket(listener);
        errno = errno_save;
 
        return -1;
}

void
timeout_cb(evutil_socket_t fd, short event, void *arg)
{
        struct event *timeout = (struct event*)arg;
	
        printf("timeout_cb called at %d: %.3f seconds elapsed.\n");

	size_t	slave_num = 0;
	std::string prefix("$");
	std::string addr_str;
	std::string all_addr;
	for(auto vec_it = server->clientptrs_.begin(); 
		 vec_it != server->clientptrs_.end(); vec_it++){
		if((*vec_it)->IsSlave()){
			int fd = server->fds_[(*vec_it)->GetBev()];		
			struct sockaddr_in& slaveaddr = server->remote_addr_[fd];
			addr_str += prefix;
			std::string tmp(inet_ntoa(slaveaddr.sin_addr));
			tmp += ':';
			tmp += std::to_string(slaveaddr.sin_port);
			addr_str += std::to_string(tmp.size());
			addr_str += "\r\n";
			addr_str += tmp;
			addr_str += "\r\n ";

			all_addr += addr_str;
			addr_str.clear();
			slave_num += 1;
		}
	}	
	int arg_num = slave_num + 2;
	std::string command("*");
	command += std::to_string(arg_num);
	command += "\r\n ";
	EncodeString("masterinfo", command);	
	command += all_addr;
 
	// encode id field
	EncodeInt(1, command);	 
	// send command msg
	if(!all_addr.empty()){
		for(auto vec_it = server->clientptrs_.begin(); 
			 vec_it != server->clientptrs_.end(); vec_it++){
			// send to route
			if(!(*vec_it)->IsSlave()){
				int fd = server->fds_[(*vec_it)->GetBev()];		

				std::cout << command << "= " <<addr_str.size() << std::endl;
				int len = ::write(fd, command.c_str(), command.size());
				if(len == -1){
					if(errno == EBADF)
						std::cout << "write EBADF" << std::endl;
				}
				char retbuf[124];	//for synchronous
				len = ::read(fd, retbuf, sizeof(retbuf));
				if(len == -1){
					if(errno == EBADF)
						std::cout << "read EBADF" << std::endl;
				}	
			}
		}
	}
        struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_sec = 2;
        event_add(timeout, &tv);
}

