#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <sys/select.h>


#include "header.h"

int isValidUsrName(char username[16], struct clientDetails clientList[]);
void multicastMessage(struct sbcpMessage curMsg, struct clientDetails clientList[], int clientDesc, int maxNumOfClients);
void handleOfflineClient(struct sbcpMessage curMsg, struct clientDetails clientList[], int clientDesc);
void swap(struct clientDetails clientList[], int i, int j);

//fd_set to store the socket descriptors of the active connections with the server.
fd_set clientDescriptors;
fd_set allDescriptors;
int numOfClients;

int main(int argc, char *argv[]) {

	int sockId, newSocketId, port, maxNumOfClients, maxDescriptors;
	char *serverIpv4;
	struct sockaddr_in serverSockAddr, clientSockAddr;
	int lenOfSockaddr_in;
	struct hostent *host;
	int selectVal;
	numOfClients = 0;

	//check arguments
	if (argc != 4) {
        fprintf(stderr,"missing arguments, please use the form ./server <IP_add> <Port_num> <max_num_of_clients>\n");
        exit(1);
    }

	//Creating a TCP socket for the server.
	sockId = socket(AF_INET, SOCK_STREAM, 0);
	if(sockId == -1) {
		printf("ERROR: Fail to create socket\n");
		return -1;
	}
	printf("INFO: Socket created successfully\n");

	serverIpv4 = argv[1];
	port = atoi(argv[2]);
	maxNumOfClients = atoi(argv[3]);

	//client lists
	struct clientDetails clientList[maxNumOfClients];

	//Initializing the serverSockAddr structure
	bzero(&serverSockAddr, sizeof(serverSockAddr));
	serverSockAddr.sin_family = AF_INET;
	serverSockAddr.sin_port = htons(port);
	host = gethostbyname(serverIpv4);
	memcpy(&serverSockAddr.sin_addr.s_addr, host->h_addr, host->h_length);
	lenOfSockaddr_in = sizeof(clientSockAddr);

	//Binding the created socket to the IP address and port number provided
	if (bind(sockId, (struct sockaddr *) &serverSockAddr, sizeof(serverSockAddr)) == -1) {
		printf("ERROR: Unable to bind the socket to IPv4 = %s and Port = %d\n", serverIpv4, port);
		return -1;
	}

	printf("INFO: Binding successfully\n");

	//Listening on the socket with the maximum clients provided
	if(listen(sockId, maxNumOfClients) == -1) {
		printf("ERROR: Fail to listen because the queue is full\n");
		return -1;
	}

	printf("INFO: Listenning at IP: %s, Port: %d...\n", serverIpv4, port);

	//clear the sets
	FD_ZERO(&clientDescriptors);
	FD_ZERO(&allDescriptors);

	FD_SET(sockId, &allDescriptors);
	maxDescriptors = sockId;
	printf("the # of descriptors are: %d\n", maxDescriptors);

	while(1) {
		clientDescriptors = allDescriptors;

		//If select succeeds, it returns the number of ready socket descriptors.
		selectVal = select(maxDescriptors + 1, &clientDescriptors, NULL, NULL, NULL);

		if(selectVal == -1) {
			printf("ERROR: Select failed for input descriptors.\n");
			return -1;
		}

		for(int i = 0; i <= maxDescriptors; i++) {
			//Check which descriptor is in the set.
			if(FD_ISSET(i, &clientDescriptors)) {

				//client connecting
				if(i == sockId) {

					//Accept the new connection coming to the server's socket descriptor.
					newSocketId = accept(sockId, (struct sockaddr *) &clientSockAddr, (socklen_t *) &lenOfSockaddr_in);

					if(newSocketId == -1) {

						printf("ERROR: Fail to connect to the client.\n");

					} else {

						//add the client desc to set
						FD_SET(newSocketId, &allDescriptors);

						//uodate the value of maximum descriptor
				        if(newSocketId >= maxDescriptors) {
							maxDescriptors = newSocketId;
						}

						printf("INFO: Established connection with the client.\n");
					}


				} else { //client sending
					struct sbcpMessage curMsg;

					//Read the message sent by the client.
					int numofbytes = read(i, (struct sbcpMessage *) &curMsg, sizeof(curMsg));

					if(numofbytes > 0) {
						//Process the received message based on the type of message being sent.
						multicastMessage(curMsg, clientList, i, maxNumOfClients);
					} else {
						//Handle resources when a client disconnects from the server.
						handleOfflineClient(curMsg, clientList, i);
					}
				}
			}
		}
	}
	close(sockId);
}

void handleSuccessfulJoin(struct sbcpMessage curMsg, struct clientDetails clientList[], int clientDesc, int maxNumOfClients, struct sbcpMessage *output_message) {
	struct clientDetails client;
	char onlineNotification[512];
	char concat_string[512] = {0};
	printf("The string form of users should be %s\n", concat_string);

	//String version of users in the chat room
	for(int j = 0; j < numOfClients; j++) {

		// printf("%s\n", clientList[j].username);

		strcat(concat_string, clientList[j].username);
		strcat(concat_string,"\t");
	}

	//create the current client variable
	strcpy(client.username, curMsg.attributes[0].payload);
	client.socket_desc = clientDesc;
	clientList[numOfClients++] = client;

	//Send ACK to the client
	output_message = (struct sbcpMessage*) malloc(sizeof(struct sbcpMessage));

	printf("The string form of users should be %s\n", concat_string);

	createAckMessage(&output_message, concat_string, curMsg.attributes[0].payload, numOfClients);
	printf("INFO: Sending ACK response for user: %s\n", curMsg.attributes[0].payload);

	write(clientDesc, (void *) output_message, sizeof(*output_message));
	printf("INFO: %s has joined.\n", curMsg.attributes[0].payload);

	//Create a FWD to all other clients that this new client is online
	strcpy(onlineNotification, curMsg.attributes[0].payload);
	strcat(onlineNotification," is joining in the chat room.");

	createOnlineMessage(&output_message, onlineNotification);

	for(int j = 0; j < numOfClients; j++) {
		if(clientList[j].socket_desc != clientDesc) {

			if(write(clientList[j].socket_desc, (void *) output_message, sizeof(*output_message)) == -1) {
				printf("ERROR: Fail multicast ONLINE message.\n");
			}
		}
	}


}

//muticast message to all clients except the one who sends the message
void multicastMessage(struct sbcpMessage curMsg, struct clientDetails clientList[], int clientDesc, int maxNumOfClients) {
	char *reason;
	struct sbcpMessage *output_message;
	char concat_string[512], message_string[512];
	int method;

	method = curMsg.type;
	if (method == JOIN) {


		//check duplicate username or burst out of range
		if(isValidUsrName(curMsg.attributes[0].payload, clientList) && numOfClients < maxNumOfClients) {

			handleSuccessfulJoin(curMsg, clientList, clientDesc, maxNumOfClients, output_message);

		} else {

			//Username duplicates or num of clients out of range
			if(numOfClients >= maxNumOfClients) {
				reason = MAX_CLIENTS_REACHED;
			} else {
				reason = INVALID_USERNAME;
			}

			output_message = (struct sbcpMessage*) malloc(sizeof(struct sbcpMessage));
			createNakMessage(&output_message, reason);
			printf("INFO: Sending NAK response for user: %s\n", curMsg.attributes[0].payload);
			write(clientDesc, (void *) output_message, sizeof(*output_message));
			FD_CLR(clientDesc, &allDescriptors);
		}


	} else if (method == SEND) {


		strcpy(message_string, curMsg.attributes[1].payload);
		strcpy(concat_string, curMsg.attributes[0].payload);
		strcat(concat_string, ": ");
		strcat(concat_string, message_string);

		output_message = (struct sbcpMessage*)malloc(sizeof(struct sbcpMessage));
		createFwdMessage(&output_message, concat_string);
		printf("INFO: Sending broadcast for user: %s\n", curMsg.attributes[0].payload);

		for(int j = 0; j < numOfClients; j++) {
			if(clientList[j].socket_desc != clientDesc) {
				if(write(clientList[j].socket_desc, (void *) output_message, sizeof(*output_message)) == -1) {
					printf("ERROR: Write error on broadcasting\n");
				}
			}
		}


	}
}

//check if the username is within the client list
int isValidUsrName(char username[16], struct clientDetails clientList[]) {
	for(int i = 0; i < numOfClients; i++) {
		if(strcmp(clientList[i].username, username) == 0) {
			printf("INFO: Duplicate Username: %s\n", username);
			return 0;
		}
	}
	return 1;
}



//handle the case when a user is offline
void handleOfflineClient(struct sbcpMessage curMsg, struct clientDetails clientList[], int clientDesc) {
	int index = -1;
	char username[16], offlineMessage[512];
	struct sbcpMessage *outputMessage;

	//find the user who is leaving the chat room
	for(int i = 0; i < numOfClients; i++) {
		if(clientList[i].socket_desc == clientDesc) {
			index = i;
			strcpy(username, clientList[i].username);
			break;
		}
	}

	//Clears the socket descriptor of the client from the fd_set and sends the FWD message to all clients indicating that the user is OFFLINE.
	if (index == -1) {
		printf("ERROR: Could not find client username in client list.\n");
	} else {
		printf("INFO: %s has disconnected from the server\n", username);
		FD_CLR(clientDesc, &allDescriptors);

		//swap the element between the leaving user and the last element, and then decrease the counter
		swap(clientList, index, numOfClients - 1);

		numOfClients--;


		//generate the broadcasting message
		strcpy(offlineMessage, username);
		strcat(offlineMessage, " is Offline.\n");
		outputMessage = (struct sbcpMessage*) malloc(sizeof(struct sbcpMessage));
		createOfflineMessage(&outputMessage, offlineMessage);
		printf("INFO: Sending offline broadcast for user: %s\n", username);


		//muticast the info to all rest users
		for(int j = 0; j < numOfClients; j++) {

			if(write(clientList[j].socket_desc, (void *) outputMessage, sizeof(*outputMessage)) == -1) {

				printf("ERROR: Write error on broadcasting.\n");

			}

		}


	}
}

//switch two elements in an array
void swap(struct clientDetails clientList[], int i, int j) {
	struct clientDetails temp = clientList[i];
	clientList[i] = clientList[j];
	clientList[j] = temp;
}
