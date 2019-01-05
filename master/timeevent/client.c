#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>
 
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
 
#include<event.h>
#include<event2/util.h> 
 
static void
timeout_cb(evutil_socket_t fd, short event, void *arg);

int main(int argc, char** argv)
{   
    struct event_base* base = event_base_new();
    struct event timeout;
    struct timeval tv; 
    event_assign(&timeout, base, -1, EV_PERSIST, timeout_cb, (void*) &timeout);
    evutil_timerclear(&tv);
    tv.tv_sec = 2;
    event_add(&timeout, &tv);

    event_base_dispatch(base);
 
    printf("finished \n");
    return 0;
}

static void
timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event *timeout = (struct event*)arg;

	printf("timeout_cb called at %d: %.3f seconds elapsed.\n");

	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = 2;
	event_add(timeout, &tv);
}

