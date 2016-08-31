CC=g++
CFLAGS= -g -Wall -Werror

all: proxy

proxy: proxy.c
	$(CC) $(CFLAGS) -o proxy_parse.o -c proxy_parse.c -std=c++11
	$(CC) $(CFLAGS) -o proxy.o -c proxy.c -std=c++11
	$(CC) $(CFLAGS) -o proxy proxy_parse.o proxy.o -std=c++11 

clean:
	rm -f proxy *.o

tar:
	tar -cvzf ass1.tgz proxy.c README Makefile proxy_parse.c proxy_parse.h
