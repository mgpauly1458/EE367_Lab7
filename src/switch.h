#define MAX_TABLE_SIZE 200

void switch_main(int);

void display_port_info(struct net_port*);

struct forward_table {
   int size;
   int valid[200];
   int HostID[200];
   int port[200];
};
