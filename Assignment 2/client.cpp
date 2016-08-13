#include<iostream>
#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<string>
#include<csignal>
using namespace std;

#define bufferSize 4096 

int main(int argc, char* argv[]){
	if(argc!=3){
		printf("usage: ./server server-ip-address port\n");
		return 0;
	}
	int port = stoi(argv[2]);
	int sockid = socket(AF_INET,SOCK_STREAM, 0); 
	struct sockaddr_in serverAdd;
	serverAdd.sin_family = AF_INET;
	serverAdd.sin_port=htons(port);
	serverAdd.sin_addr.s_addr=inet_addr(argv[1]);
	int addrLen = sizeof(serverAdd);
	int conn_status = connect(sockid,(struct sockaddr*)&serverAdd, addrLen);
	if(conn_status==0){
		printf("Connection established\n");
	}
	else{
		printf("Connection failed: %d\n ", conn_status);
		return 0;
	}

	char buffer[bufferSize+1];
	buffer[bufferSize] = '\0';

	FILE* socketRead = fdopen(sockid, "r+");
	int n = fread(buffer, 1, 15, socketRead);
	if(n==0){
		printf("server exited\n");
		return 0;
	}
	printf("%s\n", buffer);
	while(1){
		bzero(buffer, (int)strlen(buffer));
		printf("Enter filename: ");
		scanf("%s", buffer);
		int status = fwrite(buffer, 1, bufferSize, socketRead);
		if(status==0){
			printf("server exited\n");
			return 0;
		}
		fflush(socketRead);

		// get fileSize
		bzero(buffer, (int)strlen(buffer));
		n=fread(buffer, 1, bufferSize, socketRead);
		if(n==0){
			printf("server exited\n");
			return 0;
		}
		int fileSize = stoi(buffer);
		int charRec = 0;

		// get file contents
		bzero(buffer, (int)strlen(buffer));
		while((n=fread(buffer, 1, bufferSize, socketRead)) == bufferSize){
			if(n==0){
				printf("server exited\n");
				return 0;
			}
			charRec = charRec+n;
			printf("%s", buffer);
			bzero(buffer, (int)strlen(buffer));
			if(fileSize-charRec<=0)
				break;
		}
	}
	return 0;
}
