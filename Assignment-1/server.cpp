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
using namespace std;

#define bufferSize 4096 

// getFileSize function is referred from stack overflow: http://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
int getFileSize(char* fileName){
	FILE *p_file = NULL;
	p_file = fopen(fileName,"rb");
	fseek(p_file,0,SEEK_END);
	int size = ftell(p_file);
	fclose(p_file);
	return size;	
}

int main(int argc, char* argv[]){
	// To handle exited clients
	signal(SIGPIPE, SIG_IGN);
	if(argc<2){
		printf("Usage ./server portNumber");
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
	// Configure ip address of server below
	serverAdd.sin_addr.s_addr=INADDR_ANY;
	int bind_status = bind(sockid, (struct sockaddr*)&serverAdd, sizeof(serverAdd));
	if(bind_status==0)
		printf("bind successful\n");
	else{
		printf("bind failed\n");
		return 0; 
	}
	int queueLimit = 10; // Max number of clients
	int listen_status = listen(sockid, queueLimit);
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

		// Seding greeting message 
		char buffer[bufferSize] = "Hello client!\n";
		FILE* socketRead = fdopen(newSockid, "r+");
		int n = fwrite(buffer, 1, 15, socketRead);
		fflush(socketRead);

		// keep accepting file requests until client closes the connection
		int clientStatus=1;
		// recieve value is 0 if peer has performed an orderly shutdown
		while(clientStatus>0){
			bzero(buffer, (int)strlen(buffer));
			clientStatus = fread(buffer, 1, bufferSize, socketRead);
			FILE* fd=NULL;
			fd = fopen(buffer, "r");
			if(fd==NULL){
				// File not found, send regret 
				bzero(buffer, (int)strlen(buffer));
				char notFoundStringSize[] = "20";
				strcpy(buffer, notFoundStringSize);
				n = fwrite(buffer, 1, bufferSize, socketRead);
				fflush(socketRead);

				char notFound[] = "no such file exists\n";
				bzero(buffer, bufferSize);
				strcpy(buffer, notFound);
				n = fwrite(buffer, 1, bufferSize, socketRead);
				fflush(socketRead);
			}
			else{
				// File found, send contents
				int fileSize = getFileSize(buffer);
				bzero(buffer, bufferSize);
				sprintf(buffer, "%d", fileSize);
				n = fwrite(buffer, 1, bufferSize, socketRead);
				fflush(socketRead);

				bzero(buffer, bufferSize); 
				while((n=fread(buffer, 1, bufferSize, fd))>0){
					fwrite(buffer, 1, bufferSize, socketRead);
					fflush(socketRead);
					bzero(buffer, (int)strlen(buffer)); 
				}
				fclose(fd);
			}
		}
		printf("Client exited\n");
		close(newSockid);
	}
	return 0;
}
