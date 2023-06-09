
#define BCAST_ADDR 100
#define PAYLOAD_MAX 100
#define STRING_MAX 100
#define NAME_LENGTH 100

enum NetNodeType { /* Types of network nodes */
	HOST,
	SWITCH
};

enum NetLinkType { /* Types of linkls */
	PIPE,
	SOCKET
};

struct net_node { /* Network node, e.g., host or switch */
	enum NetNodeType type;
	int id;
	struct net_node *next;
};

struct net_port { /* port to communicate with another node */
	enum NetLinkType type;
	int pipe_host_id;
	int pipe_send_fd;
	int pipe_recv_fd;
	struct net_port *next;
   int sock_host_id;
};

/* Packet sent between nodes  */

struct packet { /* struct for a packet */
	char src;
	char dst;
	char type;
	int length;
	char payload[PAYLOAD_MAX];
};

/* Types of packets */

#define PKT_PING_REQ		0
#define PKT_PING_REPLY		1
#define PKT_FILE_UPLOAD_START	2
#define PKT_FILE_UPLOAD_CONT 3
#define PKT_FILE_UPLOAD_END 4
#define PKT_FILE_DOWNLOAD_SEND 5
#define PKT_FILE_DOWNLOAD_RECV 6
#define PKT_GET_ID_P 7
#define PKT_GET_ID_D 8
#define PKT_SET_DOMAIN 9
#define PKT_ID_P 10
#define PKT_ID_D 11
