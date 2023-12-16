/* chat-client.c */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define BUF_SIZE 4096

static int connfd;

/* Functions so a single client can concurrenly send/recv data */
void *sendfunc(void *data) {

	/* Buffer for message and its respective length */
	char msg[BUF_SIZE];
	int n; 

	/* Infinite loop for send() */
	while((n = read(0, msg, BUF_SIZE - 1)) > 0) {
		if (n < 0) {
			perror("read");
			continue;
		}
		
		/* n shoulder never be > 4096: the max read is 4096. Just ensure null termination. (n-1) */
		msg[n] = '\0';
		send(connfd, msg, n, 0);
	}

	/* Send server "EOF" in event of client disconnect (Ctrl-D) */
	printf("Exiting.\n");
	return NULL;
}

void *recvfunc(void *data) {

	/* Buffer to store incoming message info and its respective length*/
	char msg[BUF_SIZE+1];
	int bytes_received;

	/* Infinite loop for recv() */
	while(1) {

		/*upon server termination, clients will terminate */
		if((bytes_received = recv(connfd, msg, BUF_SIZE, 0)) == 0) {
			puts("Connection closed by remote host.");
			exit(EXIT_SUCCESS);
		}

		/* Display null terminated message that server has sent */
		msg[bytes_received] = '\0';
		puts(msg);
	}
	return NULL;  
}

int main(int argc, char *argv[])
{
	char *dest_hostname, *dest_port;
	struct addrinfo hints, *res;
	int rc;

	if (argc != 3) {
		printf("CLIENT FAILURE: specified unexpected number of arguments. Only provide hostname and port number.\n");
		exit(EXIT_FAILURE);
	}
	
	dest_hostname = argv[1];
	dest_port     = argv[2];

	/* Create a socket */
	connfd = socket(PF_INET, SOCK_STREAM, 0);
	if(connfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* Client usually doesn't bind, which lets kernel pick a port number, but we do need to find the IP address of the server */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if((rc = getaddrinfo(dest_hostname, dest_port, &hints, &res)) != 0) {
		printf("getaddrinfo failed: %s\n", gai_strerror(rc));
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}

	/* Connect to the server */
	if(connect(connfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("connect");
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}

	printf("Connected\n");

	/* Create separate threads for send & recv */
	pthread_t sendthread, recvthread;

	if(pthread_create(&sendthread, NULL, sendfunc, NULL) != 0) {
		perror("pthread_create");
	}

	if(pthread_create(&recvthread, NULL, recvfunc, NULL) != 0) {
		perror("pthread_create");
	}

	/* Join sending thread, so we are ensured it finishes its duty */
	if(pthread_join(sendthread, NULL) != 0) {
		perror("pthread_join");
	}

	freeaddrinfo(res);
	close(connfd);
}
