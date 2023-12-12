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
#define CMD_DELIM " \n"
#define MSG_DELIM "\n"

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
int listen_fd;

/* Some functions of importance for handling sockets, ports, and clients */
char* getPort(int argc, char* argv[]);
void creatBindSock(struct addrinfo* hintsP, struct addrinfo** resP, int* listen_fdP, char* listen_port);
client* addClient(int client_fd, struct sockaddr_in remote_sa);
int removClient(client* new_client);
void freeAll(void);

/* Sighandler for sigint */
void sigintHandler(int signum);

/* Allows broadcast message formatting for when changing usernames */
int reNick(char msg[BUF_SIZ], char* out_msg, client* new_client, int* nbytes_send);

/* Allows broadcast message formatting when exiting CTRL-D */
int exitMsg(char* out_msg, client* new_client, int* nbytes_send);

/* Allows standard message broadcast */
int formatMsg(char msg[BUF_SIZ], char* out_msg, client* new_client, int* nbytes_send);

/* Broadcasts message to all */
int broadcast(char* out_msg, int nbytes);

/* Client Handler */
void* clientHandler(void* arg);

int main(int argc, char *argv[]) {

	/* Socket and port info */
	char* listen_port;
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
	
	/* Sig handler for CTR-C */
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = sigintHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction(SIGINT, &sa, NULL) < 0) {
		perror("sigaction");
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

		/* Announce new client */
		printf("New connection from %s:%d\n", inet_ntoa(remote_sa.sin_addr), ntohs(remote_sa.sin_port));

		/* Adds client with threading */
		if((new_client = addClient(client_fd, remote_sa)) == NULL) {
			perror("malloc");
			close(listen_fd);
			continue;
		}
	
		/* New thread for that client that was connected */
		pthread_t new_thread;
		if(pthread_create(&new_thread, NULL, clientHandler, (void*)new_client) < 0) {
			removClient(new_client);
			perror("pthread_create");
			continue;
		}
		pthread_detach(new_thread);
	} 
}

void sigintHandler(int signum) {

	/* Free everything and close socket */	
	freeAll();
 
	/* Kill process and all associated threads */
	kill(getpid(), SIGTERM);
}

/* Destroys linked list and does cleanup */
void freeAll(void) {

	/* Iterate thru and destroy struct with free */
	pthread_mutex_lock(&mutex);
	client* cur_client = head_client;
	client* next_client;
	while(cur_client != NULL) {
		next_client = cur_client->next;
		free(cur_client->name);
		free(cur_client);
		cur_client = next_client;
	}
	pthread_mutex_unlock(&mutex);
	
	/* Close listen socket */
	close(listen_fd);
	pthread_mutex_destroy(&mutex);
}

/* Format informative message about renicknaming */
int reNick(char msg[BUF_SIZ], char* out_msg, client* new_client, int* nbytes_send) {

	/* Unfortunately, strtok() is destructive to original input and so I am forced to copy and perform parsing on copy. 		 *
 	 * See: https://wiki.sei.cmu.edu/confluence/display/c/STR06-C.+Do+not+assume+that+strtok()+leaves+the+parse+string+unchanged */
	char* msg_cpy = malloc(strlen(msg) + 1);
	if(msg_cpy == NULL) {
		return -1;
	}
	strncpy(msg_cpy, msg, strlen(msg) + 1);

	/* First token and time information for broadcast message */
	char* token = strtok(msg_cpy, CMD_DELIM);
	if(token == NULL) {
		free(msg_cpy);
		return -1;
	}
	time_t timeval = time(NULL);
	struct tm* tmstruct = localtime(&timeval);

	/* First token is is nickname command */
	if(strcmp(token, "/nick") == 0) {
		token = strtok(NULL, CMD_DELIM);
		if(token != NULL) {
			pthread_mutex_lock(&mutex);

			/* Set message to broadcast to this */
			*nbytes_send = snprintf(out_msg, BUF_SIZ, "%d:%d:%d: User %s (%s:%d) is now known as %s", tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec, 
					 				new_client->name, inet_ntoa(new_client->remote_sa.sin_addr), ntohs(new_client->remote_sa.sin_port), token);
			
			/* No need to realloc since buffer is already max that can go with a single send */
			token[strlen(token)] = '\0';
			strncpy(new_client->name, token, strlen(token) + 1);
			pthread_mutex_unlock(&mutex);
			free(msg_cpy);
			return 1;
		}
	}
	free(msg_cpy);
	return 0;
}

/* Format the exit message */
int exitMsg(char* out_msg, client* new_client, int* nbytes_send) {
	time_t timeval = time(NULL);
	struct tm* tmstruct = localtime(&timeval);
	pthread_mutex_lock(&mutex);

	/* Copy exit message to buffer */
	*nbytes_send = snprintf(out_msg, BUF_SIZ, "%d:%d:%d: User %s (%s:%d) has disconnected.", tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec,
			 new_client->name, inet_ntoa(new_client->remote_sa.sin_addr), ntohs(new_client->remote_sa.sin_port));
	pthread_mutex_unlock(&mutex);
	return 0;
}

/* Format a regular message */
int formatMsg(char msg[BUF_SIZ], char* out_msg, client* new_client, int* nbytes_send) {
	time_t timeval = time(NULL);
	struct tm* tmstruct = localtime(&timeval);
	pthread_mutex_lock(&mutex);

	/* This delimeter ensures if user its enter (\n) at end to send msg, it removes new line */
	msg = strtok(msg, MSG_DELIM);
	msg[strlen(msg)] = '\0';
	
	/* User entered new line with no message */
	if(msg == NULL) {
		return -1;
	}

	/* Copy message to buffer */
	*nbytes_send = snprintf(out_msg, BUF_SIZ, "%d:%d:%d: %s: %s", tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec, new_client->name, msg);
	pthread_mutex_unlock(&mutex);
	return 0;
}

/* Broadcasts message to ever client in client linked list */
int broadcast(char* out_msg, int nbytes) {
	pthread_mutex_lock(&mutex);
	client* cur_client = head_client;
	out_msg[BUF_SIZ] = '\0';
	while(cur_client != NULL) {
		if(send(cur_client->conn_fd, out_msg, nbytes + 1, 0) < 0) {
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
	int nbytes_send;
	char in_msg[BUF_SIZ];
	char out_msg[BUF_SIZ];
	int reNick_success;	

	/* Get incoming message to server to broadcast back out */
	while((nbytes_recv = recv(new_client->conn_fd, in_msg, BUF_SIZ, 0)) > 0) {
	
		/* Attempt to tokenize input to see if /nick command passed, tokenization failure gives us -1 which means msg was bad */
		if((reNick_success = reNick(in_msg, out_msg, new_client, &nbytes_send)) == 1) { 
			broadcast(out_msg, nbytes_send);
			puts(out_msg);
			continue;
		} else if (reNick_success == -1) {
			continue;
		}
		
		/* Msg was not a command: otherwise to regular message formatting and broadcasting */
		if(formatMsg(in_msg, out_msg, new_client, &nbytes_send) < 0) {
			continue;
		}
		broadcast(out_msg, nbytes_send);
	}

	/* Exit broadcasting setup */
	exitMsg(out_msg, new_client, &nbytes_send);
	printf("Lost connection from %s.\n", new_client->name);
	broadcast(out_msg, nbytes_send);
	
	/* Clean up once client done and exits */
	close(new_client->conn_fd);
	removClient(new_client);
	return NULL;
}

/* Function for adding new connected clients to tracking */
client* addClient(int client_fd, struct sockaddr_in client_sa) {
	client* new_client;
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
	strncpy(new_client->name, default_name, strlen(default_name) + 1);
	new_client->remote_sa = client_sa;
	new_client->next = NULL;
	pthread_mutex_lock(&mutex);

	/* When first adding client list w/ first person */
	if(head_client == NULL) {
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
int removClient(client* del_client) {
	pthread_mutex_lock(&mutex);
	client* cur_client = head_client;
	if(cur_client == del_client) {
		head_client = cur_client->next;
		free(cur_client->name);
		free(cur_client);
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	while(cur_client->next != NULL) {
		if(cur_client->next == del_client) {
			cur_client->next = del_client->next;
			free(del_client->name);
			free(del_client);
			pthread_mutex_unlock(&mutex);
			return 0;
		}
	}	
	pthread_mutex_unlock(&mutex);
	return -1;
}

/* Make sure argument actually exists for port */
char* getPort(int argc, char* argv[]) {
	if(argc != 2) {
		printf("SERVER FAILURE: specified unexpected number of arguments. Only pass the port number...\n");
		exit(EXIT_FAILURE);
	}
	return argv[1];
}

/* Basic bind and create socket to port function for server */
void creatBindSock(struct addrinfo* hintsP, struct addrinfo** resP, int* listen_fdP, char* listen_port) {
	int rc;

	/* Create a socket */
	if((*listen_fdP = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("SERVER FAILURE: could not create socket\n");
		exit(EXIT_FAILURE);
	}
	
	/* Clear struct, specify to listen, getting addr, bind to port */
	memset(hintsP, 0, sizeof(*hintsP));
	hintsP->ai_family = AF_INET;
	hintsP->ai_socktype = SOCK_STREAM;
	hintsP->ai_flags = AI_PASSIVE;
	if((rc = getaddrinfo(NULL, listen_port, hintsP, resP)) != 0) {
		printf("SERVER FAILURE: getaddrinfo failed: %s\n", gai_strerror(rc));
		exit(EXIT_FAILURE);
	}
	if(bind(*listen_fdP, (*resP)->ai_addr, (*resP)->ai_addrlen) < 0) {
		printf("SERVER FAILURE: Could not bind socket to port...\n");
		exit(EXIT_FAILURE);
	}
	return;
}
