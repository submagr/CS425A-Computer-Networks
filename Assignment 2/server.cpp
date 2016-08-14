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
#include <unistd.h> // For finding curr working directory
#include <errno.h>
using namespace std;

#define bufferSize 4096 
#define reqLineSize 8000
#define maxClients 100
#define maxFileLocationSize 1024


struct status{
	int status;
	char msg[1000];
};
typedef struct status status;

status S200 = {200, "OK\0"};
status S400 = {400, "Bad Request\0"};
status S404 = {404, "Not Found\0"};

// getFileSize function is referred from stack overflow: http://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
int getFileSize(char* fileName){
	FILE *p_file = NULL;
	p_file = fopen(fileName,"rb");
	fseek(p_file,0,SEEK_END);
	int size = ftell(p_file);
	fclose(p_file);
	return size;	
}

class parsedReq{
	public:
		char reqType[reqLineSize], url[reqLineSize], protocol[reqLineSize], message[reqLineSize];
		status stat;
		parsedReq(char* req){
			bzero(reqType,  reqLineSize);
			bzero(url,      reqLineSize);
			bzero(protocol, reqLineSize);
			bzero(message,  reqLineSize);
			int i=0, j=0;
			int reqSize = strlen(req);
			while(i<reqSize and req[i]!= ' '){
				reqType[j] = req[i];
				i++; j++;
			}
			i++; //ignore space
			j=0;
			while(i<reqSize and req[i]!= ' '){
				url[j] = req[i];
				i++; j++;
			}
			i++; //ignore space
			j=0;
			while(i<reqSize and req[i]!= '\r'){
				protocol[j] = req[i];
				i++; j++;
			}
			i++; 
			if(req[i]=='\n'){
				//req line completely parsed
				stat = S200;
				correctURI(url);
			}
		}

		void correctURI(char* url){
			if(url[0]=='/'){
				if(url[1]=='\0')
					strcpy(url, "/index.html");
			}else{
				stat = S400;
			}
			return;
		}
};

int main(int argc, char* argv[]){
	// To handle exited clients
	signal(SIGPIPE, SIG_IGN);
	if(argc<2){
		printf("Usage ./server portNumber baseDirectory");
		return 0;
	}
	int port = stoi(argv[1]);

	char cwd[1024];
	if(argc>2){
		strcpy(cwd, argv[2]);
		fprintf(stdout, "Using base dir: %s\n", cwd);
	}else{
		if (getcwd(cwd, sizeof(cwd)) != NULL)
			fprintf(stdout, "Using base dir: %s\n", cwd);
		else{
			perror("getcwd() error");
			return 0;
		}
	}

	int sockid = socket(AF_INET,SOCK_STREAM, 0); 
	//To handle multiple clients
	int clientFd[maxClients];
	int slot = 0, i;
	for(i=0;i<maxClients; i++)
		clientFd[i] = -1;
	if(sockid<0){
		printf("Unable to get sockid");
		return 0;
	}
	struct sockaddr_in serverAdd;
	serverAdd.sin_family = AF_INET;
	serverAdd.sin_port=htons(port);
	// Configure ip address of server below
	serverAdd.sin_addr.s_addr=inet_addr("127.0.0.1");
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
		clientFd[slot] = accept(sockid, &clientAdd, &addrLen);
		if( clientFd[slot] < 0){
			printf("error in accept at slot");
			exit(0);
		}
		else{
			//Child will handle request
			if(fork()==0){
				// Seding greeting message 
				int newSockid = clientFd[slot];
				//char buffer[bufferSize] = "Hello client!\n";
				char buffer[bufferSize];
				//FILE* socketRead = fdopen(newSockid, "r+");
				//int n = fwrite(buffer, 1, 15, socketRead);
				int n;
				//fflush(socketRead);

				// keep accepting file requests until client closes the connection
				int clientStatus=1;
				// recieved value is 0 if peer has performed an orderly shutdown
				while(clientStatus>0){
					bzero(buffer, (int)strlen(buffer));
					clientStatus = recv(newSockid, buffer, bufferSize, 0);
					printf("%d %s\n", clientStatus, buffer);
					parsedReq* pReq = new parsedReq(buffer);	
					printf("%s---%s---%s---%d---%s---\n",pReq->reqType, pReq->url, pReq->protocol, pReq->stat.status, pReq->stat.msg); 
					//TODO: send response according to parsing error if any
					FILE* fd=NULL;
					char fileLocation[maxFileLocationSize];
					strcpy(fileLocation, cwd);
					strcat(fileLocation, pReq->url);
					fd = fopen(fileLocation, "r"); // ignore '/'
					if(fd==NULL){
						// File not found, send regret 
						bzero(buffer, (int)strlen(buffer));
						sprintf(buffer, "HTTP/1.1 404 Not Found\nContent-type: text/html\nContent-length: 0\n\n");
						printf("Response: %s\n", buffer);
						//n = fwrite(buffer, 1, bufferSize, socketRead);
						//fflush(socketRead);
						n = send(newSockid, buffer, strlen(buffer), 0);
						printf("response sent\n");
						//TODO : handle n if less than buffer size
					}
					else{
						// File found, send contents
						int fileSize = getFileSize(pReq->url+1);
						bzero(buffer, bufferSize);
						char contentType[]="application/octet-stream";
						sprintf(buffer, "HTTP/1.1 %d %s\nContent-type: %s\nContent-length: %d\n\n", pReq->stat.status, pReq->stat.msg, contentType, fileSize);
						n = send(newSockid, buffer, (int)strlen(buffer), 0);
						printf("Response: \n");
						printf("%s", buffer);
						//TODO : Check using n, that total number of bytes are sent.
						
						//bzero(buffer, bufferSize);
						//sprintf(buffer, "%d", fileSize);
						//n = fwrite(buffer, 1, bufferSize, socketRead);
						//fflush(socketRead);

						bzero(buffer, bufferSize); 
						while((n=fread(buffer, 1, bufferSize, fd))>0){
							n = send(newSockid, buffer, n, 0);
							printf("%s", buffer);
							//TODO: Check using n, that total number of bytes are sent.
							//fflush(socketRead);
							bzero(buffer, (int)strlen(buffer)); 
						}
						fclose(fd);
					}
				}
				printf("Client exited\n");
				close(clientFd[slot]);
				exit(0);
			}
			//Parent will listen on another socket
		}
		while(clientFd[slot]!=-1)
			slot = (slot+1)%maxClients;
	}
	return 0;
}
