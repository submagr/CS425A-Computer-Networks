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
	//printf("waiting for server to write\n");
	//fread(buffer, 1, 15, socketRead);
	//printf("Buffer read:\n");
	//printf("%s\n", buffer);

        //bzero(buffer,(int)strlen(buffer));
	//strcpy(buffer, "hello server!\n");
        //int n = fwrite(buffer, 1, 15, socketRead);
        //printf("%d\n", n);
	//fflush(socketRead);

	
	//int n = recv(sockid, &buffer, bufferSize, 0); //300 is max size
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
		//send(sockid, &fileName, int(strlen(fileName))+1, 0);
		int status = fwrite(buffer, 1, bufferSize, socketRead);
		if(status==0){
			printf("server exited\n");
			return 0;
		}
		fflush(socketRead);
		printf("sent filename >>>>>\n");

		bzero(buffer, (int)strlen(buffer));
		n=fread(buffer, 1, bufferSize, socketRead);
		if(n==0){
			printf("server exited\n");
			return 0;
		}
		int fileSize = stoi(buffer);
		int charRec = 0;
		printf("FileSize: %d\n",fileSize );

		bzero(buffer, (int)strlen(buffer));
		printf("File contents are:\n");
		//while((n=recv(sockid, &buffer, bufferSize, 0)) == bufferSize){
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
		//if(n!=0){
		//	printf("%s", buffer);
		//}
		printf("\n>>>>>>%d %d", n, charRec);
	}
	return 0;
}
