/* 
 * host.h 
 */

#include <stdio.h>

#define MAX_FILE_BUFFER 1000
#define MAX_MSG_LENGTH 100
#define MAX_DIR_NAME 100
#define MAX_FILE_NAME 100
#define PKT_PAYLOAD_MAX 100
#define TENMILLISEC 10000   /* 10 millisecond sleep */
#define DNS_SERVER_ID 100   /* DNS Servers hostID */
#define MAX_NAME_LENGTH 50  /* Max length of domain names */
#define TABLE_SIZE 256

struct file_buf {
   char name[MAX_FILE_NAME];
   int name_length;
   char buffer[MAX_FILE_BUFFER+1];
   int head;
   int tail;
   int occ;
   FILE *fd;
};


enum host_job_type {
	JOB_SEND_PKT_ALL_PORTS,
	JOB_PING_SEND_REQ,	
	JOB_PING_SEND_REPLY,
	JOB_PING_WAIT_FOR_REPLY,
	JOB_FILE_DOWNLOAD_SEND,
   JOB_FILE_DOWNLOAD_RECV,
   JOB_FILE_UPLOAD_SEND,
	JOB_FILE_UPLOAD_RECV_START,
	JOB_FILE_UPLOAD_RECV_CONT,
   JOB_FILE_UPLOAD_RECV_END,
   JOB_RECV_GET_ID_P,
   JOB_RECV_GET_ID_D,
   JOB_SET_DOMAIN
};

struct host_job {
	enum host_job_type type;
	struct packet *packet;
	int in_port_index;
	int out_port_index;
	char fname_download[100];
	char fname_upload[100];
	int ping_timer;
	int file_upload_dst;
	int file_download_dst;
   struct host_job *next;
};


struct job_queue {
	struct host_job *head;
	struct host_job *tail;
	int occ;
};

void host_main(int host_id);
void dns_main(int host_id);
void display_packet_info(struct packet *);
void display_host_job_info(struct host_job*, int);
