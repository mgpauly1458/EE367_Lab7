#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "switch.h"
#include "packet.h"

void display_port_info(struct net_port *p) {
    printf("Net port:\n");
    printf("  type: %d\n", p->type);
    printf("  pipe_host_id: %d\n", p->pipe_host_id);
    printf("  pipe_send_fd: %d\n", p->pipe_send_fd);
    printf("  pipe_recv_fd: %d\n", p->pipe_recv_fd);
    printf("  next: %p\n", (void *) p->next);
    printf("  sock_host_id: %d\n", p->sock_host_id);
}

void display_forward_table(struct forward_table table) {
   int i;
   printf("Forward table:\n");
   printf("Size: %d\n", table.size);
   printf("Valid\tHost ID\tPort\n");
   for (i = 0; i < 100; i++) {
      if (table.valid[i] != 0 || table.HostID[i] != -1) {
         printf("%d\t%d\t%d\n", table.valid[i], table.HostID[i], table.port[i]);
      }
   }
}


void init_forward_table(struct forward_table *table) {
   int i;
   table->size = 0;
   for (i = 0; i < MAX_TABLE_SIZE; i++) {
      table->valid[i] = 0;
      table->HostID[i] = -1;
      table->port[i] = 0;
   }
}

void add_src_to_table(struct forward_table *table, struct packet *pkt, int port_index) {
   if (table->valid[(int)pkt->src]) return;
   table->valid[(int)pkt->src] = 1;
   table->HostID[(int)pkt->src] = pkt->src;
   table->port[(int)pkt->src] = port_index;

}

void send_to_all_ports(int node_port_num, struct net_port **node_port, struct packet *pkt) {
   for (int k = 0; k < node_port_num; k++) {
      packet_send(node_port[k], pkt);
   }
  free(pkt);
}


int is_host_in_table(struct forward_table *table, char dst) {
  // printf("\ndst=%d table->valid[(intt)dst]=%d\n", dst, table->valid[(int)dst]);
   return table->valid[(int)dst];
}
