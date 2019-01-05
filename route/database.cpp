#include <vector>
#include <string>
#include <string.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>

#include <event.h>
#include <event2/bufferevent.h>

#include "requestandreply.h"
#include "dbm.h"
#include "db.h"
#include "dbmserver.h"
#include "connect.h"

void accept_cb(int fd, short events, void* arg);
void socket_read_cb(bufferevent* bev, void* arg);
void event_cb(struct bufferevent *bev, short event, void *arg);
int tcp_server_init(int port, int listen_num);

void
ping_timeout_cb(evutil_socket_t fd, short event, void *arg);

void
timeout_cb(evutil_socket_t fd, short event, void *arg);

DbmServer *server = new DbmServer(4);
int main(int argc, char** argv)
{
   
    int listener = tcp_server_init(9999, 10);
    Connection *conn = new Connection("192.168.1.232", 6479);
    server->nodes_.push_back(conn);
    /* 
    conn = new Connection("192.168.1.233", 6479);
    server->nodes_.push_back(conn); 
    */
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

    // add time event
    struct event ping_timeout;
    struct timeval ptv;
    event_assign(&ping_timeout, base, -1, EV_PERSIST, ping_timeout_cb, (void*) &ping_timeout);
    evutil_timerclear(&ptv);
    ptv.tv_sec = 2;
    event_add(&ping_timeout, &ptv);

        //添加监听客户端请求连接事件
    struct event* ev_listen = event_new(base, listener, EV_READ | EV_PERSIST,
                                        accept_cb, base);
    event_add(ev_listen, NULL); 

    // master read event 
    bufferevent* bev = bufferevent_socket_new(base, conn->GetSockfd(), BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, socket_read_cb, NULL, event_cb, base);
    bufferevent_enable(bev, EV_READ | EV_ET);

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
    //evutil_make_socket_nonblocking(sockfd);
 
    printf("accept a client %d\n", sockfd);
 
    struct event_base* base = (event_base*)arg;
 
    bufferevent* bev = bufferevent_socket_new(base, sockfd, BEV_OPT_CLOSE_ON_FREE);

    server->fds_.insert({bev, sockfd});

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
    //evutil_make_socket_nonblocking(listener);
 
    return listener;
 
    error:
        errno_save = errno;
        evutil_closesocket(listener);
        errno = errno_save;
 
        return -1;
}
void EncodeInt(int val, std::string &ret){
        std::string str = std::to_string(val);
        ret.append("$");
        ret.append(std::to_string(str.size()));
        ret.append("\r\n");
        ret.append(str);
        ret.append("\r\n ");
}

void
timeout_cb(evutil_socket_t fd, short event, void *arg)
{
        struct event *timeout = (struct event*)arg;
	
        printf("timeout_cb called at %d: %.3f seconds elapsed.\n");
	std::string command("*2\r\n ");
	for(auto vec_it = server->vec_clients_.begin(); 
		vec_it != server->vec_clients_.end(); vec_it++){
		if((*vec_it)->IsMaster()){
			// inc ping time
			(*vec_it)->IncTimer();		
		}
	}
	for(auto vec_it = server->nodes_.begin(); 
		 vec_it != server->nodes_.end(); vec_it++){
			command += "$4\r\nPING\r\n ";
			EncodeInt(1, command);
			int fd = (*vec_it)->GetSockfd();
			::write(fd, command.c_str(), command.size());
			command.resize(5);
	}
	struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_sec = 2;
        event_add(timeout, &tv);
}

void
ping_timeout_cb(evutil_socket_t fd, short event, void *arg)
{
        struct event *timeout = (struct event*)arg;
	
        printf("ping_timeout_cb......\n");

	for(auto vec_it = server->vec_clients_.begin(); 
		 vec_it != server->vec_clients_.end(); vec_it++){
		if((*vec_it)->IsMaster()){
			if((*vec_it)->GetTimer() > 2){		
				std::cout << "ping timeout......" << std::endl;
				server->nodes_.clear();
				if(!(*vec_it)->GetSlaves().empty()){
					const char* ip = (*vec_it)->GetSlaves()[0].ip_.c_str();  
					short port = (*vec_it)->GetSlaves()[0].port_;  
					Connection *conn = new Connection(ip, 6479);
					server->nodes_.push_back(conn);
				}
			}
		}
	}
	struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_sec = 2;
        event_add(timeout, &tv);
}


