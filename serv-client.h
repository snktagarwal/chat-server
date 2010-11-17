/* Defines the common header types for both the server and the client */

#define WM_MAX	500	// Welcome message size
#define M_MAX   500	// General message size
#define UN_MAX  20	// Username maxsize
#define PASS_MAX 20	// Password maximum size
#define CHAT_MAX 20	// Maximum users in a room
#define CHAT_ROOM_MAX 20	// Maximum chat rooms
#define BUDDIES_MAX 100
#define MSG_MAX 500
#define PKT_SIZE 1024			// Size in bytes

#define CRED_FN "uname.dat"

void show_menu();

enum BOOLEAN{
	
	TRUE,
	FALSE
};

enum LOGIN_RESULT{
	NEW_CREATED,
	SIGNED_IN_OR_PASS,
	OLD_EXIST,
	OLD_SIGNED_IN,
	DO_NOT_EXIST
};

enum STATUS{
	SIGNED_IN,
	SIGNED_OUT
};

enum SERV_TYPE{
	
	WM,		/* Welcome message */
	M,		/* General Message */
	LOGIN_RES,	/* Login status */
	BL_RES,		/* Sends the buddy list */
	LOGIN,	/* Login Process */
	CM,		/* Chat Message */
	IM_REQ_SERV,		/* Instant Message */
	CHAT_ROOM_CREAT,	/* Start a new chat room on server */
	CHAT_ROOM_JOINED,	/* Acceptance to join chat room */
	CHAT_MSG,			/* Actual chat message packet */
	ERR,				/* Any error message */
	IM_START_CONF,		/* Confirms the client to start the IM */
	SIGN_OUT_CONF,		/* Confirms the sign out */
	IM_RECV,				/* Client receives an IM */
	FILE_XFER_RECV		/* File received by  a client */
};

enum CLIENT_TYPE{
	EXIST_USER,	/* Exist user logon */
	NEW_USER,	/* New user logon */
	GET_BL,		/* Buddy List on server */
	IM_REQ_CL,	/* Request a client to start IM */
	IM_ACK_CL,	/* Acknowledge a IM request */
	CHAT_ROOM_CREAT_REQ_CL,	/* Request a chat room start */
	CHAT_ROOM_JOIN_REQ_CL,	/* Join a chat room */
	CHAT_ROOM_SEND_MSG,		/* Broadcasts the message to all the clients in that room */
	IM_START_REQ,			/* Requests a start of IM stanza */
	SIGN_OUT_CL,			/* Signs the client out */
	IM_SEND,				/* Sends an IM */
	FILE_XFER_SEND			/* File xfer protocol */

};

typedef struct _cred{
	
	char u_name[UN_MAX];
	char pass[PASS_MAX];
	enum STATUS status;
	int socket_id;
}Cred;

typedef struct _c_i_m{
	Cred cr;
	char msg[MSG_MAX];
}IM;



typedef struct _buddy_list{
	
	Cred buddies[BUDDIES_MAX];
	int tos;
}Buddy_List;

typedef struct _chat_message{
	char msg[MSG_MAX];
}ChatMessage;

typedef struct _fx{
	
	char *data[PKT_SIZE];
	char file_name[100];
	int pkt_no;
	int pkt_data;
	int file_size;
}FX;

typedef struct _serv_msg{
	char wm[WM_MAX];
	char m[M_MAX];
	Cred b_l[BUDDIES_MAX];
	ChatMessage cm;
	IM im;
	int b_l_tos;
	enum SERV_TYPE type;
	enum LOGIN_RESULT l_r;
	FX fx;
}Serv_Msg;

typedef struct _client_msg{
	Cred cr;
	char chat_room_name[UN_MAX+10];
	ChatMessage cm;
	IM im;
	enum CLIENT_TYPE type;
	FX fx;
}Cl_Msg;

typedef struct _chat_room{
	char name[UN_MAX+10];
	Cred users[CHAT_MAX];
	int no_users;
}ChatRoom;

typedef struct _chat_rooms{
	ChatRoom chat_room[CHAT_ROOM_MAX];
	int no_rooms;
}ChatRooms;

typedef struct _s_i_m{
	Cred cr1;
	Cred cr2;
	int cr1_ent;
	int cr2_ent;
}ServIM;

typedef struct _i_ms{
	
	ServIM im_arr[CHAT_ROOM_MAX];
	int tos;
}InstantMsgs;
