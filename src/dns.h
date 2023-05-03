/*
   This file contains the functions neccesary for the dns server. I will move them into the right files after writing them. 
*/

#include <stdio.h>
#include <string.h>

#define DNS_SERVER_ID 100
#define TABLE_SIZE 256
#define MAX_NAME_LENGTH 50

struct naming_table_entry {
   char domain_name[MAX_NAME_LENGTH+1];  // one extra char for the null terminating string
   int physical_id;
   int valid;
};

struct naming_table {
   int size;
   struct naming_table_entry entries[TABLE_SIZE];
};

void init_dns_table(struct naming_table * naming_table);

void print_dns_table (struct naming_table *  naming_table);


/*
 In the main file we can create a struct dns_entry dns_server[TABLE_SIZE]. Then pass &dns_server into the init_dns_server function
 */
