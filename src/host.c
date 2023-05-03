/**
 * @file host.c
 * 
 * @brief Implements the host main loop.
 * 
 * Assignment 4.1 was to allow the download and upload of files of 
 * up to 1000 bytes from other hosts without changing the packet 
 * capacity (100 bytes). I did this by modifying the upload code.
 * 
 * When a user wants to upload a file to another host, the file is 
 * first opened and read, and then broken down into smaller packets. 
 * The initial packet, containing the file name, is sent as a 
 * PKT_FILE_UPLOAD_START packet, followed by one or more PKT_FILE_UPLOAD_CONT 
 * packets containing the actual file content. These packets are sent via the 
 * network using the JOB_SEND_PKT_ALL_PORTS job type, which ensures that the 
 * packets are transmitted through all available ports. Once the entire file 
 * has been sent, a PKT_FILE_UPLOAD_END packet is transmitted to signal the 
 * completion of the file transfer. On the receiving host, the packets are 
 * processed and reassembled, and the file is stored in the destination host's
 *  designated directory.
 * 
 * When a user wants to download a file, the filename is sent along with a 
 * PKT_FILE_DOWNLOAD_SEND packet, which contains the filename in the payload.
 * The receiving host then reads the file name, it initiates an upload job
 * to itself with the src of the packet as the dst.
 * 
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
#include "host_util.h"

void host_main(int host_id)
{

/* State */
char dir[MAX_DIR_NAME];
int dir_valid = 0;


char man_msg[MAN_MSG_LENGTH];
char man_reply_msg[MAN_MSG_LENGTH];
char man_cmd;
struct man_port_at_host *man_port;  // Port to the manager

struct net_port *node_port_list;
struct net_port **node_port;  // Array of pointers to node ports
int node_port_num;            // Number of node ports

int ping_reply_received;

int i, k, n;
int dst;
char name[MAX_FILE_NAME];
char domain_name[MAX_NAME_LENGTH];
char string[PKT_PAYLOAD_MAX+1]; 

FILE *fp;

struct packet *in_packet; /* Incoming packet */
struct packet *new_packet;

struct net_port *p;
struct host_job *new_job;
struct host_job *new_job2;

struct job_queue job_q;

struct file_buf f_buf_upload;  
struct file_buf f_buf_download; 

file_buf_init(&f_buf_upload);
file_buf_init(&f_buf_download);

/*
 * Initialize pipes 
 * Get link port to the manager
 */

man_port = net_get_host_port(host_id);

/*
 * Create an array node_port[ ] to store the network link ports
 * at the host.  The number of ports is node_port_num
 */
node_port_list = net_get_port_list(host_id);

	/*  Count the number of network link ports */
node_port_num = 0;
for (p=node_port_list; p!=NULL; p=p->next) {
	node_port_num++;
}

/* Create memory space for the array */
node_port = (struct net_port **) 
	malloc(node_port_num*sizeof(struct net_port *));

	/* Load ports into the array */
p = node_port_list;
for (k = 0; k < node_port_num; k++) {
	node_port[k] = p;
   p = p->next;
}	

/* Initialize the job queue */
job_q_init(&job_q);

while(1) {
	/* Execute command from manager, if any */

		/* Get command from manager */
	n = get_man_command(man_port, man_msg, &man_cmd);

		/* Execute command */
	if (n>0) {
		switch(man_cmd) {
			case 's':
				reply_display_host_state(man_port,
					dir, 
					dir_valid,
					host_id);
				break;	
			
			case 'm':
				dir_valid = 1;
				for (i=0; man_msg[i] != '\0'; i++) {
					dir[i] = man_msg[i];
				}
				dir[i] = man_msg[i];
				break;

			case 'p': // Sending ping request
				// Create new ping request packet
				sscanf(man_msg, "%d", &dst);
				new_packet = (struct packet *) 
						malloc(sizeof(struct packet));	
				new_packet->src =  host_id;
				new_packet->dst =  dst;
				new_packet->type = (char) PKT_PING_REQ;
				new_packet->length = 0;
				new_job = (struct host_job *) 
						malloc(sizeof(struct host_job));
				new_job->packet = new_packet;
				new_job->type = JOB_SEND_PKT_ALL_PORTS;
				job_q_add(&job_q, new_job);

				new_job2 = (struct host_job *) 
						malloc(sizeof(struct host_job));
				ping_reply_received = 0;
				new_job2->type = JOB_PING_WAIT_FOR_REPLY;
				new_job2->ping_timer = 20;
				job_q_add(&job_q, new_job2);

				break;

			case 'u': /* Upload a file to a host */
				sscanf(man_msg, "%d %s", &dst, name);
				new_job = (struct host_job *) 
						malloc(sizeof(struct host_job));
				new_job->type = JOB_FILE_UPLOAD_SEND;
				new_job->file_upload_dst = dst;	
				for (i=0; name[i] != '\0'; i++) {
					new_job->fname_upload[i] = name[i];
				}
				new_job->fname_upload[i] = '\0';
				job_q_add(&job_q, new_job);
				
            break;
			
         case 'd':
            sscanf(man_msg, "%d %s", &dst, name);
            new_job = (struct host_job *) malloc(sizeof(struct host_job));
            new_job->type = JOB_FILE_DOWNLOAD_SEND;
            new_job->file_download_dst = dst;
            for (i=0; name[i] != '\0'; i++) {
               new_job->fname_download[i] = name[i];
            }
            new_job->fname_download[i] = '\0';
            job_q_add(&job_q, new_job);
            
            break;
         
         case 'g': // Ping with domain name
            break;
         
         case 'j': // Download with domain name
            break;
         
         /* 
          * Set Domain Name
          * 
          * From the current host node ...
          *    Create a packet with dst of DNS SERVER, and payload of domain_name
          *    Create a job to send the packet on all ports
          */
         case 'k': 
            sscanf(man_msg, "%s", domain_name);
            printf("debug: domain name %s request recieved in host.c\n", domain_name);
            // Create the request packet to send to DNS Server w/ domain name
            new_packet = (struct packet*) malloc(sizeof(struct packet *));
            new_packet->src = (char)host_id;
            new_packet->dst = (char)DNS_SERVER_ID;
            new_packet->type = (char)PKT_SET_DOMAIN;
            //printf("debug: packet->src: %d\n", new_packet->src);
            //printf("debug: packet->dst (should be 100): %d\n", new_packet->dst);
            //printf("debug: packet->type (should be 9) : %d\n", new_packet->type);
            for (i=0; domain_name[i] != '\0'; i++) {
               new_packet->payload[i] = domain_name[i];
            }
            new_packet->payload[i] = '\0';
            new_packet->length = i;
            //printf("debug: packet->paylod: %s\n", new_packet->payload);
            //printf("debug: packet->length: %d\n", new_packet->length);
            // Create job to send the packet to all ports (it eventually ends up at dns server)
            new_job = (struct host_job*)malloc(sizeof(struct host_job));
            new_job->packet = new_packet;
            new_job->type = JOB_SEND_PKT_ALL_PORTS;
            job_q_add(&job_q, new_job);
            printf("Set domain job sent to job queue\n");
            break;
         default:
			;
		}
	}
	
	/*
	 * Get packets from incoming links and translate to jobs
  	 * Put jobs in job queue
 	 */

	for (k = 0; k < node_port_num; k++) { /* Scan all ports */

		in_packet = (struct packet *) malloc(sizeof(struct packet));
		n = packet_recv(node_port[k], in_packet);

		if ((n > 0) && ((int) in_packet->dst == host_id)) {
			new_job = (struct host_job *) 
				malloc(sizeof(struct host_job));
			new_job->in_port_index = k;
			new_job->packet = in_packet;

			switch(in_packet->type) {
				/* Consider the packet type */

				/* 
				 * The next two packet types are 
				 * the ping request and ping reply
				 */
				case (char) PKT_PING_REQ: 
					new_job->type = JOB_PING_SEND_REPLY;
					job_q_add(&job_q, new_job);
					break;

				case (char) PKT_PING_REPLY:
					ping_reply_received = 1;
					free(in_packet);
					free(new_job);
					break;

				/* 
				 * The next two packet types
				 * are for the upload file operation.
				 *
				 * The first type is the start packet
				 * which includes the file name in
				 * the payload.
				 *
				 * The second type is the end packet
				 * which carries the content of the file
				 * in its payload
				 */
		
				case (char) PKT_FILE_UPLOAD_START:
					new_job->type 
						= JOB_FILE_UPLOAD_RECV_START;
					job_q_add(&job_q, new_job);
					break;
            case (char) PKT_FILE_UPLOAD_CONT:
               new_job->type = JOB_FILE_UPLOAD_RECV_CONT;
               job_q_add(&job_q, new_job);
               break;
				case (char) PKT_FILE_UPLOAD_END:
					new_job->type 
						= JOB_FILE_UPLOAD_RECV_END;
					job_q_add(&job_q, new_job);
					break;
            case (char) PKT_FILE_DOWNLOAD_SEND:        
               new_job->type = JOB_FILE_DOWNLOAD_RECV;
               job_q_add(&job_q, new_job);
				   break;
            case (char) PKT_ID_P:
               printf("id p packet received\n");
               break;
            case (char) PKT_ID_D:
               printf("id d packet received\n");
               break;
            default:
					free(in_packet);
					free(new_job);
			}
		}
		else {
			free(in_packet);
		}
	}

	/*
 	 * Execute one job in the job queue
 	 */

	
      if (job_q_num(&job_q) > 0) {
        

		/* Get a new job from the job queue */
		new_job = job_q_remove(&job_q);

      //if (host_id == 0)display_host_job_info(new_job, host_id);

		/* Send packet on all ports */
		switch(new_job->type) {

      case JOB_RECV_GET_ID_P:
         printf("recv get id p job started\n");
         break;
      case JOB_RECV_GET_ID_D:
         printf("recv get id d job started\n");
         break;
      /* Send packets on all ports */	
		case JOB_SEND_PKT_ALL_PORTS:
			for (k=0; k<node_port_num; k++) {
				packet_send(node_port[k], new_job->packet);
			}
			free(new_job->packet);
			free(new_job);
			break;

		/* The next three jobs deal with the pinging process */
		case JOB_PING_SEND_REPLY:
			/* Send a ping reply packet */

			/* Create ping reply packet */
			new_packet = (struct packet *) 
				malloc(sizeof(struct packet));
			new_packet->dst = new_job->packet->src;
			new_packet->src =  host_id;
			new_packet->type = PKT_PING_REPLY;
			new_packet->length = 0;

			/* Create job for the ping reply */
			new_job2 = (struct host_job *)
				malloc(sizeof(struct host_job));
			new_job2->type = JOB_SEND_PKT_ALL_PORTS;
			new_job2->packet = new_packet;

			/* Enter job in the job queue */
			job_q_add(&job_q, new_job2);

			/* Free old packet and job memory space */
			free(new_job->packet);
			free(new_job);
			break;

		case JOB_PING_WAIT_FOR_REPLY:
			/* Wait for a ping reply packet */
			if (ping_reply_received == 1) {
				n = sprintf(man_reply_msg, "Ping acked!"); 
				man_reply_msg[n] = '\0';
				write(man_port->send_fd, man_reply_msg, n+1);
				free(new_job);
			}
			else if (new_job->ping_timer > 1) {
				new_job->ping_timer--;
				job_q_add(&job_q, new_job);
			}
			else { /* Time out */
				n = sprintf(man_reply_msg, "Ping time out!"); 
				man_reply_msg[n] = '\0';
				write(man_port->send_fd, man_reply_msg, n+1);
				free(new_job);
			}

			break;	


      case JOB_FILE_DOWNLOAD_SEND:
         if (dir_valid == 1) {

            for(i = 0; new_job->fname_download[i] != '\0'; i++);

            new_packet = (struct packet *) malloc(sizeof(struct packet));
            new_packet->src = host_id;
            new_packet->dst = new_job->file_download_dst;
            new_packet->type = PKT_FILE_DOWNLOAD_SEND;
            new_packet->length = i;
            strcpy(new_packet->payload, new_job->fname_download);


            new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
            new_job2->type = JOB_SEND_PKT_ALL_PORTS;
            new_job2->packet = new_packet;
            job_q_add(&job_q, new_job2);
           

         }
            free(new_job);
            break;

      case JOB_FILE_DOWNLOAD_RECV:
            
            
            new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
            new_job2->type = JOB_FILE_UPLOAD_SEND;
            strcpy(new_job2->fname_upload, new_job->packet->payload);
            new_job2->file_upload_dst = new_job->packet->src;
            job_q_add(&job_q, new_job2);
            printf("\n\ndownload recv\n\n");
            break;

         /* The next three jobs deal with uploading a file */
case JOB_FILE_UPLOAD_SEND:

			/* Open file */
			if (dir_valid == 1) {
				n = sprintf(name, "./%s/%s", 
					dir, new_job->fname_upload);
				name[n] = '\0';
				
            fp = fopen(name, "r");
				if (fp != NULL) {

				        /* 
					 * Create first packet which
					 * has the file name 
					 */
					new_packet = (struct packet *) 
						malloc(sizeof(struct packet));
					new_packet->dst 
						= (char) new_job->file_upload_dst;
					new_packet->src = host_id;
					new_packet->type 
						= PKT_FILE_UPLOAD_START;
					for (i=0; 
						new_job->fname_upload[i]!= '\0'; 
						i++) {
						new_packet->payload[i] = 
							new_job->fname_upload[i];
					}
					new_packet->length = i;

					/* 
					 * Create a job to send the packet
					 * and put it in the job queue
					 */
					new_job2 = (struct host_job *)
						malloc(sizeof(struct host_job));
					new_job2->type = JOB_SEND_PKT_ALL_PORTS;
					new_job2->packet = new_packet;
					job_q_add(&job_q, new_job2);

					/* 
					 * Create the second packet which
					 * has the file contents
					 */
					

               int maxFileBuff = MAX_FILE_BUFFER;
               while ((n  = fread(string,sizeof(char),PKT_PAYLOAD_MAX, fp)) > 0 && maxFileBuff > 0) {
               
               maxFileBuff -= PKT_PAYLOAD_MAX;

               new_packet = (struct packet *) 
						malloc(sizeof(struct packet));
					new_packet->dst 
						= (char) new_job->file_upload_dst;
					new_packet->src =  host_id;
					new_packet->type = PKT_FILE_UPLOAD_CONT;


					string[n] = '\0';

					for (i=0; i<n; i++) {
						new_packet->payload[i] 
							= string[i];
					}

					new_packet->length = n;

					/*
					 * Create a job to send the packet
					 * and put the job in the job queue
					 */

					new_job2 = (struct host_job *)
						malloc(sizeof(struct host_job));
					new_job2->type 
						= JOB_SEND_PKT_ALL_PORTS;
					new_job2->packet = new_packet;
					job_q_add(&job_q, new_job2);

               
               }
               
               fclose(fp);
               // add test end job and packet
               
               new_packet = (struct packet *) malloc(sizeof(struct packet));
               new_packet->src = host_id;
               new_packet->dst = (char) new_job->file_upload_dst;
               new_packet->type = PKT_FILE_UPLOAD_END;
               new_packet->length = 0;
               strcpy(new_packet->payload, "No Data");

               new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
               new_job2->type = JOB_SEND_PKT_ALL_PORTS;
               new_job2->packet = new_packet;
               job_q_add(&job_q, new_job2);
					free(new_job);
				}
				else {  
					/* Didn't open file */
				}
			}
			break;

case JOB_FILE_UPLOAD_RECV_START:

			/* Initialize the file buffer data structure */
			file_buf_init(&f_buf_upload);

			/* 
			 * Transfer the file name in the packet payload
			 * to the file buffer data structure
			 */
			file_buf_put_name(&f_buf_upload, 
				new_job->packet->payload, 
				new_job->packet->length);

			free(new_job->packet);
			free(new_job);
			break;

		case JOB_FILE_UPLOAD_RECV_CONT:

			/* 
			 * Download packet payload into file buffer 
			 * data structure 
			 */
			file_buf_add(&f_buf_upload, 
				new_job->packet->payload,
				new_job->packet->length);

			free(new_job->packet);
			free(new_job);
         break;

      case JOB_FILE_UPLOAD_RECV_END:

			if (dir_valid == 1) {

				/* 
				 * Get file name from the file buffer 
				 * Then open the file
				 */
				file_buf_get_name(&f_buf_upload, string);
				n = sprintf(name, "./%s/%s", dir, string);
				name[n] = '\0';
				fp = fopen(name, "w");

				if (fp != NULL) {
					/* 
					 * Write contents in the file
					 * buffer into file
					 */

					while (f_buf_upload.occ > 0) {
						n = file_buf_remove(
							&f_buf_upload, 
							string,
							PKT_PAYLOAD_MAX);
						string[n] = '\0';
						n = fwrite(string,
							sizeof(char),
							n, 
							fp);
					}

					fclose(fp);
				}	
			}

			break;
     
      }

	}
	

	/* The host goes to sleep for 10 ms */
	usleep(TENMILLISEC);

} /* End of while loop */

}




