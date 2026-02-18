/* Header: chain_server.h */

#ifndef CHAIN_SERVER_H
#define CHAIN_SERVER_H

#include <pthread.h>
#include <zookeeper/zookeeper.h>
#include "client_stub.h"
#include "network_client.h" 

typedef struct String_vector zoo_string;


void get_addr(char *hostbuf, size_t buf);
int zoo_forward_to_successor(MessageT *msg);
int zoo_init_server(char *hp, int port, struct list_t *list);
void zoo_clean_server();


#endif
