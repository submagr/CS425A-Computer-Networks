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
#include<unistd.h> // For finding curr working directory
#include<errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <dirent.h>
using namespace std;

#define bufferSize 4096 
#define reqLineSize 8000
#define maxClients 100
#define maxFileLocationSize 1024
#define maxContentTypeSize 1024 
#define smallBufferSize 1024

void getCurrTime(char* buffer, int maxSize){
	// Parameters:
	//		buffer := final time to be stored here,
	//		maxSize := max size of time string
	// Return-Value:
	//		current time in gmt
	// Example:
	//		Thu, 18 Aug 2016 15:19:48 GMT
	time_t t = time(NULL);
	struct tm tm = *gmtime(&t);
	strftime(buffer, maxSize, "%a, %d %Y %X %Z", &tm);
	return;
}

struct status{
	int status;
	char msg[1000];
};
typedef struct status status;

status S200 = {200, "OK\0"};
status S400 = {400, "Bad Request\0"};
status S404 = {404, "Not Found\0"};
status S501 = {501, "Not Implemented\0"};
status S505 = {505, "HTTP Version Not Supported\0"};
char ResponseFormat[] = "HTTP/1.1 %d %s\nContent-type: %s\nContent-length: %d\nDate: %s\nServer: %s\n\n%s";
char errorHtml[] = "<html><head><title>404 Not Found</title>\r\n</head><body><h1>%s</h1><p>Http Response Status: %d<br>Http Response Message: %s</p></body></html>";

char fileTypeHtml[][maxContentTypeSize] = {".html", ".htm"};
char fileTypePdf[][maxContentTypeSize] = {".pdf"};
char fileTypeImage[][maxContentTypeSize] = {".jpg", ".png", ".gif"};
char fileTypeText[][maxContentTypeSize] = {".txt", ".text", ".c", ".cpp"};

char contentTypeHtml[] = "text/html";
char contentTypePdf[] = "application/pdf";
char contentTypeImage[] = "image/gif";
char contentTypeOther[] = "application/octet-stream";
char contentTypeText[] = "text/plain";
char contentTypeDir[] = "Directory";

char cwd[1024];


// Reference: http://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
int is_directory(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

// Function for finding whether extension is in array of predefined arrays of content types
int inArray(char* srcExt, char targetExt[][maxContentTypeSize], int length ){
	for(int i=0; i<length;i++)
		if(strcmp(srcExt, targetExt[i])==0){
			printf("Matched extension %s\n", targetExt[i]);
			return 1;
		}
	return 0;		
}

// Returns content type
//Reference : http://stackoverflow.com/questions/5309471/getting-file-extension-in-c
void getContentType(char* filePath, char* contentType){
	char* ext = strrchr(filePath, '.');
	printf("Extracted extension: %s\n", ext);
	if (!ext || strcmp(ext,filePath)==0){
		//directory or file with no extension
		if(is_directory(filePath)){
			//directory
			strcpy(contentType, contentTypeDir);
			return;
		}
	}
	//html
	else if(inArray(ext, fileTypeHtml, sizeof(fileTypeHtml)/sizeof(*fileTypeHtml))){
		strcpy(contentType, contentTypeHtml);
		return;
	}
	//Image
	else if (inArray(ext, fileTypeImage, sizeof(fileTypeImage)/sizeof(*fileTypeImage))){
		strcpy(contentType, contentTypeImage);
		return;
	}
	//Pdf
	else if (inArray(ext, fileTypePdf, sizeof(fileTypePdf)/sizeof(*fileTypePdf))){
		strcpy(contentType, contentTypePdf);
		return;
	}
	//Pdf
	else if (inArray(ext, fileTypeText, sizeof(fileTypeText)/sizeof(*fileTypeText))){
		strcpy(contentType, contentTypeText);
		return;
	}
	//Others
	else{
		strcpy(contentType, contentTypeOther);
		return;
	}
}

// getFileSize function is referred from stack overflow: http://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
int getFileSize(char* fileName){
	FILE *p_file = NULL;
	p_file = fopen(fileName,"rb");
	fseek(p_file,0,SEEK_END);
	int size = ftell(p_file);
	fclose(p_file);
	return size;	
}

int getDirListing(char* buffer, char* directory, char* origDir){
	if(origDir[(int)strlen(origDir)-1]!='/')
		origDir[(int)strlen(origDir)] = '/';
	DIR *dp;
	struct dirent *ep;
	dp = opendir(directory);

	char listHtml[bufferSize];
	char relPath[smallBufferSize];
	char temp[smallBufferSize];
	char dirListFormat[] = "<!DOCTYPE html><html><body><h1>Directory Contents</h1><br>%s</body></html>";
	//printf("dirListFormat: %s\n", dirListFormat);
	if (dp != NULL)
	{
		while (ep = readdir(dp)){
			bzero(relPath, (int)strlen(relPath));
			bzero(temp, (int)strlen(temp));
			strncpy(relPath, origDir, smallBufferSize-1);
			strncat(relPath, ep->d_name, smallBufferSize - (int)strlen(relPath));
			snprintf(temp, smallBufferSize, "<li><a href=\"%s\">%s</li>", relPath, ep->d_name);
			printf("%s\n", temp);
			strncat(listHtml, temp, bufferSize - (int)strlen(listHtml));
		}
		printf("dirListFormat: %s\n", dirListFormat);
		snprintf(buffer, bufferSize, dirListFormat, listHtml);
		bzero(listHtml, (int)strlen(listHtml));
		(void) closedir (dp);
	}
	else{
		sprintf(buffer, "Couldn't open the directory");
		return 0;
	}
	return 1;
}

// Class for parsing request into various components such as url, protocol, method etc.
class parsedReq{
	public:
		char reqType[reqLineSize], url[reqLineSize], originalUrl[reqLineSize], protocol[reqLineSize], message[reqLineSize];
		status stat;
		parsedReq(char* req){
			bzero(reqType,  reqLineSize);
			bzero(url,      reqLineSize);
			bzero(originalUrl,      reqLineSize);
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
				originalUrl[j] = req[i];
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
				correctURI(originalUrl);
				checkReqSemantics();
			}else{
				stat = S400;
			}
		}

		void correctURI(char* originalUrl){
			if(originalUrl[0]=='/'){
				if(originalUrl[1]=='\0')
					strcpy(originalUrl, "/index.html");
			}else{
				stat = S400;
				strcpy(stat.msg,"URL should start with /");
			}
			sprintf(url, "%s%s", cwd, originalUrl);
			return;
		}
		void checkReqSemantics(){
			if(strcmp(reqType, "GET\0")!=0){
				stat=S501;
			}
			if(strcmp(protocol,"HTTP/1.1\0")!=0){
				stat = S505;
			}
		}
};

int main(int argc, char* argv[]){
	// To handle exited clients
	signal(SIGPIPE, SIG_IGN);
	if(argc<1){
		printf("Usage ./proxy portNumber\n");
		return 0;
	}
	int port = stoi(argv[1]);

	int sockid = socket(AF_INET,SOCK_STREAM, 0); 
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

	int clientFd[maxClients]; //To handle multiple clients
	int slot = 0, i;

	for(i=0; i<maxClients; i++)
		clientFd[i] = -1;
	int queueLimit = maxClients; // Max number of clients
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
			printf("error in accept at slot %d", slot);
			exit(0);
		}
		else{
			//Child will handle request
			if(fork()==0){
				// Send greeting message 
				close(sockid); // Close listening connection
				int newSockid = clientFd[slot];
				char buffer[bufferSize];
				int n;

				// keep accepting file requests until client closes the connection
				int clientStatus=1;
				char resHtml[bufferSize]; 
				char currTime[smallBufferSize];
				while(clientStatus>0){
					bzero(buffer, (int)strlen(buffer));
					bzero(resHtml, (int)strlen(resHtml));
					bzero(currTime, (int)strlen(currTime));
					clientStatus = recv(newSockid, buffer, bufferSize, 0);
					printf("Request recieved:\n---------------\n%s\n-----------------\n", buffer);
					parsedReq* pReq = new parsedReq(buffer);	
					if(pReq->stat.status!=200){
						bzero(buffer, (int)strlen(buffer));
						//get response html
						sprintf(resHtml, errorHtml, "Request parsing error", pReq->stat.status, pReq->stat.msg);
						//get response time
						getCurrTime(currTime, smallBufferSize);
						sprintf(buffer, ResponseFormat, pReq->stat.status, pReq->stat.msg, "text/html", strlen(resHtml), currTime, serverName, resHtml);
						send(newSockid, buffer, strlen(buffer), 0);
						printf("Response sent:\n---------------------------%s\n------------------\n",buffer);
						continue;
					}
					// send client request forward	
					//int sockid2 = socket(AF_INET,SOCK_STREAM, 0); 
					//struct sockaddr_in serverAdd2;
					//serverAdd.sin_family = AF_INET;
					//serverAdd.sin_port= 80// server port here ;
					//serverAdd.sin_addr.s_addr= INADDR_ANY// server ip here
					//int addrLen = sizeof(serverAdd);
					//int conn_status = connect(sockid,(struct sockaddr*)&serverAdd, addrLen);
					//if(conn_status == 0){
					//	printf("Connection established\n");
					//}
					//else{
					//	printf("Connection failed: %d\n ", conn_status);
					//	// TODO: Handle connection failed here
					//}

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
				printf("Client exited\n");
				close(clientFd[slot]);
				//TODO : Check race conditions here
				clientFd[slot] = -1;
				exit(0);
			}
			//Parent will listen on another socket
		}
		while(clientFd[slot]!=-1)
			slot = (slot+1)%maxClients;
	}
	return 0;
}
