#include <iostream>
#include <string>
#include <vector>
#include <string.h>	//for strchr

#include "requestandreply.h"

int RequestAndReply::ProcessMsg()
{
	const char *tmpptr = msg_;

	if(msg_[0] != '*')
		return -1;
	char *ptr = strchr((char*)msg_, '\n');
	if(ptr == NULL)
		return -1;
	if(*(ptr-1) != '\r')
		return -1;
	if((msg_+1) == (ptr-1))
		return -1;

	std::string num_str = std::string((msg_+1), (ptr-2-msg_));
	std::string::size_type sz;
	int len = std::stoi(num_str, &sz);

	for(int i=0; i<len; i++){
		ptr++;
		if(ptr == '\0')
			return -1;
		/* skip space */
		while(*ptr == ' ') ptr++;
		msg_ = ptr;
		if(msg_[0] != '$')
			return -1;
		ptr = strchr((char*)msg_, '\n');
		if(ptr == NULL)
			return -1;
		if(*(ptr-1) != '\r')
			return -1;
		if((msg_+1) == (ptr-1))
			return -1;
		num_str = std::string((msg_+1), (ptr-2-msg_));
		std::string::size_type sz;
		int char_len = std::stoi(num_str, &sz);
		ptr++;

		if((*(ptr+char_len) != '\r') && (*(ptr+char_len+1) != '\n'))
			return -1;
		std::string str(ptr, char_len);
		if(i == 0)
			function_name_ = str;	//Gets the name of the method to execute,
						//Always in the first field of the message protocol.
		else if(i == (len-1))
			db_id_ = stoi(str);     // database id,Always in the last field of the message protocol
		else
			function_args_.push_back(str); 
		ptr = ptr + char_len + 1;
	}
	prolen_ =  ptr - tmpptr + 1 + 1;
	msg_ = tmpptr;
	return prolen_;
}

void RequestAndReply::Clear(){
	function_name_.clear();
	function_args_.clear();
	request_reply_.clear();
}

