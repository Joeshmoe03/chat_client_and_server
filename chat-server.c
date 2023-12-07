#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

typdef struct client {
	char* name;
	_	ip;
	char* port;
	int connfd;
	void* next;
} client;

int main(int argc, char* argv[]) {
	return 0;
}
