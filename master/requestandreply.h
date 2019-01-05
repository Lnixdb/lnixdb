#ifndef  REQUEST_AND_REPLY_H
#define	 REQUEST_AND_REPLY_H

#include <string>
#include <vector>
#include "dbm.h"
#include "client.h"

class Client;
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
	RequestAndReply() : msg_(NULL) { }

	RequestAndReply(const char *input) : msg_(input) { }
	
 	void SetMsg(const char *msg) { msg_ = msg; }
	
	const char* GetMsgPtr() { return msg_; }
	
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
	
	void Clear();
	
	Client* GetClient() { return cli_; }
	
	void SetClient(Client* cli) { cli_ = cli; }

 private:
	// request msg
	const char *msg_;

	// Length of message processed
	int prolen_;
	int arg_num_;

	// Go back to the client object
	Client* cli_;

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
