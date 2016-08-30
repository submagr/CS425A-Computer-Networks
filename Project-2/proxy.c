#include "proxy_parse.h"
#include<iostream>
#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<iostream>
#include<string>
#include<csignal>
#include<netdb.h>
using namespace std;

#define maxClientsWaiting 100  // Max number of waiting clients
#define bufferSize 9000 
#define smallBufferSize 1000 
#define maxForkedChilds 2
char proxyRequestFormat[] = "GET %s HTTP/1.0\r\nHost: %s\r\n%s\r\n\r\n";
char defaultProxyPort[] = "80\0";
int numForkedChild = 0;
char errResponseFormat[] = "HTTP/1.0 %d %s\r\n\r\n";

struct status{
	int status;
	char msg[1000];
};
typedef struct status status;

status S500 = {500, "Internal Error\0"};

void sendError(int sockid, status resStatus){
	char buffer[smallBufferSize];
	bzero(buffer, smallBufferSize);
	snprintf(buffer, smallBufferSize, errResponseFormat, resStatus.status, resStatus.msg);
	send(sockid, buffer, strlen(buffer),0);
	return;
}
void updateNumForkedChild(int update){
	//TODO: Make it shared and race free in parent and child
	numForkedChild = numForkedChild + update;
	return;
}
int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
		printf("get host by name error");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
    printf("%d\n", i) ;
    return 1;
}

void getProxyRequest(ParsedRequest* req, char* buffer){
	//TODO: Fill headers
	snprintf(buffer, bufferSize, proxyRequestFormat, req->path, req->host, "" );
	return;
}

int main(int argc, char* argv[]){
 	if(argc<2){
		printf("Usage ./proxy portNumber");
		return 0;
	}
	int port = stoi(argv[1]);
	int sockid = socket(AF_INET,SOCK_STREAM, 0); 
	if(sockid<0){
		printf("Unable to get sockid");
		return 0;
	}
	struct sockaddr_in serverAdd;
	serverAdd.sin_family = AF_INET;
	serverAdd.sin_port=htons(port);
	serverAdd.sin_addr.s_addr=INADDR_ANY;
	int bind_status = bind(sockid, (struct sockaddr*)&serverAdd, sizeof(serverAdd));
	if(bind_status==0)
		printf("bind successful\n");
	else{
		printf("bind failed\n");
		return 0; 
	}
	int listen_status = listen(sockid, maxClientsWaiting);
	if(listen_status==0)
		printf("Listening ...\n");
	else{
		printf("Listening failed\n");
		return 0;
	}
	// accept connections in loop

	while(1){
		struct sockaddr clientAdd;
		socklen_t addrLen;
		int newSockid = accept(sockid, &clientAdd, &addrLen);
		// Keep waiting until
		while(numForkedChild >= maxForkedChilds);
		printf("Number of forked Children: %d\n", numForkedChild);
		if(fork()==0){
			close(sockid); //Child will not listen
			char buffer[bufferSize];
			bzero(buffer, strlen(buffer));
			int n = recv(newSockid, buffer, bufferSize, 0);
			printf("--------Request Recieved(%d)------\n%s\n-----------------\n", n, buffer);
			
			//Parse request 
			int len = strlen(buffer); 
			//Create a ParsedRequest to use. This ParsedRequest
			//is dynamically allocated.
			ParsedRequest *req = ParsedRequest_create();
			if (ParsedRequest_parse(req, buffer, len) < 0) {
				printf("Request parsing failed\n");
				sendError(newSockid, S500);
				return 0;
			}
			printf("Request parsing completed\n");

			if(strcmp(req->method, "GET") != 0 ){
				printf("Method %s not implemented\n", req->method);
				sendError(newSockid, S500);
				return 0;
			}
			if(req->port==NULL){
				req->port = (char*)malloc(6);
				bzero(req->port, 6);
				strcpy(req->port, defaultProxyPort);
				printf("Port NUll, Using %s\n", req->port);
			}

			char ip[50];
			bzero(ip, 50);
			int k = hostname_to_ip(req->host, ip);
			printf("hostname_to_ip completed\n");
			printf("%d host: %s, port: %d, ip: %s path: %s\n", k, req->host, stoi(req->port), ip, req->path);
			// TODO: read above from previous request. 
			int fSockid = socket(AF_INET,SOCK_STREAM, 0); 
			struct sockaddr_in fServerAdd;
			fServerAdd.sin_family = AF_INET;
			fServerAdd.sin_port=htons(stoi(req->port));
			fServerAdd.sin_addr.s_addr=inet_addr(ip);
			int fAddrLen = sizeof(fServerAdd);
			int conn_status = connect(fSockid,(struct sockaddr*)&fServerAdd, fAddrLen);
			if(conn_status==0){
				printf("Connection to host established\n");
			}
			else{
				printf("Connection to host failed: %d\n ", conn_status);
				return 0;
				//TODO: Handle
			}
			//TODO: Check if method is not get
			char proxyRequest[bufferSize];	
			bzero(proxyRequest, bufferSize);
			getProxyRequest(req, proxyRequest);
			printf("--------Proxy sending request------\n%s\n-----------------\n", proxyRequest);
			n = send(fSockid,proxyRequest,strlen(buffer),0);
			printf("%d bytes sent\n", n);
			bzero(buffer, strlen(buffer));
			printf("Waiting from server to reply...\n");
			int bytes = 0;
			printf("--------Response from host recieved------\n");
			while((n = recv(fSockid, buffer, bufferSize, 0)) != 0){
				printf("%s",buffer);
				// Send to client
				send(newSockid,buffer,strlen(buffer),0);
				bytes = bytes+n;
				bzero(buffer, strlen(buffer));
			}
			printf("\n-----------------\n");
			updateNumForkedChild(-1);
			close(newSockid);
			return 0;
		}
		else{
			updateNumForkedChild(1);
		}
	}
	return 0;
}
