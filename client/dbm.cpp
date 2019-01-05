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

class Connection{	
 typedef struct sockaddr SA;
 friend class Dbm;
 public :
	Connection(const char *ip, int port);
	int GetSockfd() { return sockfd_; }
	int tcp_connect_server(const char *server_ip, int port = 9999);
 private:
	int	port_;
	int	sockfd_;
	const std::string ip_;
};


class Dbm{
 public :	
	static int distributor_id_;
	Dbm(Connection* conn) : conn_(conn), db_id_(distributor_id_) { distributor_id_++; }
	int GetId() { return db_id_; }
	int db_open(const std::string& path);
	std::string db_fetch(const std::string& key);
	int db_store(const std::string& key, const std::string& value, int flag);
	int db_delete(const std::string& key);
	int db_nextrec(const std::string& key);	
	int db_rewind();	
	int db_close();
 private:
	int db_id_;
	Connection *conn_;
};
int Dbm::distributor_id_ = 0;

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

int Dbm::db_open(const std::string& path)
{
	std::string sndbuf("*3\r\n $4\r\nopen\r\n ");
	
	EncodeString(path, sndbuf);
	EncodeInt(db_id_, sndbuf);

	::write(conn_->GetSockfd(), sndbuf.c_str(), sndbuf.size());	
	char retbuf[1024];
	int retlen = ::read(conn_->GetSockfd(), retbuf, sizeof(retbuf));
	retbuf[retlen] = '\0';
	std::cout << retbuf << std::endl;
	return 0;
}

int Dbm::db_close()
{
	std::string sndbuf("*2\r\n $5\r\nclose\r\n ");
	EncodeInt(db_id_, sndbuf);
	
	::write(conn_->GetSockfd(), sndbuf.c_str(), sndbuf.size());	
	char retbuf[1024];
	int retlen = ::read(conn_->GetSockfd(), retbuf, sizeof(retbuf));
	retbuf[retlen] = '\0';
	std::cout << retbuf << std::endl;
	return 0;
}


int Dbm::db_rewind()
{
	std::string sndbuf("*2\r\n $6\r\nrewind\r\n ");
	EncodeInt(db_id_, sndbuf);

	::write(conn_->GetSockfd(), sndbuf.c_str(), sndbuf.size());	
	char retbuf[1024];
	int retlen = ::read(conn_->GetSockfd(), retbuf, sizeof(retbuf));
	return 0;
}


std::string Dbm::db_fetch(const std::string& key)
{
	std::string sndbuf("*3\r\n $5\r\nfetch\r\n ");
	EncodeString(key, sndbuf);
	EncodeInt(db_id_, sndbuf);
	
	::write(conn_->GetSockfd(), sndbuf.c_str(), sndbuf.size());	
	char retbuf[1024];
	int retlen = ::read(conn_->GetSockfd(), retbuf, sizeof(retbuf));
	
	return std::string(retbuf, retlen);
}

int Dbm::db_nextrec(const std::string& key)
{
	std::string sndbuf("*3\r\n $7\r\nnextrec\r\n ");
	
	EncodeString(key, sndbuf);
	EncodeInt(db_id_, sndbuf);

	::write(conn_->GetSockfd(), sndbuf.c_str(), sndbuf.size());	
	char retbuf[1024];
	int retlen = ::read(conn_->GetSockfd(), retbuf, sizeof(retbuf));
	
	return 0;
}


int Dbm::db_delete(const std::string& key)
{
	std::string sndbuf("*3\r\n $5\r\ndelet\r\n ");
	EncodeString(key, sndbuf);
	EncodeInt(db_id_, sndbuf);
	
	::write(conn_->GetSockfd(), sndbuf.c_str(), sndbuf.size());	
	char retbuf[1024];
	int retlen = ::read(conn_->GetSockfd(), retbuf, sizeof(retbuf));
	retbuf[retlen] = '\0';
	std::cout << retbuf << std::endl;
	return 0;
}


int Dbm::db_store(const std::string& key, const std::string& value, int flag)
{
	std::string sndbuf("*5\r\n $5\r\nstore\r\n ");
	EncodeString(key, sndbuf);
	EncodeString(value, sndbuf);
	EncodeInt(flag, sndbuf);
	EncodeInt(db_id_, sndbuf);
	
	::write(conn_->GetSockfd(), sndbuf.c_str(), sndbuf.size());	
	char retbuf[1024];
	int retlen = ::read(conn_->GetSockfd(), retbuf, sizeof(retbuf));
	retbuf[retlen] = '\0';
	std::cout << retbuf << std::endl;
	
	return 0;
}


int main(int argc, char** argv)
{
	Connection *conn = new Connection("192.168.1.232", 9999);
	Dbm *dbm = new Dbm(conn);
	
	dbm->db_open("/root/mydir/apuev3/database/version/dbdir/dbfile");
	dbm->db_store("hello", "world", 1);

	dbm->db_store("key", "value", 1);
	
	std::string retstr = dbm->db_fetch("key");
	std::cout << retstr << std::endl;
	
	retstr = dbm->db_fetch("key");
	std::cout << retstr << std::endl;
	
	retstr = dbm->db_fetch("hello");
	std::cout << retstr << std::endl;

	//dbm->db_close();
	
}

