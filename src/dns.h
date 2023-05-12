#define TABLE_SIZE 200
#define MAX_NAME_LENGTH 100
#define DNS_SERVER_ID 100

struct naming_table_entry {
   char domain_name[MAX_NAME_LENGTH+1];
   int physical_id;
   int valid;
};

struct naming_table {
   int size;
   struct naming_table_entry entries[TABLE_SIZE];
};


void dns_main(int);
void init_dns_table(struct naming_table *table);
void print_dns_table(struct naming_table *table);

