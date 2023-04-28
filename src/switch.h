#define MAX_TABLE_SIZE 100

void switch_main(int);

void display_port_info(struct net_port*);

struct forward_table {
   int size;
   int valid[100];
   int HostID[100];
   int port[100];
};
