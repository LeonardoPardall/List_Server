/* 
 * Grupo nº 07
 * Afonso Henriques - 61826
 * Leonardo Pardal - 61836
 * Pedro Carvalho - 61800
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zookeeper/zookeeper.h>
#include <pthread.h>

#include "client_stub.h"

#define ZDATALEN 1024 * 1024
#define MAX_ADDR 16

typedef struct String_vector zoo_string;

struct rlist_t **remote_list_head = NULL;
struct rlist_t **remote_list_tail = NULL;
char prev_remote_list_head_addr[16] = {0};
char prev_remote_list_tail_addr[16] = {0};
pthread_mutex_t* rls_lock = NULL;

char *host_port;
char *root_path = "/chain";
zhandle_t *zh;
int is_connected;
char *watcher_ctx = "Client Child Watcher";

void get_node_data(const char *path) {
  char buffer[ZDATALEN];
  int buffer_len = sizeof(buffer);
  struct Stat stat;

  char str[1024] = "/chain/";
  strcat(str, path);

  int rc = zoo_get(zh, str, 0, buffer, &buffer_len, &stat);

  if (rc == ZOK) {
    buffer[buffer_len] = '\0'; // Adiciona terminador nulo
    printf("Dados de %s:\n", path);
    printf("  Conteúdo: %s\n", buffer);
    printf("  Tamanho: %d bytes\n", buffer_len);
    printf("  Versão: %d\n", stat.version);
    printf("  mtime: %ld\n", stat.mtime);
    printf("  ctime: %ld\n", stat.ctime);
  } else {
    fprintf(stderr, "Erro ao obter %s: %d\n", path, rc);
  }
}

void update_remote_list(const char *path, struct rlist_t **rl,
                        char *prev_addr) {
  char buffer[ZDATALEN];
  int buffer_len = sizeof(buffer);
  struct Stat stat;

  char zk_path[1024] = "/chain/";
  strcat(zk_path, path);

  int rc = zoo_get(zh, zk_path, 0, buffer, &buffer_len, &stat);

  if (rc != ZOK) {
    fprintf(stderr, "Erro ao obter %s: %d\n", path, rc);
    return;
  }

  // Garante string terminada
  buffer[buffer_len] = '\0';

  // REMOVE \n E \r DO ZOOKEEPER (IMPORTANTÍSSIMO)
  buffer[strcspn(buffer, "\r\n")] = '\0';

  pthread_mutex_lock(rls_lock);

  if (*rl != NULL) {
    if (rlist_disconnect(*rl) == -1) {
      printf("Erro ao desconectar do último servidor\n");
    }
  }

  *rl = rlist_connect(buffer);
  if (*rl == NULL) {
    if (errno == EBUSY) {
      printf("Server busy. Try again later.\n");
    } else {
      printf("Error in connect\n");
    }
  }
  pthread_mutex_unlock(rls_lock);
}

void update(zoo_string *children_list) {
  if (children_list->count == 0) {
    return;
  } else {

    int min = INT_MAX;
    int max = INT_MIN;
    int minIndex = 0;
    int maxIndex = 0;

    for (int i = 0; i < children_list->count; i++) {
      int num;
      // Parse the number after "node-"
      if (sscanf(children_list->data[i], "node-%d", &num) == 1) {
        if (num < min) {
          min = num;
          minIndex = i;
        }
        if (num > max) {
          max = num;
          maxIndex = i;
        }
      }
    }

    char *node_head = children_list->data[minIndex];
    char *node_tail = children_list->data[maxIndex];

    update_remote_list(node_head, remote_list_head, prev_remote_list_head_addr);
    update_remote_list(node_tail, remote_list_tail, prev_remote_list_tail_addr);
  }
}

/* Observador para a conexão */
static void connection_watcher(zhandle_t *zzh, int type, int state,
                               const char *path, void *context) {
  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_CONNECTED_STATE) {
      is_connected = 1;
    } else {
      is_connected = 0;
    }
  }
}

/* Observador de filhos */
static void child_watcher(zhandle_t *wzh, int type, int state,
                          const char *zpath, void *watcher_ctx) {
  // Verificar se estou conectado ao zoo e se o evento é do tipo child
  if (state == ZOO_CONNECTED_STATE && type == ZOO_CHILD_EVENT) {
    zoo_string children_list;

    /* Obtém a lista atualizada de filhos */
    int rc = zoo_wget_children(zh, root_path, child_watcher, watcher_ctx,
                               &children_list);
    if (rc != ZOK) {
      fprintf(stderr, "Erro ao definir watch em %s! Código: %d\n", root_path,
              rc);
      return;
    } else {
      update(&children_list);
    }

    deallocate_String_vector(&children_list);
  }
}

int zoo_init_client(struct rlist_t **rlh, struct rlist_t **rlt, char *hp, pthread_mutex_t* lock) {
  if (!hp) {
    fprintf(stderr, "Host é null (zoo_init_client)\n");
    return -1;
  }

  remote_list_head = rlh;
  remote_list_tail = rlt;
  rls_lock = lock;

  zoo_set_debug_level((ZooLogLevel)0);

  zh = zookeeper_init(hp, connection_watcher, 2000, 0, 0, 0);
  if (zh == NULL) {
    fprintf(stderr, "Erro ao conectar ao ZooKeeper! Código: %d\n", errno);
    return -1;
  }

  zoo_string children_list;
  int rc;

  // Criar  o nó /chain caso não exista
  if (zoo_exists(zh, root_path, 0, NULL) == ZNONODE) {
    rc = zoo_create(zh, root_path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    if (rc == ZOK) {
      printf("Nó %s criado com sucesso!\n", root_path);
    } else {
      fprintf(stderr, "Erro ao criar %s! Código: %d\n", root_path, rc);
      return -1;
    }
  }

  // Configura o watcher inicial para /chain
  rc = zoo_wget_children(zh, root_path, child_watcher, watcher_ctx,
                         &children_list);
  if (rc != ZOK) {
    fprintf(stderr, "Erro ao configurar watcher inicial! Código: %d\n", rc);
    return -1;
  } else {
    update(&children_list);
  }

  deallocate_String_vector(&children_list);

  // Se houve um erro ao conectar no update retorna erro
  if (*remote_list_head == NULL || *remote_list_tail == NULL || errno == EBUSY) {
    return -1;
  }

  return 1;
}

void zoo_clean_client() {
  if (!zh)
    return;

  zookeeper_close(zh);
}

