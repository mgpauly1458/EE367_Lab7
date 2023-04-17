 /*
  * host.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "net.h"
#include "man.h"
#include "host.h"
#include "packet.h"
#include "switch.h"


void display_host_job_info(struct host_job *job, int hostid) {
  const char* job_type_str;

  switch (job->type) {
    case JOB_SEND_PKT_ALL_PORTS:
      job_type_str = "JOB_SEND_PKT_ALL_PORTS";
      break;
    case JOB_PING_SEND_REQ:
      job_type_str = "JOB_PING_SEND_REQ";
      break;
    case JOB_PING_SEND_REPLY:
      job_type_str = "JOB_PING_SEND_REPLY";
      break;
    case JOB_PING_WAIT_FOR_REPLY:
      job_type_str = "JOB_PING_WAIT_FOR_REPLY";
      break;
    case JOB_FILE_DOWNLOAD_SEND:
      job_type_str = "JOB_FILE_DOWNLOAD_SEND";
      break;
    case JOB_FILE_DOWNLOAD_RECV:
      job_type_str = "JOB_FILE_DOWNLOAD_RECV";
      break;
    case JOB_FILE_UPLOAD_SEND:
      job_type_str = "JOB_FILE_UPLOAD_SEND";
      break;
    case JOB_FILE_UPLOAD_RECV_START:
      job_type_str = "JOB_FILE_UPLOAD_RECV_START";
      break;
    case JOB_FILE_UPLOAD_RECV_CONT:
      job_type_str = "JOB_FILE_UPLOAD_RECV_CONT";
      break;
    case JOB_FILE_UPLOAD_RECV_END:
      job_type_str = "JOB_FILE_UPLOAD_RECV_END";
      break;
    default:
      job_type_str = "Unknown Job Type";
  }

  printf("\n\n\nHost Id: %d\n", hostid);
  printf("\nJob Type: %s\n", job_type_str);
  printf("Input Port Index: %d\n", job->in_port_index);
  printf("Output Port Index: %d\n", job->out_port_index);
  printf("Download Filename: %s\n", job->fname_download);
  printf("Upload Filename: %s\n", job->fname_upload);
  printf("Ping Timer: %d\n", job->ping_timer);
  printf("File Upload Destination: %d\n", job->file_upload_dst);
  printf("File Download Destination: %d\n", job->file_download_dst);
  printf("Next Job: %p\n", job->next); // assuming next is a pointer
   display_packet_info(job->packet);
   printf("\n\n\n");
}

/*
 * File buffer operations
 */

/* Initialize file buffer data structure */
void file_buf_init(struct file_buf *f)
{
f->head = 0;
f->tail = MAX_FILE_BUFFER;
f->occ = 0;
f->name_length = 0;
}

/* 
 * Get the file name in the file buffer and store it in name 
 * Terminate the string in name with tne null character
 */
void file_buf_get_name(struct file_buf *f, char name[])
{
int i;

for (i=0; i<f->name_length; i++) {
	name[i] = f->name[i];
}
name[f->name_length] = '\0';
}

/*
 *  Put name[] into the file name in the file buffer
 *  length = the length of name[]
 */
void file_buf_put_name(struct file_buf *f, char name[], int length)
{
int i;

for (i=0; i<length; i++) {
	f->name[i] = name[i];
}
f->name_length = length;
}

/*
 *  Add 'length' bytes n string[] to the file buffer
 */
int file_buf_add(struct file_buf *f, char string[], int length)
{
int i = 0;

while (i < length && f->occ < MAX_FILE_BUFFER) {
	f->tail = (f->tail + 1) % (MAX_FILE_BUFFER + 1);
	f->buffer[f->tail] = string[i];
	i++;
        f->occ++;
}
return(i);
}

/*
 *  Remove bytes from the file buffer and store it in string[] 
 *  The number of bytes is length.
 */
int file_buf_remove(struct file_buf *f, char string[], int length)
{
int i = 0;

while (i < length && f->occ > 0) {
	string[i] = f->buffer[f->head];
	f->head = (f->head + 1) % (MAX_FILE_BUFFER + 1);
	i++;
        f->occ--;
}

return(i);
}


/*
 * Operations with the manager
 */

int get_man_command(struct man_port_at_host *port, char msg[], char *c) {

int n;
int i;
int k;

n = read(port->recv_fd, msg, MAN_MSG_LENGTH); /* Get command from manager */
if (n>0) {  /* Remove the first char from "msg" */
	for (i=0; msg[i]==' ' && i<n; i++);
	*c = msg[i];
	i++;
	for (; msg[i]==' ' && i<n; i++);
	for (k=0; k+i<n; k++) {
		msg[k] = msg[k+i];
	}
	msg[k] = '\0';
}
return n;

}

/*
 * Operations requested by the manager
 */

/* Send back state of the host to the manager as a text message */
void reply_display_host_state(
		struct man_port_at_host *port,
		char dir[],
		int dir_valid,
		int host_id)
{
int n;
char reply_msg[MAX_MSG_LENGTH];

if (dir_valid == 1) {
	n =sprintf(reply_msg, "%s %d", dir, host_id);
}
else {
	n = sprintf(reply_msg, "None %d", host_id);
}

write(port->send_fd, reply_msg, n);
}



/* Job queue operations */

/* Add a job to the job queue */
void job_q_add(struct job_queue *j_q, struct host_job *j)
{
if (j_q->head == NULL ) {
	j_q->head = j;
	j_q->tail = j;
	j_q->occ = 1;
}
else {
	(j_q->tail)->next = j;
	j->next = NULL;
	j_q->tail = j;
	j_q->occ++;
}
}

/* Remove job from the job queue, and return pointer to the job*/
struct host_job *job_q_remove(struct job_queue *j_q)
{
struct host_job *j;

if (j_q->occ == 0) return(NULL);
j = j_q->head;
j_q->head = (j_q->head)->next;
j_q->occ--;
return(j);
}

/* Initialize job queue */
void job_q_init(struct job_queue *j_q)
{
j_q->occ = 0;
j_q->head = NULL;
j_q->tail = NULL;
}

int job_q_num(struct job_queue *j_q)
{
return j_q->occ;
}

/*
 *  Main 
 */

const char* get_job_type_string(int job_type) {
    switch (job_type) {
        case JOB_SEND_PKT_ALL_PORTS:
            return "JOB_SEND_PKT_ALL_PORTS";
        case JOB_PING_SEND_REQ:
            return "JOB_PING_SEND_REQ";
        case JOB_PING_SEND_REPLY:
            return "JOB_PING_SEND_REPLY";
        case JOB_PING_WAIT_FOR_REPLY:
            return "JOB_PING_WAIT_FOR_REPLY";
        case JOB_FILE_UPLOAD_SEND:
            return "JOB_FILE_UPLOAD_SEND";
        case JOB_FILE_UPLOAD_RECV_START:
            return "JOB_FILE_UPLOAD_RECV_START";
        case JOB_FILE_UPLOAD_RECV_CONT:
            return "JOB_FILE_UPLOAD_RECV_CONT";
        case JOB_FILE_UPLOAD_RECV_END:
            return "JOB_FILE_UPLOAD_RECV_END";
        case JOB_FILE_DOWNLOAD_SEND:
            return "JOB_FILE_DOWNLOAD_SEND";
        case JOB_FILE_DOWNLOAD_RECV:
            return "JOB_FILE_DOWNLOAD_RECV";
        default:
            return "UNKNOWN_JOB_TYPE";
    }
}

void display_job_number(struct job_queue *j) {
   printf("\nnumber of jobs = %d\n", job_q_num(j));
}

void display_packet_info(struct packet *pkt) {
    const char* type_string;
    printf("\n\nPacket Info:\n");
    switch (pkt->type) {
        case PKT_PING_REQ:
            type_string = "PKT_PING_REQ";
            break;
        case PKT_PING_REPLY:
            type_string = "PKT_PING_REPLY";
            break;
        case PKT_FILE_UPLOAD_START:
            type_string = "PKT_FILE_UPLOAD_START";
            break;
        case PKT_FILE_UPLOAD_CONT:
            type_string = "PKT_FILE_UPLOAD_CONT";
            break;
        case PKT_FILE_UPLOAD_END:
            type_string = "PKT_FILE_UPLOAD_END";
            break;
        case PKT_FILE_DOWNLOAD_SEND:
            type_string = "PKT_FILE_DOWNLOAD_SEND";
            break;
        default:
            type_string = "UNKNOWN_PACKET_TYPE";
            break;
    }

    printf("Source: %d\n", pkt->src);
    printf("Destination: %d\n", pkt->dst);
    printf("Type: %s\n", type_string);
    printf("Length: %d\n", pkt->length);
    printf("Payload: %s\n", pkt->payload);
}

void print_job_queue_contents(struct job_queue *queue) {
    struct host_job *current_job = queue->head;

    printf("\n\n\n\nPrinting job queue contents:\n\n\n\n");
    while (current_job != NULL) {
        printf("Job type: %s\n", get_job_type_string(current_job->type));
        printf("Input port index: %d\n", current_job->in_port_index);
        printf("Output port index: %d\n", current_job->out_port_index);

        if (current_job->packet != NULL) {
            printf("Packet data: %s\n", current_job->packet->payload);
        } else {
            printf("Packet data: NULL\n");
        }

        printf("Download file name: %s\n", current_job->fname_download);
        printf("Upload file name: %s\n", current_job->fname_upload);
        printf("Ping timer: %d\n", current_job->ping_timer);
        printf("File upload destination: %d\n", current_job->file_upload_dst);

        current_job = current_job->next;
    }
}


