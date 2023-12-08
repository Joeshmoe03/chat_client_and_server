/* chat-server.c */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define BACKLOG 10
#define BUF_SIZE 4096

pthread_mutex_t mutex;

/* Our struct serving as list of clients w/ crucial info */
typedef struct clientinfo {
	char* name;
    struct sockaddr_in remote_sa; 	/*filled in by accept*/
    char* ip; 						/*can get from struct remote_sa*/
	int port; 						/*can get from struct remote_sa*/
	int connfd; 					/*result of accept*/
	char msg[SOME_SIZE]; //SOME BUFFER FOR NOW
	struct clientinfo *next;
} client;

/* Node information for singly-linked list */
static *client head = NULL;

void insertClient() {
	return;	
}

void deleteClient() {
	return;
}

void broadcast() {
	return;
}

/* Make sure argument actually exists for port */
char* getPort(int argc, char* argv[]) {
	if(argc != 2) {
		printf("SERVER FAILURE: specified unexpected number of arguments. Only pass the port number...");
		exit(EXIT_FAILURE);
	}
	return argv[1];
}

/* Basic bind and create socket to port function for server */
void creatBindSock(struct addrinfo* hintsP, struct addrinfo** resP, char** listen_fdP, char* listen_port) {
	int rc;

	/* Create a socket */
	if((*listen_fdP = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("SERVER FAILURE: could not create socket");
		exit(EXIT_FAILURE);
	}
	
	/* Clear struct, specify to listen, getting addr, bind to port */
	memset(hintsP, 0, sizeof(*hintsP));
	*hintsP.ai_family = AF_INET;
	*hintsP.ai_socktype = SOCK_STREAM;
	*hints.ai_flags = AI_PASSIVE;
	if((rc = getaddrinfo(NULL, listen_port, hintsP, respP)) != 0) {
		printf("SERVER FAILURE: getaddrinfo failed: %s\n", gai_strerror(rc));
		exit(EXIT_FAILURE);
	}
	if(bind(*listen_fdP, *resP->ai_addr, *resP->ai_addrlen) < 0) {
		printf("SERVER FAILURE: Could not bind socket to port...");
		exit(EXIT_FAILURE);
	}
	return;
}

int main(int argc, char *argv[]) {
	char* listen_port;
    int listen_fd;	
	struct addrinfo hints, *res;
    struct sockaddr_in remote_sa;
    uint16_t remote_port;
    socklen_t addrlen;

    char *remote_ip;
    int bytes_received;

	/* Person running server specifies port w/ error checking */
	listen_port = getPort(argc, argv);

	/* Create and bind socket to port for server w/ error checking */
	creatBindSock(&hints, &res, &listen_fd, listen_port);

	/* We set to listen max */
	if(listen(listen_fd, SOMAXCONN) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* Loop body for connection */
	while(1) {
	
		/* Accept incoming connection */
		addrlen = sizeof(remote_sa);
		conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen);
 	
		//TODO: LOCK THREAD
		if connection not already in clients list based on connfd {
			insertClient();
			create new thread for incoming messages from this client;
		}
		//TODO: UNLOCK THREAD
	
		//TODO: LOCK THREAD
		receive message from the connection;
		traverse list to the correct client and update client struct with message and/or new nickname;
		somehow save that user just changed nickname if specified to inform other clients later - in struct possibly;
		//TODO: UNLOCK THREAD
	
		close connection
		
		check users that left when?

		//TODO: LOCK THREAD
		iterate over each user in the SLL:
			send message from last user to each client
		//TODO: UNLOCK THREAD

	close at end
	}
}



   // listen_port = argv[1];

   // /* create a socket */
   // listen_fd = socket(PF_INET, SOCK_STREAM, 0);

   // /* bind it to a port */
   // memset(&hints, 0, sizeof(hints));
   // hints.ai_family = AF_INET;
   // hints.ai_socktype = SOCK_STREAM;
   // hints.ai_flags = AI_PASSIVE;
   // if((rc = getaddrinfo(NULL, listen_port, &hints, &res)) != 0) {
   //     printf("getaddrinfo failed: %s\n", gai_strerror(rc));
   //     exit(1);
   // }

   // bind(listen_fd, res->ai_addr, res->ai_addrlen);

   // /* start listening */
   // listen(listen_fd, BACKLOG);

   // /* infinite loop of accepting new connections and handling them */ /* THIS IS WHERE WE NEED THE SAUCE */
   // while(1) {
   //     
   //     /* WHENEVER A CLIENT MAKES CONNECTION, WILL SET UP THREAD TO HANDLE IT, 
   //        our thread will trigger system that prints desired output based on client's inputs (sends this thread's message to all other threads?) */

   //     /* NEED A STRUCT holding all clients (thread info)
   //        add to it when we get new connection
   //        remove from it when we get new connection
   //        traverse it and give incoming msg from one client to all clients */


   //     /* accept a new connection (will block until one appears) */
   //     addrlen = sizeof(remote_sa);
   //     conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen);

   //     /* announce our communication partner */
   //     remote_ip = inet_ntoa(remote_sa.sin_addr);
   //     remote_port = ntohs(remote_sa.sin_port);
   //     printf("new connection from %s:%d\n", remote_ip, remote_port);

   //     /* A LOT OF THE SAUCE WILL ALSO BE HERE:  we can't just echo, gotta spread the word!*/
   //     /* receive and echo data until the other end closes the connection */
   //     while((bytes_received = recv(conn_fd, buf, BUF_SIZE, 0)) > 0) {
   //         printf(".");
   //         fflush(stdout);

   //         /* send it back */
   //         send(conn_fd, buf, bytes_received, 0);
   //     }
   //     printf("\n");

   //     close(conn_fd);
    }
}

