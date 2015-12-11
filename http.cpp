/*
This cpp file is used to implement a simple HTTP protocol using C/C++ , 
POST and GET method available through this code.

In order to send a net packet like a browser , you can use "sendGet" for sending a GET request
or "sendPost" for sending a POST request. And after request you will receive a HTTPRESPONSE struct
which contains the status code and reply from remote server.

For more information about HTTPRESPONSE , check it on stdafx.h

HTTPS is currently not supported.

===================================================================

Copyright (C) 2013-2015 Sunflyer

This source code is provided 'as-is', without any express or implied
warranty. In no event will the author be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this source code must not be misrepresented; you must not
claim that you wrote the original source code. If you use this source code
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original source code.

3. This notice may not be removed or altered from any source distribution.

Sunflyer : sunflyer@qq.com

*/
#include "stdafx.h"
#pragma warning(disable:4996)

#define BUFFER_LEN 6000

map<string, string> getBasicHeader(string host, int port){
	map<string, string> newMap;
	if (port > 0 && port != 80){
		host.append(":");
		char cport[6];
		itoa(port, cport, 10);
		host.append(cport);
	}
	newMap.insert(pair<string, string>("Host", host));
	newMap.insert(pair<string, string>("Cache-Control", "max-age=0"));
	newMap.insert(pair<string, string>("DNT", "1"));

	return newMap;
}

map<string, string> getNewMap(){
	map<string, string> newMap;
	return newMap;
}

void split(const string& str, const string& delim, vector<string>& parts) {
	size_t start, end = 0;
	while (end < str.size()) {
		start = end;
		while (start < str.size() && (delim.find(str[start]) != string::npos)) {
			start++;  // skip initial whitespace
		}
		end = start;
		while (end < str.size() && (delim.find(str[end]) == string::npos)) {
			end++; // skip to end of word
		}
		if (end - start != 0) {  // just ignore zero-length strings.
			parts.push_back(string(str, start, end - start));
		}
	}
}


string dealChunk(string chunked){
	string real;
	if (chunked.length() > 0){
		int len = 0, path = 0, pathCrLf = 0, flag = 0;
		string tmp;
		string proc = chunked;
		while (proc.length() > 0){
			len = path = flag = 0;
			pathCrLf = proc.find("\r\n");
			if (!pathCrLf){
				pathCrLf = proc.find("\r\n", 2);
				path += 2;
				flag = 1;
			}
			if (pathCrLf > 0 && pathCrLf < 10){
				tmp = proc.substr(path, pathCrLf);
				sscanf(tmp.c_str(), "%x", &len);

				path += tmp.length();
				if (!flag){
					path += 2;
				}
				// FA0 \r\n 
				tmp = proc.substr(path, len);
				path += len;
				real.append(tmp);

				if (path >= proc.length())
					break;
				proc = proc.substr(path);
			}
			else{
				real.append(proc);
				break;
			}
		}
	}
	return real.substr(0, real.length() - 2);
}

void processResponse(HTTPRESPONSE * target){
	if (target != NULL){

		target->code = target->recvStr.length();
		target->httpCode = 0;

		if (target->code == 0){
			target->code = -1;
			return;
		}


		int path = target->recvStr.find("\r\n\r\n");
		string header = target->recvStr.substr(0, path);
		string content = target->recvStr.substr(path + 4);

		vector<string> head;
		split(header, "\r\n", head);

		//process header , the first line should be SCHEME and STATUS CODE , use space to split
		vector<string> temp;
		split(head[0], " ", temp);

		target->scheme = temp[0];
		target->httpCode = atoi(temp[1].c_str());
		target->httpCodeDescription = temp[2];

		int tmpnum = -1;
		string tmpstr;
		for (int i = 1; i < head.size(); i++){
			tmpnum = head[i].find(": ");
			if (tmpnum >= 0){
				tmpstr = head[i].substr(0, tmpnum);
				if (!strcmp(tmpstr.c_str(), "Set-Cookie")){
					tmpstr = head[i].substr(tmpnum + 2);
					temp.clear();
					split(tmpstr, "; ", temp);
					int cook = -1;
					for (tmpnum = 0; tmpnum < temp.size(); tmpnum++){
						cook = temp[tmpnum].find("=");
						if (cook > 0){
							tmpstr = temp[tmpnum].substr(0, cook);
							if (cook + 1 >= temp[tmpnum].length()){
								target->cookie.insert(pair<string, string>(tmpstr, ""));
							}
							else{
								target->cookie.insert(pair<string, string>(tmpstr, temp[tmpnum].substr(cook + 1)));
							}
						}
					}
				}
				else{
					target->header.insert(pair<string, string>(tmpstr, tmpnum + 2 >= head[i].length() ? "" : head[i].substr(tmpnum + 2)));
				}
			}
		}

		map<string, string>::iterator headerIt = target->header.find("Transfer-Encoding");
		if (headerIt != target->header.end() && !headerIt->second.compare("chunked")){

			target->recvStr = dealChunk(content);
		}
		else{
			target->recvStr = content;
		}

	}
}


/*
Send a http request to target host.
@Author CrazyChen@CQUT
@Date Dec 8 , 2015

@param addr Target Host Name , only ip or hostname allowed . e.g for http://192.168.1.1/myip.php the Host should be 192.168.1.1
@param port Host port for sending request. 
@param isGet whether the request method is GET ( if value set to 1 ) or POST ( if value set to 0 )
@param target what's the file you want to request. e.g for http://192.168.1.1/myip.php  the target should be /myip.php
@param headers The HTTP Headers Group , or NULL if no headers should be followed. Headers should be preprocessed like "Key: Value"
@param cookie The cookie you want to be sent,
@param data The Data You want to be sent , only if you set isGet=0 (POST Method)
@param replyData The Char Group For Storaging the reply data received from remote.

@return A HTTPRESPONSE Structure


**/
HTTPRESPONSE sendNet(const char * addr, int port, int isGet, const char* target, map<string, string>& headers, map<string, string>& cookie, const char * data){
	HTTPRESPONSE response;

	if (!addr){
		response.code = -1;
		return response;
	}

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData)){
		response.code = -2;
		return response;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct hostent *host = gethostbyname(addr);

	SOCKADDR_IN SockAddr;
	SockAddr.sin_port = htons(port >= 0 ? port : 80);
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

	if (connect(sock, (SOCKADDR*)(&SockAddr), sizeof(SockAddr))){
		response.code = -3;
		return response;
	}

	string toSend;

	toSend.append(isGet ? "GET " : "POST ").append((target == NULL || strlen(target) == 0) ? "/" : target).append(" HTTP/1.1\r\n");
	string tmpHost = addr;
	map<string, string> basicHeader = getBasicHeader(tmpHost, port);
	if (!basicHeader.empty()){
		map<string, string>::iterator headerIt = basicHeader.begin();
		for (; headerIt != basicHeader.end(); ++headerIt){
			toSend.append(headerIt->first).append(": ").append(headerIt->second).append("\r\n");
		}
	}
	if (!headers.empty()){
		map<string, string>::iterator headerIt = headers.begin();
		for (; headerIt != headers.end(); ++headerIt){
			toSend.append(headerIt->first).append(": ").append(headerIt->second).append("\r\n");
		}
	}


	//Set Content-Length
	if (!isGet){
		char len[10];
		itoa(strlen(data), len, 10);
		toSend.append("Content-Length: ").append(len).append("\r\n");
	}

	if (!cookie.empty()){
		toSend.append("Cookie:");
		map<string, string>::iterator cookieIt = cookie.begin();
		for (; cookieIt != cookie.end(); ++cookieIt){
			toSend.append(" ").append(cookieIt->first).append("=").append(cookieIt->second).append(";");
		}
		toSend.append("\r\n");
	}
	toSend.append("\r\n");
	if (!isGet){
		toSend.append(data);
	}

	send(sock, toSend.c_str(), toSend.length(), 0);

	char buffer[BUFFER_LEN + 1] = { 0 };
	int recvLen = 0;
	while (1){
		memset(buffer, 0, sizeof(char)* BUFFER_LEN);
		recvLen = recv(sock, buffer, BUFFER_LEN, 0);
		if (recvLen > 0){
			response.recvStr.append(buffer);
			continue;
		}
		break;
	}

	response.code = response.recvStr.length();

	processResponse(&response);

	return response;
}


/*
Send a request using GET Method

@param url The request addr , without "http://"
@param cookie The cookie you would like to go
@param header The Header You want to be sent

@return a HTTPRESPONSE struct
**/
HTTPRESPONSE sendGet(const char * url, map<string, string> cookie, map<string, string> header){
	string addr;
	addr.append(url);

	int path = addr.find("/");
	string host = path > 0 ? addr.substr(0, path) : addr;
	string target = path > 0 ? addr.substr(path) : "";
	int port = 80;

	if ((path = host.find(":")) != string::npos){
		port = atoi(path + 1 >= host.length() ? "80" : host.substr(path + 1).c_str());
		host = host.substr(0, path);
	}

	return sendNet(host.c_str(), port, 1, target.c_str(), header, cookie, NULL);
}

/*
Send a request using POST method

@param url The request addr , without "http://"
@param cookie The cookie you would like to go
@param header The Header You want to be sent
@param data The data you would like to post to server

@return a HTTPRESPONSE struct
*/
HTTPRESPONSE sendPost(const char * url, map<string, string> cookie, map<string, string> header, const char * data){

	string addr;
	addr.append(url);

	int path = addr.find("/");
	string host = path > 0 ? addr.substr(0, path) : host;
	string target = path > 0 ? addr.substr(path) : "";
	int port = 80;

	if ((path = host.find(":")) != string::npos){
		port = atoi(path + 1 >= host.length() ? "80" : host.substr(path + 1).c_str());
		host = host.substr(0, path);
	}

	return sendNet(host.c_str(), port, 0, target.c_str(), header, cookie, data);

}
