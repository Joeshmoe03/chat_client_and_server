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
	char msg[BUF_SIZE+1];
	int n; 

	/* Infinite loop for send() */
	while((n = read(0, msg, BUF_SIZE)) > 0) {
		if (n < 0) {
			perror("read");
			continue; //break; exit; are alternatives
		}

		// fflush(stdout); needed?

		msg[n] = '\0'; //SUS?
		send(connfd, msg, n, 0);
	}

	//SUS: to send server EOF
	msg[0] = '\0';
	send(connfd, msg, 0, 0);
	printf("Exiting.");

	return NULL;
}

void *recvfunc(void *data) {

	/* Buffer to store incoming message info */
	char msg[BUF_SIZE+1];
	// char output[BUF_SIZE];
	int bytes_received;

	/* Infinite loop for recv() */
	while(1) {

		/*upon server termination using, clients will terminate */
		if((bytes_received = recv(connfd, msg, BUF_SIZE, 0)) == 0) {
			puts("Connection closed by remote host.");
			exit(EXIT_SUCCESS); //pthread exit instead?
		}
		msg[bytes_received] = '\0'; /* SUPER IMPORTANT*/
		puts(msg);
	}

	return NULL;  
}


int main(int argc, char *argv[])
{
	char *dest_hostname, *dest_port;
	struct addrinfo hints, *res;
	int rc;

	void *retval;

	dest_hostname = argv[1];
	dest_port     = argv[2];

	/* create a socket */
	connfd = socket(PF_INET, SOCK_STREAM, 0);
	if(connfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* client usually doesn't bind, which lets kernel pick a port number */

	/* but we do need to find the IP address of the server */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if((rc = getaddrinfo(dest_hostname, dest_port, &hints, &res)) != 0) {
		printf("getaddrinfo failed: %s\n", gai_strerror(rc));
		exit(EXIT_FAILURE);
	}

	/* connect to the server */
	if(connect(connfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	printf("Connected\n");

	/* create separate threads for send & recv */
	pthread_t sendthread, recvthread;
	if(pthread_create(&sendthread, NULL, sendfunc, NULL) != 0) {
		perror("pthread_create");
	}
	if(pthread_create(&recvthread, NULL, recvfunc, NULL) !=0) {
		perror("pthread_create");
	}

	/*join threads, so they are ensured to finish their duty ?*/
	if(pthread_join(sendthread, &retval) != 0) {
		perror("pthread_join");
	}
	//pthread_join(recvthread, &retval); // won't work b/cuz the loop in recvthread is infinite so that thread will never finish, think it's fine
	//make it detached? or make it exit? or do we even need this?

	close(connfd);
}
