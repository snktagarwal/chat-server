
/*  Client for the chat client server 

    Copyright (C) <2010>  <Sanket Agarwal>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include  <netdb.h>
#include  <sys/types.h>
#include  <sys/socket.h>
#include  <netinet/in.h>
#include  <arpa/inet.h>
#include  <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>

/* local includes */
#include "serv-client.h"

#define port 5692
#define ip "127.0.0.1"

int sockint;
char username[100];
int login_status = 0;
int valid_download = 1;
pthread_t thread1;
pthread_t thread2;

void ex_program(int sig){
	
	/* Caught signal, exit */

	printf("Caught exit signal, exiting gracefully... closing socket!\n");
	close(sockint);
	exit(0);
}

void accept_im_req(Serv_Msg *s_m){
	
	/* Accept an incoming connection request */

	printf("Do we want to accept the incoming connection ?\n");
}


void file_transfer_recv(Serv_Msg *s_m){

	/* Accepts the whole file */

	/* Checks if there is an existing copy of the same file, say for existing user */

	struct stat *f_stat = (struct stat *)malloc(sizeof(struct stat));
	FILE *fp;
	/* This function is entered on the first packet received */
	char output_filename[100];
	output_filename[0] = '\0';	
	strcpy(output_filename, username);
	strcat(output_filename, "/");
	strcat(output_filename, s_m->fx.file_name);
	
	if(s_m->fx.pkt_no==0){
		struct stat *s = (struct stat *)malloc(sizeof(struct stat));
		stat(output_filename, s);
		if(s->st_size > 0){
			printf("File exists, overwriting....\n");
		}
		printf("Accepting file %s\n", s_m->fx.file_name);
		fp = fopen(output_filename, "w");	/* Open the file stream for a write/append */
	}
	else fp = fopen(output_filename, "a+");

	fwrite(s_m->fx.data, sizeof(char), s_m->fx.pkt_data, fp);
	fclose(fp);
}

void create_xfer_dir(char *dir_name){
	
	struct stat *s = (struct stat *)malloc(sizeof(struct stat));
	char cmd[100];
	stat(dir_name, s);
	if(s->st_size != 0){
		
		printf("You already have a downloads folder, enjoy!\n");
	}

	else{
		sprintf(cmd, "mkdir %s", dir_name);
		system(cmd);
		printf("A directory successfully created for downloads!\n");
	}
}
		

void receive_data()                           // to receive the data
{
		/* Read the welcome message from the server first */
		int n;	
        Serv_Msg *s_m = (Serv_Msg *)malloc(sizeof(Serv_Msg));
		Cl_Msg *c_m = (Cl_Msg *)malloc(sizeof(Cl_Msg));

			while(1)
				if( ( n = recv( sockint, s_m, sizeof(Serv_Msg),0)) !=-1 && n != 0)
				{
					/* Identify the type of the message */
					switch(s_m->type){
						case WM:
							printf("Welcome Message from server: %s\n", s_m->wm);
							break;
						case M:
							printf("Message from Server: %s\n", s_m->m);
							break;
						case LOGIN_RES:
							switch(s_m->l_r){
								
								case NEW_CREATED:
									printf("A new account was created and you are logged in\n");
									create_xfer_dir(s_m->m);
									login_status = 1;
									break;
								case SIGNED_IN_OR_PASS:
									printf("Incorrect password or already signed in\n");
									break;
								case OLD_EXIST:
									printf("New user cannot be registered");
									break;
								case OLD_SIGNED_IN:
									printf("You have been signed in!\n");
									create_xfer_dir(s_m->m);
									login_status = 1;
									break;
								case DO_NOT_EXIST:
									printf("You do not exist on the server\n");
							}
							break;
						case BL_RES:
							/* Obtained the buddy list, print it */

							printf("Buddy list from server:\n");
							int i;
							for(i=0;i<s_m->b_l_tos;i++)
								printf("%d: %s\n", i+1, s_m->b_l[i].u_name);
							break;
#if 0
					case IM_REQ_CL:
							/* Choose whether to accept or deny the IM request */
							char opt[2];
							while(1){
								printf("Do you want to accept the incomming IM req from, %s ?[Y/N]\n", s_m->cr.u_name);
								scanf("%s", opt);
								if(opt[0] != 'Y' && opt[0] != 'N') continue;
								else if(opt[0] == 'Y'){
										c_m->type = IM_ACK_CL;
										c_m->cr = s_m->cr;
										send(sockint,type = IM_ACK_CL;
										c_m->cr = s_m->cr;
										send(sockint, c_m, sizeof(Cl_Msg), 0);
								}
							}
							break;
#endif
						case CHAT_ROOM_CREAT:
							printf("From server: %s",s_m->m);
							break;

						case CHAT_ROOM_JOINED:
							printf("From Message: %s", s_m->m);
							break;
						
						case CHAT_MSG:
							printf("Chat: %s\n", s_m->cm.msg);
							break;

						case IM_START_CONF:
							accept_im_req(s_m);
							break;

						case IM_RECV:
							printf("IM from (%s): %s\n", s_m->im.cr.u_name, s_m->im.msg);
							break;

						case FILE_XFER_RECV:
							
							file_transfer_recv(s_m);
							break;

						case SIGN_OUT_CONF:
							printf("Server says: %s", s_m->m);
							login_status=0;
							break;

						case ERR:
							printf("ERROR: %s", s_m->m);
							if(!strcmp(s_m->m, "The user  is offline!\n")) valid_download=0;
							if(!strcmp(s_m->m, "The user  does not exist\n")) valid_download=0;
							break;

					}
				}
				else
				{
					perror("Server has perhaps quit! Exiting :(\n");
					close(sockint);
					exit(1);
				}
}




void logon(Cl_Msg *c_m, Cred *cr){

	if(login_status) return;
	/* Logs a person on */
		char reg[2];
		while(1){
			printf("Are you a registered user ?[Y/N]");
			scanf("%s", reg);
			if(reg[0] != 'Y' && reg[0] != 'N'){
				getchar();
				continue;
			}
			if(reg[0] == 'Y'){
				
				/* If the user is regstered then ask for his creds and send to server */
				
				printf("Enter your [old]credentials\n");
				printf("Exiting user name[%d]: ",UN_MAX);
				scanf("%s", cr->u_name);
				strcpy(username, cr->u_name);
				printf("Existing password[%d]: ", PASS_MAX);
				scanf("%s", cr->pass);

				/* Write the credentials to the server */

				c_m->cr = *cr;
				c_m->type = EXIST_USER;
				send(sockint, c_m, sizeof(Cl_Msg), 0);
			}
			else{
				
				/* If not registered */
				printf("Enter your [new]credentials\n");
				printf("User name[%d]: ",UN_MAX);
				scanf("%s", cr->u_name);
				printf("Password[%d]: ", PASS_MAX);
				scanf("%s", cr->pass);

				/* Write the credentials to the server */

				c_m->cr = *cr;
				c_m->type = NEW_USER;
				send(sockint, c_m, sizeof(Cl_Msg), 0);
			}
			/* Sleep for the server to identify what we are upto */
			sleep(2);
			/* Check what server said */
			if(login_status)
				break;
		}

}

void im_buddy_req(Cl_Msg *c_m){
	
	/* Requests an IM with the buddy */
	Cred cr;

	printf("Enter the username: ");
	scanf("%s", cr.u_name);
	strcpy(username, cr.u_name);

	c_m->cr = cr;
	c_m->type = IM_REQ_CL;
}

void start_chat_room(Cl_Msg *c_m){	
	
	/* Creates new room on server */

	c_m->type = CHAT_ROOM_CREAT_REQ_CL;
}
	
void join_chat_room(Cl_Msg *c_m){
	
	/* Joins a chat room */

	c_m->type = CHAT_ROOM_JOIN_REQ_CL;

	printf("Please enter a valid chat room name: ");
	scanf("%s", c_m->chat_room_name);
}

void send_chat_message(Cl_Msg *c_m){
	
	/* Input a channel */
	
	c_m->type = CHAT_ROOM_SEND_MSG;

	printf("Chat room to enter: ");
	scanf("%s", c_m->chat_room_name);

	printf("Type \'q\' to exit chat room\n");
	getchar();
	while(1){

		printf("[%s]>> ", c_m->chat_room_name);
		gets(c_m->cm.msg);
		if(!strcmp(c_m->cm.msg, "q")){
			
			show_menu();
			return;
		}
		send(sockint, c_m, sizeof(Cl_Msg), 0);
	}

}



void file_transfer_send(Cl_Msg *c_m){
	
	/* File transfer protocol */

	char file_name[100];
	Cred cr;
	FILE *fp;
	int size, packets, i;
	struct stat *f_stat = (struct stat *)malloc(sizeof(struct stat));
	struct timespec *ts = (struct timespec *)malloc(sizeof(struct timespec));
	ts->tv_sec = 0;
	ts->tv_nsec = 100000000;
	
	printf("Enter username to transfer: ");
	scanf("%s", c_m->cr.u_name);

	printf("Enter the file name: ");
	scanf("%s", file_name);

	stat(file_name, f_stat);
	size = f_stat->st_size;

	if(!size){
		printf("Cannot open file for transfer\n");
		return;
	}
	
	valid_download = 1;	// assume all is well

	printf("The file to be transferred is %d bytes\n", size);

	fp = fopen(file_name, "r");

	/* We will have to form packets to send this file! */

	packets = size/PKT_SIZE;	// Last packet is size%PKT_SIZE
	
	c_m->type = FILE_XFER_SEND;
	for(i=0;i<packets;i++){
		c_m->fx.file_size = size;
		c_m->fx.pkt_data = PKT_SIZE;
		strcpy(c_m->fx.file_name, file_name);
		c_m->fx.pkt_no = i;
		fread(c_m->fx.data, sizeof(char), PKT_SIZE, fp);
		send(sockint, c_m, sizeof(Cl_Msg), 0);
		nanosleep(ts, NULL);
		if(!valid_download){
			fclose(fp);
			return;
		}
	}

	if(size%PKT_SIZE>0){
		c_m->fx.file_size = size;
		c_m->fx.pkt_data = size%PKT_SIZE;
		strcpy(c_m->fx.file_name, file_name);
		c_m->fx.pkt_no = i;
		fread(c_m->fx.data, sizeof(char), size%PKT_SIZE, fp);
		send(sockint, c_m, sizeof(Cl_Msg), 0);
	}

	
}




void send_im(Cl_Msg *c_m){	
	
	/* Send request for an IM */

	c_m->type = IM_SEND;
	getchar();
	printf("Input the name of the user: ");
	gets(c_m->im.cr.u_name);
	printf("Message[%d]: ", MSG_MAX);
	gets(c_m->im.msg);
}

void send_sign_out(Cl_Msg *c_m){
	
	/* Send a sign out request */

	c_m->type = SIGN_OUT_CL;
}

void show_menu(){
	
	/* Shows a simple minded menu */

	printf("1. Get the buddy list from the server\n");
	printf("2. Start IM with buddy\n");
	printf("3. Start a new chat room\n");
	printf("4. Join a chat room\n");
	printf("5. Send chat message\n");
	printf("6. File xfer\n");
	printf("7. Sign out\n");
	printf("-----------------xxxxxx-----------------\n");
}

void send_data()                                          // to send the data 
{
       	Cl_Msg *c_m=(Cl_Msg *)malloc(sizeof(Cl_Msg));
		char opt[2];
		Cred *cr = (Cred *)malloc(sizeof(Cred));

		/* Let the server send the welcome message */
		sleep(2);
		/* The first thing that needs to be done is to tell client to register */
		while(1){
		
			sleep(1);
			
			/* Checks if the user is signed out or not */
			logon(c_m, cr);
		
			/* Show the menu for today... chef */

			show_menu();

			printf(">> ");
			scanf("%s", opt);

			switch(opt[0]){
				
				case '1':
					
					/* Get the buddy list from the server */
					c_m->type = GET_BL;
					break;
				case '2':
					/* Start a new Instant Message */
					send_im(c_m);
					break;	
				case '3':
					/* Start a new chat room on the server */
					start_chat_room(c_m);
					break;
				case '4':
					/* Join a chat room */
					join_chat_room(c_m);
					break;
				case '5':
					/* Send chat message */
					send_chat_message(c_m);
					break;
				case '6':
					/* File transfer */
					file_transfer_send(c_m);
					break;
				case '7':
					/* Sign out the user */
					send_sign_out(c_m);
					break;
				default:
					printf("Feature to be implemented\n");
			}
			if(opt[0]!='6' && opt[0]!='5') // file xfer already handled!
				send(sockint, c_m, sizeof(Cl_Msg), 0);
		
		}

}










void main(int argc, char*argv[])                      // usage ./client username
{
	sockint= socket(AF_INET,SOCK_STREAM,0);
	
	if(sockint==-1)
	{	perror("socket");
	        exit(0);
	 }
	 
	 char *aux="localhost" ;
	
	struct hostent *h;
	
	if ((h=gethostbyname(aux)) == NULL) {
    		herror("gethostbyname");
    		exit(1);
	}

	printf("Host name : %s\n", h->h_name);
        printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *)h->h_addr)));	
		
	struct sockaddr_in socket_details;
	socket_details.sin_family=AF_INET;
	socket_details.sin_port= htons( port);            // to be on the safer side they are converted to network byte order
	socket_details.sin_addr.s_addr=inet_addr(ip);
	bzero( &(socket_details.sin_zero),8);
	
	// my_addr.sin_port = 0; /* choose an unused port at random */
	// my_addr.sin_addr.s_addr = INADDR_ANY; /* use my IP address */	
	
	if( connect( sockint, (struct sockaddr*)&socket_details, sizeof(struct sockaddr) ) ==-1)
		printf("Error man");
	else
		printf("Connected to the server on the port %d\n",port);	
	
	/* In case we grab a sigint close the socket and exit */
	signal(SIGINT, ex_program);

       	pthread_create(&thread1, NULL, receive_data,NULL);              //It must be passed by reference as a pointer cast of type void   
	
       	pthread_create(&thread2, NULL, send_data, NULL);              //It must be passed by reference as a pointer cast of type void   
	
	void *aux1;
	void *aux2;
	pthread_join(thread1, &aux1);              //waiting for these threads to  complete and join with the original thread and aux are for status check , see documentation for more
	pthread_join(thread2, &aux2);
	 
    close(sockint);                 // when done close the socket
}
