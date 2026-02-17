/*
 * Grupo nº 07
 * Afonso Henriques - 61826
 * Leonardo Pardal - 61836
 * Pedro Carvalho - 61800
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "list_skel.h"
#include "message-private.h"
#include "network_server.h"
#include "sdmessage.pb-c.h"
#include <pthread.h>

#include "chain_server.h"

static volatile sig_atomic_t server_shutdown = 0;
static int listen_fd = -1;

/////////////////////////////
// Server log
// As funções create_server_log e close_server_log, ambas precisam de ser
// chamadas pelo menos uma vez
/////////////////////////////

// Criar uma lista de mensagens e uma thread que pegue nestas mensagens e
// escreva no server.log
static int filed = 0;
static pthread_cond_t cond_log;
static pthread_t writer_thread;
static int writer_running = 0;

/* lista ligada para as linhas de log */
struct log_node {
  char *str;
  struct log_node *next;
};

static struct log_node *log_tail = NULL;
static struct log_node *log_head = NULL;

/* mutex usado pelo logger (deve ser inicializado em create_server_log) */
static pthread_mutex_t lock_log;

/* types de eventos que podem ser escritos no server.log */
enum event_type {
  CONNECT,
  REQUEST,
  CLOSE,
};

void write_in_server_log(int client_socket, enum event_type eventType,
                         MessageT *msg) {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0) {
    perror("Erro ao obter timestamp (write_in_server_log)");
    return;
  }
  long ts = (long)tv.tv_sec;

  char addrbuf[64] = "UNKNOWN:0";
  if (client_socket >= 0) {
    struct sockaddr_in peer;
    socklen_t plen = sizeof(peer);
    if (getpeername(client_socket, (struct sockaddr *)&peer, &plen) == 0) {
      char ip[INET_ADDRSTRLEN];
      if (inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip)) != NULL) {
        snprintf(addrbuf, sizeof(addrbuf), "%s:%d", ip, ntohs(peer.sin_port));
      }
    }
  }

  const char *etype_str = "UNKNOWN";
  switch (eventType) {
  case CONNECT:
    etype_str = "CONNECT";
    break;
  case REQUEST:
    etype_str = "REQUEST";
    break;
  case CLOSE:
    etype_str = "CLOSE";
    break;
  }

  char *final_line = NULL;
  if (msg) {
    /* obter strings opcode / c_type */
    const char *op_str = "UNKNOWN";
    const char *ct_str = "UNKNOWN";
    switch (msg->opcode) {
    case 10:
      op_str = "OP_ADD";
      break;
    case 20:
      op_str = "OP_GET";
      break;
    case 30:
      op_str = "OP_DEL";
      break;
    case 40:
      op_str = "OP_SIZE";
      break;
    case 50:
      op_str = "OP_GETMODELS";
      break;
    case 60:
      op_str = "OP_GETLISTBYTEAR";
      break;
    case 70:
      op_str = "OP_ORDER";
      break;
    case 99:
      op_str = "OP_ERROR";
      break;
    case 100:
      op_str = "OP_BUSY";
      break;
    case 101:
      op_str = "OP_READY";
      break;
    default:
      break;
    }
    switch (msg->c_type) {
    case 10:
      ct_str = "CT_DATA";
      break;
    case 20:
      ct_str = "CT_MARCA";
      break;
    case 30:
      ct_str = "CT_YEAR";
      break;
    case 40:
      ct_str = "CT_MODEL";
      break;
    case 50:
      ct_str = "CT_RESULT";
      break;
    case 60:
      ct_str = "CT_LIST";
      break;
    case 70:
      ct_str = "CT_NONE";
      break;
    default:
      break;
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_ADD) {
      const char *modelo =
          (msg->data && msg->data->modelo) ? msg->data->modelo : "-";
      int marca = (msg->data && msg->data->marca) ? msg->data->marca : 0;
      int comb =
          (msg->data && msg->data->combustivel) ? msg->data->combustivel : 0;
      int ano = (msg->data) ? msg->data->ano : 0;
      double preco = (msg->data) ? msg->data->preco : 0.0;

      size_t need =
          snprintf(NULL, 0, "%ld %s %s %s %s %s %d %.2f %d %d\n", ts, addrbuf,
                   etype_str, op_str, ct_str, modelo, ano, preco, marca, comb) +
          1;
      final_line = malloc(need);
      if (!final_line) {
        perror("Erro ao alocar memória (write_in_server_log)");
        return;
      }
      snprintf(final_line, need, "%ld %s %s %s %s %s %d %.2f %d %d\n", ts,
               addrbuf, etype_str, op_str, ct_str, modelo, ano, preco, marca,
               comb);
    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_DEL) {
      const char *mode = (*(msg->models)) ? *(msg->models) : "-";

      size_t need = snprintf(NULL, 0, "%ld %s %s %s %s %s\n", ts, addrbuf,
                             etype_str, op_str, ct_str, mode) +
                    1;
      final_line = malloc(need);
      if (!final_line) {
        perror("Erro ao alocar memória (write_in_server_log)");
        return;
      }
      snprintf(final_line, need, "%ld %s %s %s %s %s\n", ts, addrbuf, etype_str,
               op_str, ct_str, mode);
    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_GET) {
      int marca = (msg->result) ? msg->result : 0;

      size_t need = snprintf(NULL, 0, "%ld %s %s %s %s %d\n", ts, addrbuf,
                             etype_str, op_str, ct_str, marca) +
                    1;
      final_line = malloc(need);
      if (!final_line) {
        perror("Erro ao alocar memória (write_in_server_log)");
        return;
      }
      snprintf(final_line, need, "%ld %s %s %s %s %d\n", ts, addrbuf, etype_str,
               op_str, ct_str, marca);
    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_GETLISTBYTEAR) {
      int ano = (msg->result) ? msg->result : 0;

      size_t need = snprintf(NULL, 0, "%ld %s %s %s %s %d\n", ts, addrbuf,
                             etype_str, op_str, ct_str, ano) +
                    1;
      final_line = malloc(need);
      if (!final_line) {
        perror("Erro ao alocar memória (write_in_server_log)");
        return;
      }
      snprintf(final_line, need, "%ld %s %s %s %s %d\n", ts, addrbuf, etype_str,
               op_str, ct_str, ano);
    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_GETMODELS ||
               msg->opcode == MESSAGE_T__OPCODE__OP_SIZE) {
      size_t need = snprintf(NULL, 0, "%ld %s %s %s %s\n", ts, addrbuf,
                             etype_str, op_str, ct_str) +
                    1;
      final_line = malloc(need);
      if (!final_line) {
        perror("Erro ao alocar memória (write_in_server_log)");
        return;
      }
      snprintf(final_line, need, "%ld %s %s %s %s\n", ts, addrbuf, etype_str,
               op_str, ct_str);
    }
  } else {
    size_t need = snprintf(NULL, 0, "%ld %s %s\n", ts, addrbuf, etype_str) + 1;
    final_line = malloc(need);
    if (!final_line) {
      perror("Erro ao alocar memória (write_in_server_log)");
      return;
    }
    snprintf(final_line, need, "%ld %s %s\n", ts, addrbuf, etype_str);
  }

  /* enfileirar linha final_line */
  struct log_node *new_node = malloc(sizeof *new_node);
  if (!new_node) {
    perror("Erro ao alocar nó (write_in_server_log)");
    free(final_line);
    return;
  }
  new_node->str = final_line;
  new_node->next = NULL;

  pthread_mutex_lock(&lock_log);
  if (log_tail == NULL) {
    log_head = new_node;
    log_tail = new_node;
  } else {
    log_tail->next = new_node;
    log_tail = new_node;
  }
  pthread_cond_signal(&cond_log);
  pthread_mutex_unlock(&lock_log);
}

void *handle_writing(void *arg) {
  (void)arg;
  while (1) {
    // fechar o acesso ao server.log
    pthread_mutex_lock(&lock_log);
    while (log_head == NULL && writer_running) {
      // caso não haja nada para ler remover o lock e esperar que haja algo
      pthread_cond_wait(&cond_log, &lock_log);
    }
    if (log_head == NULL && !writer_running) {
      // caso não haja nada para ler e a thread deve morrer, terminar
      pthread_mutex_unlock(&lock_log);
      break;
    }

    struct log_node *node = log_head;
    log_head = node->next;
    if (log_head == NULL)
      log_tail = NULL;

    pthread_mutex_unlock(&lock_log);

    if (node && node->str) {
      size_t to_write = strlen(node->str);
      ssize_t w = 0;
      char *p = node->str;
      while (to_write > 0) {
        w = write(filed, p, to_write);
        if (w < 0) {
          if (errno == EINTR)
            continue;
          perror("Não foi possivel escrever no ficheiro server.log "
                 "(handle_writing)");
          break;
        }
        to_write -= (size_t)w;
        p += w;
      }
    }
    free(node->str);
    free(node);
  }
  return NULL;
}

int create_server_log(int port) {
  // Criar ficheiro server.log
  char hostbuf[256] = {0};
  get_addr(hostbuf, sizeof(hostbuf));

  char filename[256];
  int rn = snprintf(filename, sizeof(filename), "server_%s_%d.log", hostbuf, port);
  if (rn < 0 || rn >= (int)sizeof(filename)) {
      fprintf(stderr, "Erro: nome do ficheiro demasiado longo\n");
      return -1;
  }

  filed = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (filed < 0) {
    perror("Não foi possivel criar o ficheiro de log");
    return -1;
  }

  pthread_mutex_init(&lock_log, NULL);
  pthread_cond_init(&cond_log, NULL);
  writer_running = 1;
  if (pthread_create(&writer_thread, NULL, handle_writing, NULL) != 0) {
    perror("Erro ao criar thread (create_server_log)");
    writer_running = 0;
    return -1;
  }

  return 1;
}

int close_server_log() {
  // sinalizar thread para terminar e aguardar
  pthread_mutex_lock(&lock_log);
  writer_running = 0;
  pthread_cond_signal(&cond_log);
  pthread_mutex_unlock(&lock_log);
  pthread_join(writer_thread, NULL);

  // liberar nós restantes
  pthread_mutex_lock(&lock_log);
  struct log_node *cur = log_head;
  while (cur) {
    struct log_node *n = cur->next;
    free((void *)cur->str);
    free(cur);
    cur = n;
  }
  log_head = log_tail = NULL;
  pthread_mutex_unlock(&lock_log);

  pthread_mutex_destroy(&lock_log);
  pthread_cond_destroy(&cond_log);

  return close(filed);
}

/////////////////////////////////////////
/////////////////////////////////////////
/////////////////////////////////////////

#define NUM_CLIENTS 5
#define NUM_MAX_CLIENTS (NUM_CLIENTS+1)

pthread_mutex_t lock_num_clients;
pthread_mutex_t lock_list;
int num_clients = 0;

struct cl {
  int client_socket;
  pthread_t tid;
};
struct cl client_list[NUM_MAX_CLIENTS];

int network_server_init(short port) {
  memset(&client_list, -1, NUM_MAX_CLIENTS * sizeof(struct cl));

  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
  
  int yes = 1;
  (void)setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  signal(SIGPIPE, SIG_IGN);
  struct sockaddr_in adress;
  memset(&adress, 0, sizeof(adress));         // limpar estrutura
  adress.sin_family = AF_INET;                // IPv4
  adress.sin_addr.s_addr = htonl(INADDR_ANY); // aceitar ligações
  adress.sin_port = htons(port);              // port

  if (bind(sock, (struct sockaddr *)&adress, sizeof(adress)) < 0) {
    perror("bind");
    close(sock);
    return -1;
  }
  
  if (listen(sock, SOMAXCONN) < 0) {
    perror("listen");
    close(sock);
    return -1;
  }
  
  listen_fd = sock;

  // Criar server.log
  create_server_log(port);

  return sock;
}

struct client_info {
  int client_socket;
  struct list_t *list;
  char *ip_port; /* opcional, pode ficar NULL */
};

/* thread que trata um cliente */
static void *client_thread(void *arg) {
  struct client_info *ci = arg;
  int client_socket = ci->client_socket;
  struct list_t *list = ci->list;

  /* marcar cliente activo */
  pthread_mutex_lock(&lock_num_clients);

  for (int x = 0; x < NUM_MAX_CLIENTS; x++) {
    if (client_list[x].client_socket == -1) {
      client_list[x].client_socket = client_socket;
      client_list[x].tid = pthread_self();
      num_clients++;
      break;
    }
  }

  pthread_mutex_unlock(&lock_num_clients);

  /* registar CONNECT */
  write_in_server_log(client_socket, CONNECT, NULL);

  /* loop de recepção/tratamento */
  while (!server_shutdown) {
    MessageT *request = network_receive(client_socket);
    if (!request)
      break;

    write_in_server_log(client_socket, REQUEST, request);

    int needs_forward = request->opcode == MESSAGE_T__OPCODE__OP_ADD ||
                        request->opcode == MESSAGE_T__OPCODE__OP_DEL ||
                        request->opcode == MESSAGE_T__OPCODE__OP_ORDER;

    if (needs_forward) {
      /* Para garantir atomicidade das operações de modificação,
         mantemos o lock da lista durante o forward e a aplicação local. */
      pthread_mutex_lock(&lock_list);

      /* Tenta reenviar o pedido atual para o sucessor */
      int fwd_rc = zoo_forward_to_successor(request);

      if (fwd_rc == -2) {
        /* É o último servidor — executa localmente */
        fprintf(stderr,
                "Não existe sucessor. A executar operação localmente\n");
        invoke(request, list);

      } else if (fwd_rc != 0) {
        /* Erro no forward — não altera estado local */
        fprintf(stderr, "A operação do sucessor: %d -> a devolver erro\n",
                fwd_rc);
        request->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        request->result = -1;

      } else {
        /* Forward OK → agora aplica localmente */
        fprintf(stderr, "A operação do sucessor foi um sucesso. A executar a "
                        "operação localmente \n");
        invoke(request, list);
      }

      pthread_mutex_unlock(&lock_list);
    } else {
      /* Operações não mutáveis, apenas executa localmente */
      pthread_mutex_lock(&lock_list);
      invoke(request, list);
      pthread_mutex_unlock(&lock_list);
    }

    /* Responder ao cliente */
    if (network_send(client_socket, request) < 0) {
      message_t__free_unpacked(request, NULL);
      break;
    }

    message_t__free_unpacked(request, NULL);
  }

  write_in_server_log(client_socket, CLOSE, NULL);
  close(client_socket);

  /* marcar cliente como inactivo */
  pthread_mutex_lock(&lock_num_clients);
  num_clients--;

  for (int x = 0; x < NUM_MAX_CLIENTS; x++) {
    if (client_list[x].client_socket == client_socket &&
        client_list[x].tid == pthread_self()) {
      client_list[x].client_socket = -1;
    }
  }

  pthread_mutex_unlock(&lock_num_clients);

  free(ci->ip_port);
  free(ci);
  return NULL;
}

int network_main_loop(int listening_socket, struct list_t *list) {
  listen_fd = listening_socket;
  printf("Server ready, waiting for connections\n");
  while (!server_shutdown) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket =
        accept(listening_socket, (struct sockaddr *)&client_addr, &client_len);
    if (server_shutdown) {
      break;
    }
    if (client_socket < 0) {
      if (errno == EINTR)
        continue;
      if (server_shutdown || errno == EBADF) {
        break;
      }
      perror("Erro ao estabelecer ligação (network_server)");
      return -1;
    }
    /* limitar número de clientes activos */
    pthread_mutex_lock(&lock_num_clients);
    if (num_clients >= NUM_MAX_CLIENTS) {
      MessageT msg = MESSAGE_T__INIT;
      msg.opcode = MESSAGE_T__OPCODE__OP_BUSY;
      network_send(client_socket, &msg);

      pthread_mutex_unlock(&lock_num_clients);
      close(client_socket);
      continue;
    }
    pthread_mutex_unlock(&lock_num_clients);

    /* preparar dados para a thread */
    struct client_info *ci = malloc(sizeof(*ci));
    if (!ci) {
      MessageT msg = MESSAGE_T__INIT;
      msg.opcode = MESSAGE_T__OPCODE__OP_BUSY;
      perror("Erro ao alocar client_info (network_main_loop)");
      network_send(client_socket, &msg);
      close(client_socket);
      continue;
    }

    ci->client_socket = client_socket;
    ci->list = list;
    ci->ip_port = NULL; /* opcional */

    pthread_t tid;
    if (pthread_create(&tid, NULL, client_thread, ci) != 0) {
      perror("Erro ao criar thread (network_main_loop)");
      free(ci);
      MessageT msg = MESSAGE_T__INIT;
      msg.opcode = MESSAGE_T__OPCODE__OP_BUSY;
      network_send(client_socket, &msg);
      close(client_socket);
      continue;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_READY;
    network_send(client_socket, &msg);

    /* não precisamos de pthread_join: detach para não acumular recursos */
    pthread_detach(tid);
  }
  return 0;
}

MessageT *network_receive(int client_socket) {
  uint16_t net_len;
  if (read_all(client_socket, &net_len, sizeof(net_len)) != sizeof(net_len)) {
    return NULL;
  }
  uint16_t len = ntohs(net_len);
  if (len == 0) {
    fprintf(stderr, "mensagem com comprimento 0 (network_server)\n");
    return NULL;
  }
  void *buffer = malloc(len);
  if (!buffer) {
    perror("malloc error");
    return NULL;
  }
  if (read_all(client_socket, buffer, len) != len) {
    perror("Erro ao ler mensagem (network_server)");
    free(buffer);
    return NULL;
  }
  MessageT *msg = message_t__unpack(NULL, len, buffer);
  free(buffer);
  return msg;
}

int network_send(int client_socket, MessageT *msg) {
  size_t len = message_t__get_packed_size(msg);
  if (len == 0 || len > 0xFFFF) {
    fprintf(stderr, "mensagem com comprimento inválido (network_server)\n");
    return -1;
  }
  uint8_t *buffer = malloc(len);
  if (!buffer) {
    perror("malloc error");
    return -1;
  }
  message_t__pack(msg, buffer);
  uint16_t net_len = htons(len);
  if (write_all(client_socket, &net_len, sizeof(net_len)) != sizeof(net_len)) {
    perror("Erro ao enviar mensagem (network_server)");
    free(buffer);
    return -1;
  }
  if (write_all(client_socket, buffer, len) != len) {
    perror("Erro ao enviar mensagem (network_server)");
    free(buffer);
    return -1;
  }
  free(buffer);
  return 0;
}

int network_server_close(int socket) {
  pthread_mutex_lock(&lock_num_clients);
  for (int x = 0; x < NUM_MAX_CLIENTS; x++) {
    if (client_list[x].client_socket != -1) {
      shutdown(client_list[x].client_socket, SHUT_RDWR);
      pthread_join(client_list[x].tid, NULL);
    }
  }
  pthread_mutex_unlock(&lock_num_clients);

  if (listen_fd >= 0) {
    shutdown(listen_fd, SHUT_RDWR);
  }
  if (socket < 0) {
    fprintf(stderr, "socket inválido (network_server_close)\n");
    return -1;
  }
  int r = close(socket);
  if (r < 0) {
    perror("Erro ao fechar socket (network_server_close)");
  }
  if (socket == listen_fd) {
    listen_fd = -1;
  }

  pthread_mutex_destroy(&lock_list);

  // Fechar o server log de maneira segura
  close_server_log();

  return r;
}

void network_server_request_shutdown() { server_shutdown = 1; }