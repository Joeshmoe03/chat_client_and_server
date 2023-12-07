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
	char message[SOME_SIZE]; //SOME BUFFER FOR NOW
	struct clientinfo *next;
} client;

/* Node information for singly-linked list */
static *client head = NULL;

void insertClient() {
	//accept 
}

void deleteClient() {

}

void broadcast() {

}

int main(int argc, char *argv[]) {
    char *listen_port;
    int listen_fd, conn_fd;
    struct addrinfo hints, *res;
    int rc;
    struct sockaddr_in remote_sa;
    uint16_t remote_port;
    socklen_t addrlen;
    char *remote_ip;
    char buf[BUF_SIZE];
    int bytes_received;

	/* Person running server specifies port */
	listen_port = arv[1];

	/* Create a socket */
	listen_fd = socket(PF_INET, SOCK_STREAM, 0);

	/* Bind socket to port for server */
	...

	/* Loop */
	while(1) {
	
	/* accept and insert client if not already */
		accept connection;

		//TODO: LOCK THREAD
		if connection not already in clients list based on connfd {
			insertClient();
			create new thread for incoming messages from this client - maybe;
		}
		//TODO: UNLOCK THREAD
	
		//TODO: LOCK THREAD
		rcv 
		//TODO: UNLOCK THREAD
	
		//TODO: LOCK THREAD
		some kind of broadcast;
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

