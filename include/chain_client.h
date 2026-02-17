#ifndef CHAIN_CLIENT_H
#define CHAIN_CLIENT_H

#include <pthread.h>

#include "client_stub.h"

int zoo_init_client(struct rlist_t **rlh, struct rlist_t **rlt, char *hp, pthread_mutex_t* lock);

void zoo_clean_client();

#endif