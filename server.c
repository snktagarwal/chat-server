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


/* Local includes */
#include "serv-client.h"

#define port 5692
#define ip "127.0.0.1"
#define admin "admin"

int *client_list;
ChatRooms *chat_rooms;
InstantMsgs *ims;
Buddy_List *bl;
int sockint;
int j=0;


void ex_program(int sig){
	
	/* Caught signal, exit */

	printf("Exiting the server\n");
	close(sockint);
	exit(1);
}

int is_exist_buddy(Buddy_List *bl, Cred cr){
	
	int i;
	
	for(i=0;i<bl->tos;i++)
		if(!strcmp(cr.u_name, bl->buddies[i].u_name)) return 1;
	return 0;
}

Cred *find_cr_by_name(Buddy_List *bl, char *name){

	int i;

	for(i=0;i<bl->tos;i++)
		if(!strcmp(name, bl->buddies[i].u_name)) return &bl->buddies[i];
	return NULL;
}



int add_buddy(Buddy_List *bl, Cred cr, int socket_id){
	
	FILE *fp;
	/* First check if the buddy is already in the list */
	if(is_exist_buddy(bl, cr)){
		return 0;
	}

	else{
		bl->buddies[bl->tos] = cr;
		bl->buddies[bl->tos].socket_id = socket_id;	// Store the socket id's as well
		bl->tos++;
		fp = fopen(CRED_FN, "a");
		fprintf(fp, "\n%s", cr.u_name);
		fprintf(fp," ");
		fprintf(fp, "%s", cr.pass);
		fclose(fp);
		return 1;
	}

}


Cred find_cr_by_client_id(Buddy_List *bl, int sock_id){
	
	int i;
	Cred dummy;	
	for(i=0;i<bl->tos;i++)
		if(bl->buddies[i].socket_id == sock_id) return bl->buddies[i];
	dummy.u_name[0] = '\0';
	return dummy;
}

int sign_out_buddy(Buddy_List *bl, Cred cr){
	int i;
	for(i=0;i<bl->tos;i++)
		if(!strcmp(cr.u_name, bl->buddies[i].u_name)){
			bl->buddies[i].status = SIGNED_OUT;
			return 1;
		}
	return 0;
}
	

int sign_on_buddy(Buddy_List *bl, Cred cr, int sock_id){

	int i;
	
	for(i=0;i<bl->tos;i++)
		if(!strcmp(cr.u_name, bl->buddies[i].u_name)){
			bl->buddies[i].status = SIGNED_IN;
			bl->buddies[i].socket_id = sock_id;
			return 1;
		}
	return 0;
}

int is_signed_in(Buddy_List *bl, Cred cr){
	
	int i;
	
	for(i=0;i<bl->tos;i++)
		if(!strcmp(cr.u_name, bl->buddies[i].u_name) && bl->buddies[i].status == SIGNED_IN) return 1;
	return 0;
}
	

int check_cred(Buddy_List *bl, Cred cr){
	int i;
	
	for(i=0;i<bl->tos;i++)
		if(!strcmp(cr.u_name, bl->buddies[i].u_name) && !strcmp(cr.pass, bl->buddies[i].pass)) return 1;
	return 0;
}

void get_buddy_list(Buddy_List *bl, Serv_Msg *s_m){
	
	/* Populates the server message structure with the online buddies */

	int i;
	s_m->b_l_tos = 0;
	for(i=0;i<bl->tos;i++)
		if(bl->buddies[i].status == SIGNED_IN) s_m->b_l[s_m->b_l_tos++] = bl->buddies[i];
}


ChatRoom *create_chat_room(ChatRooms *chat_rooms, char *name){

	/* Create with name u_name */
	
	ChatRoom *c_r = (ChatRoom *)malloc(sizeof(ChatRoom));
	c_r->no_users=0;

	sprintf(c_r->name, "%s:%d", name, chat_rooms->no_rooms);	// store at top of stack
	chat_rooms->chat_room[chat_rooms->no_rooms++] = *c_r;
	return c_r;
}

void remove_user_chatrooms(ChatRooms *chat_rooms, char *name){
	
	/* Check if name is in a room and remove him */

	int i, j;
	printf("Going to remove user %s from all chatrooms\n");

	for(i=0;i<chat_rooms->no_rooms;i++){
		
		ChatRoom c_r = chat_rooms->chat_room[i];

		for(j=0;j<c_r.no_users;j++){
			printf("Comparing %s against user %s\n", c_r.users[i].u_name, name);
			if(!strcmp(c_r.users[i].u_name, name))
				c_r.users[i].u_name[0] = '\0';
		}
	}
}


void add_cr_to_chat_room(ChatRoom *chat_room, Cred cr){
	
	chat_room->users[chat_room->no_users++] = cr;
}

ChatRoom *find_chat_room(ChatRooms *chat_rooms, char *chat_room_name){	
	
	/* Find the chat room with the specified name */

	int i;

	for(i=0;i<chat_rooms->no_rooms;i++)
		if(!strcmp(chat_room_name, chat_rooms->chat_room[i].name)) return &(chat_rooms->chat_room[i]);
	return NULL;
}

void print_chat_room(ChatRoom *chat_room){
	int i;
	printf("Chat room: %s\n", chat_room->name);
	for(i=0;i<chat_room->no_users;i++)
		printf("%s:%d\n", chat_room->users[i].u_name, chat_room->users[i].socket_id);
}

void broadcast_chat_msg(ChatRoom *chat_room, ChatMessage cm, Cred cr){
	
	/* Find all descripters and flush one by one */

	int i;
	print_chat_room(chat_room);
	Serv_Msg *s_m = (Serv_Msg *)malloc(sizeof(Serv_Msg));

	for(i=0;i<chat_room->no_users;i++){
		
		int sockid_cr = chat_room->users[i].socket_id;
		printf("Now sending to sockid: %d\n", sockid_cr);
		s_m->type = CHAT_MSG;
		s_m->cm = cm;
		if(strcmp(chat_room->users[i].u_name, cr.u_name) && is_signed_in(bl, chat_room->users[i]))
			send(sockid_cr, s_m, sizeof(Serv_Msg), 0);
	
	}
}

int is_user_added_chat_room(ChatRoom *chat_room, Cred cr){
	
	int i;
	for(i=0;i<chat_room->no_users;i++)
		if(!strcmp(cr.u_name, chat_room->users[i].u_name)) return 1;
	return 0;
}

void handle_client( void* addrnew_client)
{
	/* Handles each new client request */
	int new_client=(int)addrnew_client;
	char *buf=(char*)malloc(sizeof(char)*255);
	int n;
	
	/* First we send a welcome message of the server, just for information */

	Serv_Msg *s_m = (Serv_Msg *)malloc(sizeof(Serv_Msg));
	Cred cr;
	Cred *cr1, *cr_recv;
	Cl_Msg *c_m = (Cl_Msg *)malloc(sizeof(Cl_Msg));
	ChatRoom *chat_room;
	
	strcpy(s_m->wm, "Welcome to AASVAR Chat Server! Enjoy\nIdentify yourself to the server.");

	send(new_client, s_m, sizeof(Serv_Msg), 0);
	

	while(1){
		if( (n= recv( new_client, c_m, sizeof(Cl_Msg), 0)) !=-1 && n !=0)
		{
			cr = find_cr_by_client_id(bl, new_client);
			switch(c_m->type){
				
				/* Logon process */

				/* Existing user */
				case EXIST_USER:
					if(is_exist_buddy(bl, c_m->cr) && !is_signed_in(bl, c_m->cr) && check_cred(bl, c_m->cr)){
						sign_on_buddy(bl, c_m->cr, new_client);
						strcpy(s_m->m, c_m->cr.u_name);
						s_m->l_r = OLD_SIGNED_IN;
					}
					else if(is_exist_buddy(bl, c_m->cr)){
						s_m->l_r = SIGNED_IN_OR_PASS;
					}
					else s_m->l_r = DO_NOT_EXIST;
					s_m->type = LOGIN_RES;
					send(new_client, s_m, sizeof(Serv_Msg), 0);
					break;

				case NEW_USER:
					if(is_exist_buddy(bl, c_m->cr))
						s_m->l_r = OLD_EXIST;
					else{
						add_buddy(bl, c_m->cr, new_client);
						sign_on_buddy(bl, c_m->cr, new_client);
						strcpy(s_m->m, c_m->cr.u_name);
						s_m->l_r = NEW_CREATED;
					}
					s_m->type = LOGIN_RES;
					send(new_client, s_m, sizeof(Serv_Msg), 0);
					break;
				case GET_BL:
					/* Send the current buddy list with all signed on users */
					get_buddy_list(bl, s_m);
					s_m->type = BL_RES;
					send(new_client, s_m, sizeof(Serv_Msg), 0);
					break;
				case IM_REQ_CL:
					/* Tries to generate an IM b/w two connecting entities */
					/* Check again if the buddy exist in the queue */
					if(is_exist_buddy(bl, c_m->cr)){
						s_m->type = IM_REQ_SERV;
						cr = find_cr_by_client_id(bl, new_client);
						/* Send a packet to the receiver to ack the packet */
						send(new_client, s_m, sizeof(Serv_Msg), 0);
					}
					else printf("Buddy not found!");
					break;
				case IM_ACK_CL:
					break;
				case CHAT_ROOM_CREAT_REQ_CL:
					/* Starts a new chat room */
					chat_room = create_chat_room(chat_rooms, cr.u_name);
					add_cr_to_chat_room(chat_room, cr);
					s_m->type = CHAT_ROOM_CREAT;
					sprintf(s_m->m, "Chat room with name %s created!\n", chat_room->name);
					send(new_client, s_m, sizeof(Serv_Msg), 0);
					break;

				case CHAT_ROOM_JOIN_REQ_CL:
					/* Search the chat room with the specified name */
					chat_room = find_chat_room(chat_rooms, c_m->chat_room_name);
					if(chat_room != NULL){
						add_cr_to_chat_room(chat_room, cr);
						s_m->type = CHAT_ROOM_JOINED;
						sprintf(s_m->m, "Chat room %s joined!\n", chat_room->name);
					}
					else{
						s_m->type = ERR;
						sprintf(s_m->m, "No chat room with name %s found!\n", c_m->chat_room_name);
					}
					send(new_client, s_m, sizeof(Serv_Msg), 0);
					break;

				case CHAT_ROOM_SEND_MSG:
					/* Sends a message to all clients in the chat room */
					if((chat_room =find_chat_room(chat_rooms, c_m->chat_room_name))!=NULL&& is_user_added_chat_room(chat_room, cr))
						broadcast_chat_msg(chat_room, c_m->cm, cr);
					else if(chat_room == NULL){
						s_m->type = ERR;
						sprintf(s_m->m, "No chat room with name %s found!\n", c_m->chat_room_name);
						send(new_client, s_m, sizeof(Serv_Msg), 0);
					}
					else{
						s_m->type = ERR;
						sprintf(s_m->m, "You are not added!\n", c_m->chat_room_name);
						send(new_client, s_m, sizeof(Serv_Msg), 0);
					}
						
					break;
				case IM_START_REQ:
					/* Asks the other client if it is okay to start IM */
					cr1 = find_cr_by_name(bl, c_m->im.cr.u_name);
					if(cr1){
						s_m->type = IM_START_CONF;
						s_m->im.cr = find_cr_by_client_id(bl, new_client);
						send(cr1->socket_id, s_m, sizeof(Serv_Msg), 0);
					}
					else{
						s_m->type = ERR;
						sprintf(s_m->m, "The user %s does not exist!\n", c_m->im.cr.u_name);
						send(new_client, s_m, sizeof(Serv_Msg), 0);
					}
					break;

				case SIGN_OUT_CL:
					
					/* Locate the user */
					cr = find_cr_by_client_id(bl, new_client);
					sign_out_buddy(bl, cr);
					s_m->type = SIGN_OUT_CONF;
					sprintf(s_m->m, "You have been successfully signed out!\n");

					send(new_client, s_m, sizeof(Serv_Msg), 0);
					break;
				case IM_SEND:
					
					/* Locate the user requested */
					cr = find_cr_by_client_id(bl, new_client);
					cr_recv = find_cr_by_name(bl,c_m->im.cr.u_name);
					if(!cr_recv){
						/* User not found! */
						s_m->type = ERR;
						sprintf(s_m->m, "The user %s does not exist\n", c_m->im.cr.u_name);
						send(new_client, s_m, sizeof(Serv_Msg), 0);
					}
					else if(!is_signed_in(bl, *cr_recv)){
						s_m->type = ERR;
						sprintf(s_m->m, "The user %s is offline!\n", c_m->im.cr.u_name);
						send(new_client, s_m, sizeof(Serv_Msg), 0);
					}
					else{
						s_m->im = c_m->im;
						s_m->im.cr = cr; 
						s_m->type = IM_RECV;
						send(cr_recv->socket_id, s_m, sizeof(Serv_Msg), 0);
					}
					break;
				case FILE_XFER_SEND:
					/* Locate the user requested */

					cr_recv = find_cr_by_name(bl,c_m->cr.u_name);
					if(!cr_recv){
						/* User not found! */
						s_m->type = ERR;
						sprintf(s_m->m, "The user %s does not exist\n", c_m->im.cr.u_name);
						send(new_client, s_m, sizeof(Serv_Msg), 0);
					}
					else if(!is_signed_in(bl, *cr_recv)){
						s_m->type = ERR;
						sprintf(s_m->m, "The user %s is offline!\n", c_m->im.cr.u_name);
						send(new_client, s_m, sizeof(Serv_Msg), 0);
					}
					else{
						s_m->fx = c_m->fx;
						s_m->type = FILE_XFER_RECV;
						send(cr_recv->socket_id, s_m, sizeof(Serv_Msg), 0);
					}
					break;
					
			}
					


		}
		else if (n == 0){
			cr = find_cr_by_client_id(bl, new_client);
			printf("Client %s has quit: removing from list\n", cr.u_name);
			sign_out_buddy(bl, cr);
			s_m->type = SIGN_OUT_CONF;
	
			return;
		}
		else
		{	perror("cant get message, exiting\n");
				exit(1);
		}
	}
}





void allocate_global(void){
		
		int i;
		FILE *fp = fopen(CRED_FN, "r");
		char u_name[UN_MAX], u_pass[PASS_MAX];
		/* Allocate some of the global datastructures */

		bl = (Buddy_List *)malloc(sizeof(Buddy_List));
		bl->tos = 0;
		
		/* And load the buddy list */
		i=0;
		while(fscanf(fp, "%s", bl->buddies[i].u_name) != -1){
			fscanf(fp, "%s",  bl->buddies[i].pass);
			bl->buddies[i].status = SIGNED_OUT;
			i++;
			bl->tos++;
		}
		
		/* Close the file */
		fclose(fp);
		chat_rooms = (ChatRooms *)malloc(sizeof(ChatRooms));
		chat_rooms->no_rooms = 0;

		ims = (InstantMsgs *)malloc(sizeof(InstantMsgs));
		ims->tos=0;

		for(i=0;i<CHAT_ROOM_MAX;i++)
			ims->im_arr[i].cr1_ent = ims->im_arr[i].cr2_ent = 0;
}







void main()
{
		sockint= socket(AF_INET,SOCK_STREAM,0);  // creating the file descriptor or socket and the id is as integer
		if(sockint==-1)
		{	perror("cant create a socket\n");
				exit(0);
		}                       

		client_list=(int*)malloc(sizeof(int)*50);                 //stores the socket descriptor for all the clients
		int max_client=15, sin_size;                                    // max size is 15 clients

		struct sockaddr_in socket_details;
		socket_details.sin_family=AF_INET;
		socket_details.sin_port= htons( port);            // to be on the safer side they are converted to network byte order
		socket_details.sin_addr.s_addr=inet_addr(ip);
		bzero( &(socket_details.sin_zero),8);

		struct sockaddr client_addr;                     // to store the details of the client 

		// my_addr.sin_port = 0; /* choose an unused port at random */
		// my_addr.sin_addr.s_addr = INADDR_ANY; /* use my IP address */	

		if( bind(sockint, (struct sockaddr*)&socket_details, sizeof(struct sockaddr) ) !=-1)
				printf("Socket successfully binded \n");
		else
		{	perror("bind error");  
				exit(0);   
		}	

		if( listen(sockint, max_client)!=-1 )                   //maxclient is the number of client that can connect to ur server
				printf("Listening on the port %d and the socket descriptor is %d \n", port, sockint );
		else
		{
			perror("listen error");
			exit(1);
		}
		signal(SIGINT, ex_program);	
		allocate_global();
		j=0;
		while(1)                   //accepts the client request and sends receives the data waits indefinitely so that a real time server
		{
				sin_size=sizeof(struct sockaddr_in);
				int new_client=accept(sockint, (struct sockaddr *)&client_addr,&sin_size); 	 // this is to store the file descriptor for a new client

				client_list[j++]=new_client;
				printf("Got connection on port  %d \n", port);

				pthread_t thread1;	               // creating the thread for receving the data on the newly opened client socket
				pthread_create(&thread1, NULL, handle_client, (void *)new_client);              //It must be passed by reference as a pointer cast of type void    


		}
		printf("Exiting from here\n");
		close(sockint);


}
