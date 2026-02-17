/* 
 * Grupo nº 07
 * Afonso Henriques - 61826
 * Leonardo Pardal - 61836
 * Pedro Carvalho - 61800
 */

#include "chain_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define ZDATALEN 1024 * 1024

typedef struct String_vector zoo_string;

char *host_port;
int server_port;
char *root_path = "/chain";
char node_path[255] = {0};
zhandle_t *zh = NULL;
int is_connected;
char *watcher_ctx = "Server Child Watcher";

struct rlist_t *rl_next_server = NULL;
pthread_mutex_t rls_lock = PTHREAD_MUTEX_INITIALIZER;

int detect_ip(char *buf, size_t buflen) {
  struct ifaddrs *ifaddr = NULL, *ifa;

  if (getifaddrs(&ifaddr) != 0)
    return -1;
  const char *preferred_prefixes[] = {"eth", "wlan"};
  const size_t n_pref =
      sizeof(preferred_prefixes) / sizeof(preferred_prefixes[0]);
  for (size_t k = 0; k < n_pref; k++) {
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
      if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
        continue;
      if (strncmp(ifa->ifa_name, preferred_prefixes[k],
                  strlen(preferred_prefixes[k])) != 0)
        continue;

      struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
      if (inet_ntop(AF_INET, &sa->sin_addr, buf, buflen)) {
        freeifaddrs(ifaddr);
        return 0;
      }
    }
  }

  for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
      continue;

    struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
    char tmp[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &sa->sin_addr, tmp, sizeof(tmp)))
      continue;

    if (strcmp(tmp, "127.0.0.1") != 0) {
      strncpy(buf, tmp, buflen - 1);
      buf[buflen - 1] = '\0';
      freeifaddrs(ifaddr);
      return 0;
    }
  }

  freeifaddrs(ifaddr);
  return -1;
}

void get_addr(char *hostbuf, size_t buf) {
  if (!hostbuf) {
    printf("Erro hostbuf é null (get_addr)");
    return;
  }

  const char *adv = getenv("ADVERTISE_ADDR");
  if (adv && adv[0]) {
    strncpy(hostbuf, adv, buf - 1);
    hostbuf[buf - 1] = '\0';
    fprintf(stderr, "Using ADVERTISE_ADDR=%s\n", hostbuf);
  } else {
    /* try to auto-detect first non-loopback IPv4 address using helper */
    if (detect_ip(hostbuf, buf) == 0) {
      fprintf(stderr, "Auto-detected advertised IP: %s\n", hostbuf);
    } else {
      /* fallback to hostname (may not be an IP) */
      if (gethostname(hostbuf, buf - 1) != 0) {
        perror("gethostname");
        strncpy(hostbuf, "127.0.0.1", buf - 1);
        hostbuf[buf - 1] = '\0';
      } else {
        fprintf(stderr, "Using hostname as advertised address: %s\n", hostbuf);
      }
    }
  }
}

void update(zoo_string *children_list) {
  pthread_mutex_lock(&rls_lock);
  // printf("Número de filhos: %d\n", children_list->count);

  if (children_list->count == 0) {
    // nenhum servidor
    pthread_mutex_unlock(&rls_lock);
    return;
  }

  if (rl_next_server != NULL) {
    if (rlist_disconnect(rl_next_server) == -1) {
      printf("Erro ao desconectar do último servidor\n");
    } else {
      printf("Desconectar do servidor\n");
    }

    rl_next_server = NULL;
  }

  if (children_list->count > 1) {
    int next = INT_MAX;
    int nextIndex = -1;
    int lower;

    if (sscanf(node_path, "/chain/node-%d", &lower) != 1) {
      fprintf(stderr, "Não foi possível obter o valor do node\n");
    }

    for (int i = 0; i < children_list->count; i++) {
      int num;
      if (sscanf(children_list->data[i], "node-%d", &num) == 1) {
        if (lower < num && num < next) {
          next = num;
          nextIndex = i;
        }
      }
    }

    if (nextIndex != -1) {
      char *node_prev = children_list->data[nextIndex];
      char buffer[ZDATALEN];
      int buffer_len = sizeof(buffer);
      struct Stat stat;

      char zk_path[1024] = "/chain/";
      snprintf(zk_path, sizeof(zk_path), "/chain/%s", node_prev);

      int rc = zoo_get(zh, zk_path, 0, buffer, &buffer_len, &stat);
      if (rc != ZOK) {
        fprintf(stderr, "Erro ao obter %s: %s (%d)\n", zk_path, zerror(rc), rc);
        pthread_mutex_unlock(&rls_lock);
        return;
      }

      if (buffer_len >= (int)sizeof(buffer))
        buffer_len = sizeof(buffer) - 1;
      buffer[buffer_len] = '\0';
      buffer[strcspn(buffer, "\r\n")] = '\0';

      printf("A conectar ao servidor %s\n", buffer);
      rl_next_server = rlist_connect(buffer);
      if (rl_next_server == NULL) {
        if (errno == EBUSY) {
          printf("Server está ocupado. Tente mais tarde\n");
        } else {
          printf("Erro ao conectar\n");
        }
      } else {
        printf("Conectado ao servidor %s\n", buffer);
      }
    }
  }

  pthread_mutex_unlock(&rls_lock);
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
  if (state == ZOO_CONNECTED_STATE && type == ZOO_CHILD_EVENT) {
    zoo_string children_list = {0};
    int rc = zoo_wget_children(zh, root_path, child_watcher, watcher_ctx,
                               &children_list);
    if (rc != ZOK) {
      fprintf(stderr, "Erro ao definir watch em %s! Código: %d\n", root_path, rc);
      /* garante que se alguma coisa foi alocada, libertamos */
      deallocate_String_vector(&children_list);
      return;
    }

    update(&children_list);
    deallocate_String_vector(&children_list);
  }
}
int _op_success(MessageT *msg, int opcode, int ctype) {
  return msg != NULL && msg->opcode == (opcode + 1) &&
         msg->opcode != MESSAGE_T__OPCODE__OP_BAD && msg->c_type == ctype;
}

int rlist_get_list_add(struct rlist_t *rlist, struct list_t *list) {
  if (rlist == NULL) {
    return -1;
  }

  MessageT msg = MESSAGE_T__INIT;
  msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
  msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
  msg.result = -1;

  MessageT *res = network_send_receive(rlist, &msg);
  if (res == NULL) {
    /* erro de rede / sem resposta */
    return -1;
  }

  if (!_op_success(res, MESSAGE_T__OPCODE__OP_GETLISTBYTEAR,
                   MESSAGE_T__C_TYPE__CT_LIST)) {
    message_t__free_unpacked(res, NULL);
    return -1;
  }

  if (res->n_cars == 0) {
    message_t__free_unpacked(res, NULL);
    return 0;
  }

  Data **res_cars = res->cars;

  for (int i = 0; i < res->n_cars; i++) {
    struct data_t *car =
        data_create(res_cars[i]->ano, res_cars[i]->preco, res_cars[i]->marca,
                    res_cars[i]->modelo, res_cars[i]->combustivel);
    if (!car) {
      message_t__free_unpacked(res, NULL);
      /* Se falhar no meio, decide como tratar: limpar a lista ou deixar o caller lidar.
         Aqui retornamos erro e assumimos que o caller saberá como lidar. */
      return -1;
    }

    /* list_add passa a ser proprietária de car — não destrua car aqui */
    if (list_add(list, car) != 0) {
      /* se list_add falhar, liberta car para evitar fuga */
      data_destroy(car);
      message_t__free_unpacked(res, NULL);
      return -1;
    }
  }

  message_t__free_unpacked(res, NULL);
  return 0;
}

int init_list(zoo_string *children_list, struct list_t *list) {
  if (children_list->count <= 1) {
    return 1;
  }

  struct rlist_t *rl = NULL;

  int prev = INT_MIN;
  int prevIndex = -1;
  int maximum;

  if (sscanf(node_path, "/chain/node-%d", &maximum) != 1) {
    fprintf(stderr, "Não foi possível obter o valor do node\n");
  }

  for (int i = 0; i < children_list->count; i++) {
    int num;
    if (sscanf(children_list->data[i], "node-%d", &num) == 1) {
      if (num < maximum && prev < num) {
        prev = num;
        prevIndex = i;
      }
    }
  }

  if (prevIndex != -1) {
    char *node_prev = children_list->data[prevIndex];
    char buffer[ZDATALEN];
    int buffer_len = sizeof(buffer);
    struct Stat stat;

    char zk_path[1024] = "/chain/";
    snprintf(zk_path, sizeof(zk_path), "/chain/%s", node_prev);

    int rc = zoo_get(zh, zk_path, 0, buffer, &buffer_len, &stat);
    if (rc != ZOK) {
      fprintf(stderr, "Erro ao obter %s: %s (%d)\n", zk_path, zerror(rc), rc);
      return -1;
    }

    if (buffer_len >= (int)sizeof(buffer))
      buffer_len = sizeof(buffer) - 1;
    buffer[buffer_len] = '\0';
    buffer[strcspn(buffer, "\r\n")] = '\0';

    printf("Conectar ao servidor %s e obter os dados\n", buffer);

    rl = rlist_connect(buffer);
    if (rl == NULL) {
      if (errno == EBUSY) {
        printf("Server busy. Try again later.\n");
        return -1;
      } else {
        printf("Erro ao conectar ao último servidor\n");
        return -1;
      }
    }

    if (rlist_get_list_add(rl, list) == -1) {
      rlist_disconnect(rl);
      return -1;
    }
    if (rlist_disconnect(rl) == -1) {
      printf("Erro ao desconectar do último servidor\n");
      return -1;
    }
  }

  return 1;
}

int zoo_forward_to_successor(MessageT *msg) {
  if (!msg)
    return -1;

  pthread_mutex_lock(&rls_lock);

  // Não existe sucessor para enviar a mensagem a returnar o código -2
  if (!rl_next_server) {
    pthread_mutex_unlock(&rls_lock);
    return -2;
  }

  MessageT *resp = network_send_receive(rl_next_server, msg);
  int ret = resp ? 0 : -1;
  if (!resp) {
    fprintf(
        stderr,
        "Houve um erro ao tentar enviar a mensagem para o próximo servidor\n");
  }
  if (resp) {
    message_t__free_unpacked(resp, NULL);
  }

  pthread_mutex_unlock(&rls_lock);

  return ret;
}

int zoo_init_server(char *hp, int port, struct list_t *list) {
  if (!hp) {
    fprintf(stderr, "Host é null");
    return -1;
  }

  // Remover as mensagens de debug
  zoo_set_debug_level((ZooLogLevel)0);

  // Iniciar zk
  zh = zookeeper_init(hp, connection_watcher, 30000, 0, 0, 0);
  if (zh == NULL) {
    fprintf(stderr, "Erro ao conectar ao ZooKeeper! Código: %d\n", errno);
    return -1;
  }

  int rc;

  // Verificar se o nó /chain existe. Se não existir criá-lo
  if (zoo_exists(zh, root_path, 0, NULL) == ZNONODE) {
    rc = zoo_create(zh, root_path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    if (rc == ZOK) {
      printf("Nó %s criado com sucesso!\n", root_path);
    } else {
      fprintf(stderr, "Erro ao criar %s! Código: %d\n", root_path, rc);
      return -1;
    }
  }

  // Obter o ip e criar um nó no zookeeper e guardar nesse node os dados ip:port
  char hostbuf[256] = {0};
  get_addr(hostbuf, sizeof(hostbuf));

  char regdata[512];
  int rn = snprintf(regdata, sizeof(regdata), "%s:%d", hostbuf, port);
  if (rn < 0 || (size_t)rn >= sizeof(regdata)) {
    fprintf(stderr, "Erro o endereço é demasiado longo (zoo_init_server)\n");
    return -1;
  }

  if (zoo_create(zh,
                 "/chain/node-",       // prefixo; ZooKeeper completa com número
                 regdata,              // dados do znode
                 strlen(regdata),      // tamanho dos dados
                 &ZOO_OPEN_ACL_UNSAFE, // ACL simples
                 ZOO_EPHEMERAL | ZOO_SEQUENCE, node_path,
                 sizeof(node_path)) != ZOK) {
    fprintf(stderr, "Erro ao criar o nó do servidor\n");
    return -1;
  } else {
    printf("Criado no zookeeper com o nó: %s\n", node_path);
  }

  // Criar o watcher para os filhos de /chain
  zoo_string children_list = {0};
  rc = zoo_wget_children(zh, root_path, child_watcher, watcher_ctx,
                         &children_list);
  if (rc != ZOK) {
    fprintf(stderr, "Erro ao configurar watcher inicial! Código: %d\n", rc);
    return -1;
  } else {
    update(&children_list);

    // Na primeira vez que ligamos é necessário perguntar, caso haja, ao
    // servidor anterior Pela sua lista e inserir na nossa lista
    if (init_list(&children_list, list) < 0) {
      fprintf(stderr, "Erro ao iniciar a list a partir do zookeeper");
      return -1;
    }
  }

  deallocate_String_vector(&children_list);

  return 1;
}

void zoo_clean_server() {
  if (!zh)
    return;

  pthread_mutex_lock(&rls_lock);

  if (rl_next_server != NULL) {
    if (rlist_disconnect(rl_next_server) == -1) {
      printf("Erro ao desconectar do último servidor\n");
    }
    rl_next_server = NULL;
  }

  pthread_mutex_unlock(&rls_lock);

  zookeeper_close(zh);
  zh = NULL;
}

int zoo_port() { return server_port; }