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

// Ref: http://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
int getFileSize(char* fileName){
	FILE *p_file = NULL;
	p_file = fopen(fileName,"rb");
	fseek(p_file,0,SEEK_END);
	int size = ftell(p_file);
	fclose(p_file);
	return size;	
}

int main(int argc, char* argv[]){
	signal(SIGPIPE, SIG_IGN);
	if(argc<2){
		printf("Usage ./server portNumber");
		return 0;
	}
	int port = stoi(argv[1]);
	int sockid = socket(AF_INET,SOCK_STREAM, 0); 
	if(sockid<0){
		printf("Unable to get sockid");
	}
	struct sockaddr_in serverAdd;
	serverAdd.sin_family = AF_INET;
	serverAdd.sin_port=htons(port);
	serverAdd.sin_addr.s_addr=inet_addr("192.168.48.1");
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
	else
		printf("Listening failed\n");
	// accept connections in loop
	
	struct sockaddr clientAdd;
	socklen_t addrLen;

	// Seding greeting message 
	//char buffer[bufferSize] = "Hello client!\n";
	//FILE* socketRead = fdopen(newSockid, "r+");
	//int n = fwrite(buffer, 1, 15, socketRead);
	//printf("%d\n", n);
	//fflush(socketRead);

	//bzero(buffer,(int)strlen(buffer));
	//printf("waiting for client to write\n");
        //fread(buffer, 1, 15, socketRead);
	//printf("read from client done\n");
	//printf("Buffer read: %s\n", buffer);

	while(1){
		struct sockaddr clientAdd;
		socklen_t addrLen;
		int newSockid = accept(sockid, &clientAdd, &addrLen);

		// Seding greeting message 
		char buffer[bufferSize] = "Hello client!\n";
		FILE* socketRead = fdopen(newSockid, "r+");
		int n = fwrite(buffer, 1, 15, socketRead);
		fflush(socketRead);
		printf("%d\n", n);

		//int n = send(newSockid, &buffer, int(strlen(buffer))+1, MSG_NOSIGNAL); // exact number of bytes to send
		// keep accepting file requests until client closes the connection
		int clientStatus=1;
		// recieve value is 0 if peer has performed an orderly shutdown
		while(clientStatus>0){
			bzero(buffer, (int)strlen(buffer));
			//clientStatus = recv(newSockid, buffer, bufferSize, 0);
			clientStatus = fread(buffer, 1, bufferSize, socketRead);
			printf("FileName recieved: %s\n", buffer);
			FILE* fd=NULL;
			fd = fopen(buffer, "r");
			if(fd==NULL){
				bzero(buffer, (int)strlen(buffer));
				char notFoundStringSize[] = "20";
				strcpy(buffer, notFoundStringSize);
				n = fwrite(buffer, 1, bufferSize, socketRead);
				fflush(socketRead);

				char notFound[] = "no such file exists";
				bzero(buffer, bufferSize);
				strcpy(buffer, notFound);
				// n = send(newSockid, &buffer, int(strlen(buffer)), MSG_NOSIGNAL); 
				// exact number of bytes to send
				n = fwrite(buffer, 1, bufferSize, socketRead);
				fflush(socketRead);
			}
			else{
				int fileSize = getFileSize(buffer);
				printf(">>>>>>>> fileSize = %d\n", fileSize);
				bzero(buffer, bufferSize);
				sprintf(buffer, "%d", fileSize);
				n = fwrite(buffer, 1, bufferSize, socketRead);
				fflush(socketRead);

				bzero(buffer, bufferSize); 
				while((n=fread(buffer, 1, bufferSize, fd))>0){
					fwrite(buffer, 1, bufferSize, socketRead);
					fflush(socketRead);
					printf("%s\n", buffer);
					bzero(buffer, (int)strlen(buffer)); 
				}
				//while((n=fread(buffer, 1, bufferSize, fd))== bufferSize){ 
				//	//send(newSockid, &buffer, bufferSize, MSG_NOSIGNAL);
				//	fwrite(buffer, 1, bufferSize, socketRead);
				//	fflush(socketRead);
				//	bzero(buffer, (int)strlen(buffer)); 
				//} 
				//if(n!=0){
				//	//send(newSockid, &buffer, (int)strlen(buffer), MSG_NOSIGNAL);
				//	fwrite(buffer, 1, bufferSize, socketRead);
				//	fflush(socketRead);
				//	bzero(buffer, (int)strlen(buffer)); 
				//}
				fclose(fd);
			}
			printf("Sent >>>>>>>>\n");
		}
		printf("client exited :(\n");
		close(newSockid);
	}
	return 0;
}
