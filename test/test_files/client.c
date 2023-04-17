#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sockets.h"

#define PAYLOAD_MAX 1024

/// This is a packet data type.
struct packet {
  char src;  /// This is a src. It is the source host ID of the packet
  char dst;
  char type;
  int length;
  char payload[PAYLOAD_MAX];
};

void test_send_packet() {
  struct packet *p = malloc(sizeof(struct packet));
  p->src = 3;
  p->dst = 0;
  p->type = 1;
  p->length = 12;
  strncpy(p->payload, "Hello world", p->length);

  create_client("wiliki.eng.hawaii.edu", 3502, p);

  printf("Packet sent.\n");
}

int main() {
  test_send_packet();
  return 0;
}
