#pragma once

#include "targetver.h"



// TODO:  在此处引用程序需要的其他头文件
#include "iostream"
#include "map"
#include "windows.h"
#include "vector"
#include <sstream>
#include <string>

using namespace std;
#pragma comment(lib, "Wsock32.lib")

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息


typedef struct HttpResponse{

	int code;
	string recvStr;
	string scheme;
	int httpCode;
	string httpCodeDescription;
	map<string, string> header;
	map<string, string> cookie;

} 
/*A HttpResponse Structure , contains several attributes after you made a valid http request.
- code :
		Code is usually used to figure out how many bytes received from remote server , or a negative number to indicate the error that occurred when trying to request.
		Currently , 
		-1 - Address NULL
		-2 - Initialize the WSADATA Failed
		-3 - Connect to remote host failed

- recvStr :
		This field means the real content received from remote server.

- scheme :
		Which scheme is used for , currently this field could only be "HTTP"

- httpCode :
		Http Status Code , from remote server , which indicates the request status. For example , 200 means OK , and 3xx means redirection , 4xx means request error , and 5xx means server runtime error.

- httpCodeDescription
		A simple description for httpCode , received from server

- header :
		The response header received from remote server , no cookie here.

- cookie :
		The cookie received from remote server.

*/
HTTPRESPONSE;

///
///	The Following Functions Implemented In "http.cpp"
///

map<string, string> getNewMap();
HTTPRESPONSE sendPost(const char * url, map<string, string> cookie, map<string, string> header, const char * data);
HTTPRESPONSE sendGet(const char * url, map<string, string> cookie, map<string, string> header);
HTTPRESPONSE sendNet(const char * addr, int port, int isGet, const char* target, map<string, string>& headers, map<string, string>& cookie, const char * data);
void split(const string& str, const string& delim, vector<string>& parts);
