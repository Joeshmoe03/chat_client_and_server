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
#include <signal.h>

#define BUF_SIZ 4096
#define DELIM " "

pthread_mutex_t mutex;

/* Our struct serving as list of clients w/ crucial info */
typedef struct client {
	char* name;
	struct sockaddr_in remote_sa;
	int conn_fd;
	struct client* next;
} client;

/* Node information for singly-linked list */
client* head_client = NULL;

/* Some functions of importance for handling sockets, ports, and clients */
char* getPort(int argc, char* argv[]);
void creatBindSock(struct addrinfo* hintsP, struct addrinfo** resP, int* listen_fdP, char* listen_port);
client* addClient(int client_fd, struct sockaddr_in remote_sa);
void removClient(client* new_client);
int reNick(char msg[BUF_SIZ], char* out_msg[BUF_SIZ], client* new_client);
void exitMsg(char* out_msg[BUF_SIZ], client* new_client);
void formatMsg(char msg[BUF_SIZ], char* out_msg[BUF_SIZ], client* new_client);
int broadcast(char* out_msg[BUF_SIZ], int nbytes);
void* clientHandler(void* arg);

int main(int argc, char *argv[]) {

	/* Socket and port info */
	char* listen_port;
	int listen_fd;
	int client_fd;
	client* new_client;
	struct addrinfo hints, *res;
	struct sockaddr_in remote_sa;
	socklen_t addrlen;

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
		if((client_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen)) < 0) {
			perror("accept");
			continue;
		}

		/* Adds client with threading */
		if((new_client = addClient(client_fd, remote_sa)) == NULL) {
			perror("malloc");
			close(listen_fd);
			continue;
		}

		/* New thread for that client that was connected */
		pthread_t new_thread;
		pthread_create(&new_thread, NULL, clientHandler, (void*)new_client);
	} 

	/* Someone ends server, so we break into here */
	//TODO: TELL USERS SERVER GOES OFFLINE
	//TODO: ANY ADDITIONAL CLEANUP OF CLIENTS LIST
	//close(listen_fd);
	//TODO: CONSIDER USING KILL(2) SYSCALL ON SIGINT: have SA struct in sigint /setup sigaction(3p), associate sigaction with signal, do close handler, destroy mutex
}

/* Format informative message about renicknaming */
int reNick(char msg[BUF_SIZ], char* out_msg[BUF_SIZ], client* new_client) {
	char* token = strtok(msg, DELIM);
	time_t timeval = time(NULL);
	struct tm* tmstruct = localtime(&timeval);

	/* First token is is nickname command */
	if(strcmp(token, "/nick") == 0) {
		token = strtok(NULL, DELIM);
		if(token != NULL) {
			pthread_mutex_lock(&mutex);

			/* Set message to broadcast to this */
			snprintf(*out_msg, BUF_SIZ, "%d:%d:%d: User %s (%s:%d) is now known as %s", tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec, 
					 new_client->name, inet_ntoa(new_client->remote_sa.sin_addr), ntohs(new_client->remote_sa.sin_port), token);
			
			/* No need to realloc since buffer is already max that can go with a single send */
			new_client->name = token;
			pthread_mutex_unlock(&mutex);
			return 0;
		}
		return -1;
	}
}

/* Format the exit message */
void exitMsg(char* out_msg[BUF_SIZ], client* new_client) {
	time_t timeval = time(NULL);
	struct tm* tmstruct = localtime(&timeval);

	/* Copy exit message to buffer */
	snprintf(*out_msg, BUF_SIZ, "%d:%d:%d: User %s (%s:%d) has disconnected.", tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec,
			 new_client->name, inet_ntoa(new_client->remote_sa.sin_addr), ntohs(new_client->remote_sa.sin_port));
	return;
}

/* Format a regular message */
void formatMsg(char msg[BUF_SIZ], char* out_msg[BUF_SIZ], client* new_client) {
	time_t timeval = time(NULL);
	struct tm* tmstruct = localtime(&timeval);

	/* Copy message to buffer */
	snprintf(*out_msg, BUF_SIZ, "%d:%d:%d: %s: ", tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec, new_client->name);
	return;

}

/* Broadcasts message to ever client in client linked list */
int broadcast(char* out_msg[BUF_SIZ], int nbytes) {
	pthread_mutex_lock(&mutex);
	client* cur_client = head_client;
	while(cur_client != NULL) {
		if(send(cur_client->conn_fd, out_msg, nbytes, 0) < 0) {
			pthread_mutex_unlock(&mutex);
			return -1;
		}
		cur_client = cur_client->next;
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

/* Function for handling client msg of given thread */
void* clientHandler(void* arg) {
	client* new_client = (client*)arg;	

	/* Message stuff */
	int nbytes_recv;
	char in_msg[BUF_SIZ];
	char out_msg[BUF_SIZ];

	/* Get incoming message to server to broadcast back out */
	while((nbytes_recv = recv(new_client->conn_fd, in_msg, BUF_SIZ, 0)) > 0) {
		
		/* Attempt to tokenize input to see if /nick command passed */
		if(reNick(in_msg, &out_msg, new_client) == 0) {
			broadcast(out_msg, BUF_SIZ);
			continue;
		}
		
		/* Otherwise to regular message formatting and broadcasting */
		formatMsg(msg, &out_msg, new_client);
		broadcast(out_msg, BUF_SIZ);
	}
	/* Exit broadcasting setup */
	exitMsg(out_msg, new_client);
	broadcast(out_msg, BUF_SIZ);
	
	/* Clean up once client done and exits */
	close(new_client->conn_fd);
	removClient(new_client);
	return NULL;
}

/* Function for adding new connected clients to tracking */
client* addClient(int client_fd, struct sockaddr_in client_sa) {
	client* new_client;
	client* cur_client = head_client;
	char* default_name = "unknown";

	/* Set aside space for new client info */
	if((new_client = malloc(sizeof(client))) == NULL) {
		return NULL;
	}
	new_client->conn_fd = client_fd;
	if((new_client->name = malloc(BUF_SIZ)) == NULL) {
		free(new_client);
		return NULL;
	}

	/* Fill in struct info for client: ip and rest can be obtained from remote_sa, so no need as entries */
	strncpy(new_client->name, default_name, strlen(default_name));
	new_client->remote_sa = client_sa;
	new_client->next = NULL;
	pthread_mutex_lock(&mutex);

	/* When first adding client list w/ first person */
	if(cur_client == NULL) {
		head_client = new_client;
		pthread_mutex_unlock(&mutex);
		return new_client;
	}

	/* I guess we just put client as new head otherwise... No need to iterate. */
	new_client->next = head_client;
	head_client = new_client;
	pthread_mutex_unlock(&mutex);
	return new_client;
}

/* Function for removing existing connected clients */
void removClient(client* new_client) {
	pthread_mutex_lock(&mutex);
	//TODO: behavior to delete client
	pthread_mutex_unlock(&mutex);
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
void creatBindSock(struct addrinfo* hintsP, struct addrinfo** resP, int* listen_fdP, char* listen_port) {
	int rc;

	/* Create a socket */
	if((*listen_fdP = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("SERVER FAILURE: could not create socket");
		exit(EXIT_FAILURE);
	}
	
	/* Clear struct, specify to listen, getting addr, bind to port */
	memset(hintsP, 0, sizeof(*hintsP));
	*hintsP->ai_family = AF_INET;
	*hintsP->ai_socktype = SOCK_STREAM;
	*hintsP->ai_flags = AI_PASSIVE;
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
