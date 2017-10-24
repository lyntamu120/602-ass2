#include <stdio.h>
#include <stdlib.h>

//Constant fields
#define INVALID_USERNAME "The provided username already exists."
#define MAX_CLIENTS_REACHED "The server has reached the maximum number of clients it can handle"

//basic features
#define JOIN 	2
#define FWD 	3
#define SEND 	4


//bonus features
#define NAK		5
#define OFFLINE 6
#define ACK		7
#define ONLINE	8
#define IDLE	9


//SBCP attribute
struct sbcpAttributes {
  int type :16;
  int payloadlength :16;
  char payload[512];
};


//SBCP message
struct sbcpMessage {
  int version :9;
  char type :7;
  int length :16;
  struct sbcpAttributes attributes[4];
};


//information of the client
struct clientDetails {
  char username[16];
  int socket_desc;
};



//a client creates a join message
void createJoinMessage(struct sbcpMessage **sbcp_message, char username[16]) {
	(*sbcp_message)->version = 3;
	(*sbcp_message)->length = 1;
	(*sbcp_message)->type = 2;
	(*sbcp_message)->attributes[0].type = 2;
	(*sbcp_message)->attributes[0].payloadlength = strlen(username);
	strcpy((*sbcp_message)->attributes[0].payload, username);
}



// The server creates an nak message if the client can't join in the chat room
void createNakMessage(struct sbcpMessage **sbcp_message, char *reason) {
	(*sbcp_message)->version = 3;
	(*sbcp_message)->length = 1;
    (*sbcp_message)->type = 5;
    (*sbcp_message)->attributes[0].type = 1;
	(*sbcp_message)->attributes[0].payloadlength = strlen(reason);
	strcpy((*sbcp_message)->attributes[0].payload, reason);
}



//The server sends to the user a ACK if join successfully
void createAckMessage(struct sbcpMessage **sbcp_message, char *string, char username[16], int client_count) {
	(*sbcp_message)->version = 3;
	(*sbcp_message)->type = 7;
	(*sbcp_message)->length = 3;

    (*sbcp_message)->attributes[0].type = 2;
	(*sbcp_message)->attributes[0].payloadlength = strlen(username);
	strcpy((*sbcp_message)->attributes[0].payload,username);

	(*sbcp_message)->attributes[1].type = 3;
	char str[15];
	sprintf(str, "%d", client_count);
	(*sbcp_message)->attributes[1].payloadlength = strlen(str);
	strcpy((*sbcp_message)->attributes[1].payload,str);

	(*sbcp_message)->attributes[2].type = 4;
	(*sbcp_message)->attributes[2].payloadlength = strlen(string);
	strcpy((*sbcp_message)->attributes[2].payload,string);
}



// Sends method
void createSendMessage(struct sbcpMessage **sbcp_message, char username[16], char input_buffer[512]) {
	(*sbcp_message)->version = 3;
	(*sbcp_message)->type = 4;
	(*sbcp_message)->length = 2;
	(*sbcp_message)->attributes[0].type = 2;
	(*sbcp_message)->attributes[0].payloadlength = strlen(username);
	strcpy((*sbcp_message)->attributes[0].payload,username);

	(*sbcp_message)->attributes[1].type = 4;
	(*sbcp_message)->attributes[1].payloadlength = strlen(input_buffer);
	strcpy((*sbcp_message)->attributes[1].payload, input_buffer);
}



// Server sends online message if some client is connected
void createOnlineMessage(struct sbcpMessage **sbcp_message, char message[512]) {
	(*sbcp_message)->version = 3;
	(*sbcp_message)->type = 8;
	(*sbcp_message)->length = 1;
	(*sbcp_message)->attributes[0].type = 4;
	(*sbcp_message)->attributes[0].payloadlength = strlen(message);
	strcpy((*sbcp_message)->attributes[0].payload,message);
}



//Server multicast its receiving message
void createFwdMessage(struct sbcpMessage **sbcp_message, char message[512]) {
	(*sbcp_message)->version = 3;
	(*sbcp_message)->type = 3;
	(*sbcp_message)->length = 1;
	(*sbcp_message)->attributes[0].type = 4;
	(*sbcp_message)->attributes[0].payloadlength = strlen(message);
	strcpy((*sbcp_message)->attributes[0].payload,message);
}



//Server creates offline method if some client is leaving the chatting room
void createOfflineMessage(struct sbcpMessage **sbcp_message, char message[512]) {
	(*sbcp_message)->version = 3;
	(*sbcp_message)->type = 6;
	(*sbcp_message)->length = 1;
	(*sbcp_message)->attributes[0].type = 4;
	(*sbcp_message)->attributes[0].payloadlength = strlen(message);
	strcpy((*sbcp_message)->attributes[0].payload,message);
}
