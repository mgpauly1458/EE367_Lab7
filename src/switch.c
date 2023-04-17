#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "net.h"
#include "man.h"
#include "host.h"
#include "packet.h"
#include "switch.h"
#include "sockets.h"
#include "switch_util.h"

void switch_main(int host_id) {

 //initialization
   struct net_port *node_port_list;
   struct net_port **node_port;
   int node_port_num;
   struct packet *in_packet;
   struct packet *new_packet;
   struct net_port *p;
   int i, k, n;
   struct forward_table table;

   init_forward_table(&table);

   node_port_list = net_get_port_list(host_id);

   node_port_num = 0;
   for(p=node_port_list; p!=NULL; p=p->next) {
      node_port_num++;
   }
   
   node_port = (struct net_port **) malloc(node_port_num*sizeof(struct net_port *));

   // populate node_port
   p = node_port_list;
   for (k=0; k<node_port_num; k++) {
      node_port[k] = p;
      p = p->next;
   
   }


  // display_forward_table(table);

      // socket
   int fd[2];
   pid_t pid;
   struct net_data **g_net_data_ptr = get_g_net_data();
   struct net_data *g_net_data = *g_net_data_ptr;
   
       // Create the pipe
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
      
          // Set the read end of the pipe to non-blocking
    if (fcntl(fd[0], F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    // Set the write end of the pipe to non-blocking
    if (fcntl(fd[1], F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

      // Fork the process
    pid = fork();

      // Check for errors
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

      // Child process
    if (pid == 0) {
        // Close the read end of the pipe
        close(fd[0]);

        create_server(g_net_data->server_port, fd[1]);

        // Exit the child process
        exit(EXIT_SUCCESS);
    }

    // Parent process
    else {
        // Close the write end of the pipe
      close(fd[1]);
      g_net_data->server_pipe = fd[0];
   
   //main loop
      while(1) {

    //get packets from incoming links
       //scan all ports
         for (k = 0; k < node_port_num; k++) {
            in_packet = (struct packet *) malloc(sizeof(struct packet));
            n = packet_recv(node_port[k], in_packet);

            if (n > 0) {
               
               // display_packet_info(in_packet); 
//              display_forward_table(table);
               // add packet routing here
               // check whole table
               if(is_host_in_table(&table, in_packet->dst)) {
                  // port is in table, send it
                  packet_send(node_port[table.port[in_packet->dst]], in_packet);
                  add_src_to_table(&table, in_packet, k);

              } else {
                  // port is not in table
                  add_src_to_table(&table, in_packet, k);
                  send_to_all_ports(node_port_num, node_port, in_packet);
               }
            }
         }
      }
   }
}
