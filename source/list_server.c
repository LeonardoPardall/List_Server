/* Source file: list_server.c */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "chain_server.h"

#include "network_server.h"
#include "list_skel.h"

static volatile sig_atomic_t stop = 0;

static void int_handler(int signum) {
  (void)signum;
  stop = 1;
  network_server_request_shutdown();
}

int main(int argc, char *argv[]) {  
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = int_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0; /* do not set SA_RESTART */
  sigaction(SIGINT, &sa, NULL);

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <zk_host:port> <server_port>\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  int port = atoi(argv[2]);
  if (port <= 0) {
      fprintf(stderr, "Invalid port\n");
      return EXIT_FAILURE;
  }

  struct list_t *list = list_skel_init();
  if (!list) {
    fprintf(stderr, "Failed to initialize list skeleton\n");
    return EXIT_FAILURE;
  }

  int sock = network_server_init((short)port);
  if (sock < 0) {
      fprintf(stderr, "Failed to initialize network server\n");
      list_skel_destroy(list);
      return EXIT_FAILURE;
  }

  if(zoo_init_server(argv[1], port, list) < 0) {
    fprintf(stderr, "Erro ao conectar ao zookeeper");
    zoo_clean_server();
    list_skel_destroy(list);
    network_server_close(sock);
    return EXIT_FAILURE;
  }
  
  if (network_main_loop(sock, list) < 0) {
      fprintf(stderr, "network_main_loop returned error\n");
  }
  
  zoo_clean_server();
  list_skel_destroy(list);
  network_server_close(sock);

  return 1;
}