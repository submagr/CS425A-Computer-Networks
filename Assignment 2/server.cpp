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
char ResponseFormat[] = "HTTP/1.1 %d %s\nContent-type: %s\nContent-length: %d\n\n%s";
char parseErrorHtml[] = "<html><body><h1>Parse Error</h1></body></html>";
char fileNotFoundHtml[] = "Requested file not found";
char cwd[1024];

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
			while(i<reqSize and req[i]!= '\r' and req[i]!='\n'){
				protocol[j] = req[i];
				printf("%c", req[i]);
				i++; j++;
			}
			// RequestLine is valid if it ends with /r/n or /n
			if(req[i]=='\n' or (i<reqSize-1 and req[i] == '\r' and req[i+1] == '\n')){
				//req line completely parsed
				stat = S200;
				correctURI(url);
				checkReqSemantics();
			}else{
				stat = S400;
			}
		}

		void correctURI(char* url){
			if(url[0]=='/'){
				if(url[1]=='\0')
					strcpy(url, "/index.html");
			}else{
				stat = S400;
				strcpy(stat.msg,"URL should start with /");
			}
			char tempUrl[1024];
			sprintf(tempUrl, "%s%s", cwd, url);
			bzero(url, strlen(url));
			strcpy(url, tempUrl);
			return;
		}
		void checkReqSemantics(){
			if(strcmp(reqType, "GET\0")!=0){
				stat=S400;
				bzero(stat.msg, (int)strlen(stat.msg));
				strcpy(stat.msg,"Bad Request: Only Get Request is supported as of now\0");
			}
			if(strcmp(protocol,"HTTP/1.1\0")!=0){
				stat = S400;
				bzero(stat.msg, (int)strlen(stat.msg));
				strcpy(stat.msg, "Bad Request: Only HTTP/1.1 protocol is supported\0");
			}
		}
};

int main(int argc, char* argv[]){
	// To handle exited clients
	signal(SIGPIPE, SIG_IGN);
	if(argc<2){
		printf("Usage ./server portNumber [baseDirectory without / symbol at end]");
		return 0;
	}
	int port = stoi(argv[1]);

	if(argc>2){
		strcpy(cwd, argv[2]);
		fprintf(stdout, "Using base dir: %s\n", cwd);
		//TODO: Detect / at the end of directory
	}else{
		if (getcwd(cwd, sizeof(cwd)) != NULL){
			fprintf(stdout, "Using base dir: %s\n", cwd);
		}
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
					printf("Request recieved:\n--------\n%s\n-------\n", buffer);
					parsedReq* pReq = new parsedReq(buffer);	
					printf("Request Type: %s\nRequested Url: %s\nRequest Protocol: %s\nRequest Status: %d\nRequest Message: %s\n",pReq->reqType, pReq->url, pReq->protocol, pReq->stat.status, pReq->stat.msg); 
					if(pReq->stat.status!=200){
						printf("Unable to parse request");
						bzero(buffer, (int)strlen(buffer));
						//"HTTP/1.1 %d %s\nContent-type: %s\nContent-length: %d\n\n%s";
						sprintf(buffer, ResponseFormat, pReq->stat.status, pReq->stat.msg, "text/html", strlen(parseErrorHtml),parseErrorHtml);
						send(newSockid, buffer, strlen(buffer), 0);
						printf("Response sent:\n%s",buffer);
						continue;
					}
					else{
						printf("Request successfully parsed\n");
						printf("Request Type: %s\nRequested Url: %s\nRequest Protocol: %s\nRequest Status: %d\nRequest Message: %s\n",pReq->reqType, pReq->url, pReq->protocol, pReq->stat.status, pReq->stat.msg); 
					}
					//TODO: send response according to parsing error if any
					FILE* fd=NULL;
					//char fileLocation[maxFileLocationSize];
					//strcpy(fileLocation, cwd);
					//strcat(fileLocation, pReq->url);
					fd = fopen(pReq->url, "r"); 
					printf("[DEBUG] opening url: %s \n", pReq->url);
					if(fd==NULL){
						printf("[DEBUG] url not found \n");
						// File not found, send regret 
						bzero(buffer, (int)strlen(buffer));
						//"HTTP/1.1 %d %s\nContent-type: %s\nContent-length: %d\n\n%s";
						sprintf(buffer, ResponseFormat, 404, "Not Found", "text/html", (int)strlen(fileNotFoundHtml), fileNotFoundHtml);
						//n = fwrite(buffer, 1, bufferSize, socketRead);
						//fflush(socketRead);
						n = send(newSockid, buffer, strlen(buffer), 0);
						printf("Response sent:\n%s\n", buffer);
						//TODO : handle n if less than buffer size
					}
					else{
						printf("[DEBUG] url found \n");
						// File found, send contents
						int fileSize = getFileSize(pReq->url);
						bzero(buffer, bufferSize);
						char contentType[]="text/html";
						//"HTTP/1.1 %d %s\nContent-type: %s\nContent-length: %d\n\n%s";
						sprintf(buffer, ResponseFormat, pReq->stat.status, pReq->stat.msg, contentType, fileSize, "");
						n = send(newSockid, buffer, (int)strlen(buffer), 0);
						printf("Response sent:\n%s", buffer);
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
							printf("--------%s---------", buffer);
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
