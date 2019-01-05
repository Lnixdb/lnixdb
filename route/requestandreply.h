#ifndef  REQUEST_AND_REPLY_H
#define	 REQUEST_AND_REPLY_H

#include <string>
#include <vector>
#include "dbm.h"

/* Parse the client message to extract the fields and 
 * put them into the relevant fields of the class.
 * protocol format :
 * 		*2\r\n $7\r\ndb_open\r\n $13\r\n/root/home/database\r\n
 * 		*2\r\n : 2 is the number of strings that follow
 *		$7\r\ndb_open\r\n : 7 is the length of the string
 *		also,$13 is the length of the string.
*/

class RequestAndReply{
 public :
	RequestAndReply() : msg_(NULL) { 
	}
	RequestAndReply(const char *input) : msg_(input) { 
	}
	const char* GetMsgPtr() { return msg_; }
	int GetMsgLen() { return prolen_; }
	void SetMsg(const char *msg) { msg_ = msg; }
	int GetProcessedLen() { return prolen_; }
	int ProcessMsg();
	const std::string& GetRequestReply() const {
		return request_reply_ ;
	}
	void SetRequestReply(const std::string reply){
		request_reply_ = reply;
	}
	const std::string& GetFunctionName() const  { return function_name_;}
	int GetId() { return db_id_; }
	const std::vector<std::string>& GetFunctionArgs() const { return function_args_; }
	long DbHash();
	void Clear();
 private:
	const char *msg_;
	int prolen_;
	int arg_num_;
	/*
 	 * The database db_id_ sent by the client to identify different databases
	 * opened by the same client.
	 * Always in the last field of the message protocol.
 	*/
	int db_id_;	 		
	std::string function_name_;      //The name of the function in the protocol,eg: db_open
	std::string request_reply_;       //Reply to the client message
	std::vector<std::string> function_args_; //Store the parameters sent by the client
};

#endif
