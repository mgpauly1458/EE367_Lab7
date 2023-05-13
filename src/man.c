/**

@file man.c
@brief Source code for the manager.
This file contains the implementation of a manager which is responsible
for managing multiple hosts. It allows the user to interact with the
hosts and perform various actions like changing the current host, displaying
host's state, pinging a host, uploading a file to a host, and downloading a file
from a host.
*/

#include "man.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "host.h"
#include "main.h"
#include "net.h"
#include "dns.h"

#define MAXBUFFER 1000
#define PIPE_WRITE 1
#define PIPE_READ 0
#define TENMILLISEC 10000
#define DELAY_FOR_HOST_REPLY 10 /* Delay in ten of milliseconds */



void display_host(struct man_port_at_man *list,
                  struct man_port_at_man *curr_host);
void change_host(struct man_port_at_man *list,
                 struct man_port_at_man **curr_host);
void display_host(struct man_port_at_man *list,
                  struct man_port_at_man *curr_host);
void display_host_state(struct man_port_at_man *curr_host);
void set_host_dir(struct man_port_at_man *curr_host);
char man_get_user_cmd(int curr_host);

/**

@brief Get the user command.
This function prompts the user to enter a command from the available options
and returns the command entered by the user.
@param curr_host ID of the current host.
@return The user command.
*/
char man_get_user_cmd(int curr_host) {
  char cmd;

  while (1) {
    /* Display command options */
    printf("\nCommands (Current host ID = %d):\n", curr_host);
    printf("   (s) Display host's state\n");
    printf("   (m) Set host's main directory\n");
    printf("   (h) Display all hosts\n");
    printf("   (c) Change host\n");
    printf("   (k) Set a domain name\n");
    printf("   (p) Ping a host using hostID\n");
    printf("   (g) Ping a host using domain name\n");
    printf("   (u) Upload a file to a host\n");
    printf("   (d) Download a file from a host using hostID\n");
    printf("   (j) Download a file from a host using domain name\n");
    printf("   (q) Quit\n");
    printf("   Enter Command: ");
    do {
      cmd = getchar();
    } while (cmd == ' ' || cmd == '\n'); /* get rid of junk from stdin */

    /* Ensure that the command is valid */
    switch (cmd) {
      case 's':
      case 'm':
      case 'h':
      case 'c':
      case 'p':
      case 'u':
      case 'd':
      case 'q':
      case 'g':
      case 'j':
      case 'k':
        return cmd;
      default:
        printf("Invalid: you entered %c\n\n", cmd);
    }
  }
}

/**

@brief Change the current host.
This function prompts the user to enter the ID of the new host to set as the current host.
It searches for the port of the new host and sets it as the current host.
@param list The list of all hosts.
@param curr_host Pointer to the current host.
*/
void change_host(struct man_port_at_man *list,
                 struct man_port_at_man **curr_host) {
  int new_host_id;

  // display_host(list, *curr_host);
  printf("Enter new host: ");
  scanf("%d", &new_host_id);
  printf("\n");

  /* Find the port of the new host, and then set it as the curr_host */
  struct man_port_at_man *p;
  for (p = list; p != NULL; p = p->next) {
    if (p->host_id == new_host_id) {
      *curr_host = p;
      break;
    }
  }
}

/**

@brief Display the hosts on the console.
This function displays the list of all hosts on the console, along with an indication
of the currently connected host.
@param list The list of all hosts.
@param curr_host Pointer to the current host.
*/
void display_host(struct man_port_at_man *list,
                  struct man_port_at_man *curr_host) {
  struct man_port_at_man *p;

  printf("\nHost list:\n");
  for (p = list; p != NULL; p = p->next) {
    printf("   Host id = %d ", p->host_id);
    if (p->host_id == curr_host->host_id) {
      printf("(<- connected)");
    }
    printf("\n");
  }
}

/**

@brief Send command to the host for its state.
This function sends a command message to the current host to get its state. The command
is a single character 's'. It then waits for a reply from the host, which should be the
host's state. The state is displayed on the console.
@param curr_host Pointer to the current host.
*/
void display_host_state(struct man_port_at_man *curr_host) {
  char msg[MAN_MSG_LENGTH];
  char reply[MAN_MSG_LENGTH];
  char dir[NAME_LENGTH];
  int host_id;
  int n;

  msg[0] = 's';
  write(curr_host->send_fd, msg, 1);

  n = 0;
  while (n <= 0) {
    usleep(TENMILLISEC);
    n = read(curr_host->recv_fd, reply, MAN_MSG_LENGTH);
  }
  reply[n] = '\0';
  sscanf(reply, "%s %d", dir, &host_id);
  printf("Host %d state: \n", host_id);
  printf("    Directory = %s\n", dir);
}

/**

@brief Set the host's main directory.
This function prompts the user to enter the name of the directory to set as the host's
main directory. It then sends a command message to the current host to set the directory.
@param curr_host Pointer to the current host.
*/
void set_host_dir(struct man_port_at_man *curr_host) {
  char name[NAME_LENGTH];
  char msg[NAME_LENGTH];
  int n;

  printf("Enter directory name: ");
  scanf("%s", name);
  n = sprintf(msg, "m %s", name);
  write(curr_host->send_fd, msg, n);
}

void register_domain_name(struct man_port_at_man * curr_host) {
   char msg[MAN_MSG_LENGTH];
   char domain_name[MAX_NAME_LENGTH];
   int n;

   printf("Enter a domain name to save: ");
   scanf("%s", domain_name);
   n = sprintf(msg, "k %s", domain_name);
   write(curr_host->send_fd, msg, n);
}

/*
 * Command host to send a ping to the host with id "curr_host"
 *
 * User is queried for the id of the host to ping.
 *
 * A command message is sent to the current host.
 *    The message starrts with 'p' followed by the id
 *    of the host to ping.
 *
 * Wiat for a reply
 */

/**

@brief Ping a host from the current host.
This function prompts the user to enter the ID of the host to ping. It then sends a command
message to the current host to ping the specified host. It waits for a reply from the host
and displays it on the console.
@param curr_host Pointer to the current host.
*/
void ping(struct man_port_at_man *curr_host) {
  char msg[MAN_MSG_LENGTH];
  char reply[MAN_MSG_LENGTH];
  int host_to_ping;
  int n;

  printf("Enter id of host to ping: ");
  scanf("%d", &host_to_ping);
  n = sprintf(msg, "p %d", host_to_ping);

  write(curr_host->send_fd, msg, n);

  n = 0;
  while (n <= 0) {
    usleep(TENMILLISEC);
    n = read(curr_host->recv_fd, reply, MAN_MSG_LENGTH);
  }
  reply[n] = '\0';
  printf("%s\n", reply);
}

void ping_by_domain_name(struct man_port_at_man *curr_host) {
  char msg[MAN_MSG_LENGTH];
  char reply[MAN_MSG_LENGTH];
  char domain_name[MAX_NAME_LENGTH];
  int n;

  printf("Enter domain name of host to ping: ");
  scanf("%s", domain_name);
  n = sprintf(msg, "g %s", domain_name);

  write(curr_host->send_fd, msg, n);

  n = 0;
  while (n <= 0) {
    usleep(TENMILLISEC);
    n = read(curr_host->recv_fd, reply, MAN_MSG_LENGTH);
  } 
  reply[n] = '\0';
  printf("%s\n", reply);
}

/**

@brief Upload a file from the current host to another host.
This function prompts the user to enter the name of the file to transfer and the ID of the
destination host. It then sends a command message to the current host to upload the file to
the specified host.
@param curr_host Pointer to the current host.
@return 0 on success, -1 on failure.
*/
int file_upload(struct man_port_at_man *curr_host) {
  int n;
  int host_id;
  char name[NAME_LENGTH];
  char msg[NAME_LENGTH];

  printf("Enter file name to upload: ");
  scanf("%s", name);
  printf("Enter host id of destination:  ");
  scanf("%d", &host_id);
  printf("\n");

  n = sprintf(msg, "u %d %s", host_id, name);
  write(curr_host->send_fd, msg, n);
  usleep(TENMILLISEC);
}

/**

@brief Download a file from a host.
This function prompts the user to enter the name of the file to download and the ID of the
host from which to download the file. It then sends a command message to the current host to
download the file from the specified host.
@param curr_host Pointer to the current host.
@return 0 on success, -1 on failure.
*/
int file_download(struct man_port_at_man *curr_host) {
  int n;
  int host_id;
  char name[NAME_LENGTH];
  char msg[NAME_LENGTH];

  printf("Enter file name to download: ");
  scanf("%s", name);
  printf("Enter host id of destination: ");
  scanf("%d", &host_id);
  printf("\n");

  n = sprintf(msg, "u %d %s", host_id, name);
  write(curr_host->send_fd, msg, n);
  usleep(TENMILLISEC);
}

int file_download_domain(struct man_port_at_man *curr_host) {
   int n;
   char domain_name[MAX_NAME_LENGTH];
   char name[NAME_LENGTH];
   char msg[NAME_LENGTH];

   printf("Enter file name to download: ");
   scanf("%s", name);
   printf("Enter domain name of destination: ");
   scanf("%s", domain_name);
   printf("\n");

   n = sprintf(msg, "j %s %s", domain_name, name);
   write(curr_host->send_fd, msg, n);
   usleep(TENMILLISEC);
}

/**

@brief Main loop of the manager.
This function contains the main loop of the manager. It repeatedly gets a command
from the user and executes it.
*/
void man_main() {
  // State
  struct man_port_at_man *host_list;
  struct man_port_at_man *curr_host = NULL;

  host_list = net_get_man_ports_at_man_list();
  curr_host = host_list;

  char cmd; /* Command entered by user */

  while (1) {
    /* Get a command from the user */
    cmd = man_get_user_cmd(curr_host->host_id);

    /* Execute the command */
    switch (cmd) {
      case 's': /* Display the current host's state */
        display_host_state(curr_host);
        break;
      case 'm': /* Set host directory */
        set_host_dir(curr_host);
        break;
      case 'h': /* Display all hosts connected to manager */
        display_host(host_list, curr_host);
        break;
      case 'c': /* Change the current host */
        change_host(host_list, &curr_host);
        break;
      case 'k': /* Set a domain name up */
        register_domain_name(curr_host);
        break;
      case 'p': /* Ping a host from the current host using hostID */
        ping(curr_host);
        break;
      case 'g': /* Ping a host from the current host using domain name */ 
        ping_by_domain_name(curr_host);
        break;
      case 'u': /* Upload a file from the current host
                   to another host */
        file_upload(curr_host);
        break;
      case 'd': /* Download a file from a host using hostID */
        file_download(curr_host);
        break;
      case 'j': /* Download a file from a host using domain name */
        file_download_domain(curr_host);
        break;
      case 'q': /* Quit */
        return;
      default:
        printf("\nInvalid, you entered %c\n\n", cmd);
    }
  }
}
