/* Source file: client_stub.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_stub-private.h"
#include "network_client.h"

void _print_car(struct data_t *car) {
  if (!car)
    return;
  printf("Modelo: %s \nAno: %d \nPreco: %.2f \nMarca: %u \nCombustivel: %u\n",
         car->modelo ? car->modelo : "(null)", car->ano, car->preco, car->marca,
         car->combustivel);
}

static int marca_to_proto(int marca) {
  switch (marca) {
  case MARCA_TOYOTA:
    return MARCA__MARCA_TOYOTA;
  case MARCA_BMW:
    return MARCA__MARCA_BMW;
  case MARCA_RENAULT:
    return MARCA__MARCA_RENAULT;
  case MARCA_AUDI:
    return MARCA__MARCA_AUDI;
  case MARCA_MERCEDES:
    return MARCA__MARCA_MERCEDES;

  default:
    return -1;
  }
}

static int proto_to_marca(int pb) {
  switch (pb) {
  case MARCA__MARCA_TOYOTA:
    return MARCA_TOYOTA;
  case MARCA__MARCA_BMW:
    return MARCA_BMW;
  case MARCA__MARCA_RENAULT:
    return MARCA_RENAULT;
  case MARCA__MARCA_AUDI:
    return MARCA_AUDI;
  case MARCA__MARCA_MERCEDES:
    return MARCA_MERCEDES;
  default:
    return -1;
  }
}

static int combustivel_to_proto(int combustivel) {
  switch (combustivel) {
  case COMBUSTIVEL_GASOLINA:
    return COMBUSTIVEL__COMBUSTIVEL_GASOLINA;
  case COMBUSTIVEL_GASOLEO:
    return COMBUSTIVEL__COMBUSTIVEL_GASOLEO;
  case COMBUSTIVEL_ELETRICO:
    return COMBUSTIVEL__COMBUSTIVEL_ELETRICO;
  case COMBUSTIVEL_HIBRIDO:
    return COMBUSTIVEL__COMBUSTIVEL_HIBRIDO;

  default:
    return -1;
  }
}

static int proto_to_combustivel(int pb) {
  switch (pb) {
  case COMBUSTIVEL__COMBUSTIVEL_GASOLINA:
    return COMBUSTIVEL_GASOLINA;
  case COMBUSTIVEL__COMBUSTIVEL_GASOLEO:
    return COMBUSTIVEL_GASOLEO;
  case COMBUSTIVEL__COMBUSTIVEL_ELETRICO:
    return COMBUSTIVEL_ELETRICO;
  case COMBUSTIVEL__COMBUSTIVEL_HIBRIDO:
    return COMBUSTIVEL_HIBRIDO;
  default:
    return -1;
  }
}

//// DEBUG

static const char *opcode_to_str(int op) {
  switch (op) {
  case MESSAGE_T__OPCODE__OP_BAD:
    return "OP_BAD";
  case MESSAGE_T__OPCODE__OP_SIZE:
    return "OP_SIZE";
  case MESSAGE_T__OPCODE__OP_ADD:
    return "OP_ADD";
  case MESSAGE_T__OPCODE__OP_DEL:
    return "OP_DEL";
  case MESSAGE_T__OPCODE__OP_GET:
    return "OP_GET";
  case MESSAGE_T__OPCODE__OP_GETMODELS:
    return "OP_GETMODELS";
  case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR:
    return "OP_GETLISTBYTEAR";
  default:
    return "OP_UNKNOWN";
  }
}

static const char *ctype_to_str(int ct) {
  switch (ct) {
  case MESSAGE_T__C_TYPE__CT_NONE:
    return "CT_NONE";
  case MESSAGE_T__C_TYPE__CT_RESULT:
    return "CT_RESULT";
  case MESSAGE_T__C_TYPE__CT_MODEL:
    return "CT_MODEL";
  case MESSAGE_T__C_TYPE__CT_MARCA:
    return "CT_MARCA";
  case MESSAGE_T__C_TYPE__CT_DATA:
    return "CT_DATA";
  case MESSAGE_T__C_TYPE__CT_LIST:
    return "CT_LIST";
  default:
    return "CT_UNKNOWN";
  }
}

void data_pb_debug_print(const Data *d, const char *prefix) {
  if (!d) {
    printf("%s Data: (null)\n", prefix ? prefix : "");
    return;
  }
  printf("%s Data { ano=%d, preco=%.2f, marca=%d, modelo=\"%s\", "
         "combustivel=%d }\n",
         prefix ? prefix : "", d->ano, d->preco, d->marca,
         d->modelo ? d->modelo : "(null)", d->combustivel);
}

void message_debug_print(const MessageT *msg, const char *tag) {
  const char *t = tag ? tag : "MSG";
  if (!msg) {
    printf("[%s] MessageT: (null)\n", t);
    return;
  }

  printf("[%s] MessageT { opcode=%d(%s), c_type=%d(%s)\n", t, msg->opcode,
         opcode_to_str(msg->opcode), msg->c_type, ctype_to_str(msg->c_type));

  // result
  printf("  result=%d\n", msg->result);

  // models[]
  if (msg->n_models > 0 && msg->models) {
    printf("  models (n=%zu):\n", msg->n_models);
    for (size_t i = 0; i < msg->n_models; ++i) {
      printf("    - \"%s\"\n", msg->models[i] ? msg->models[i] : "(null)");
    }
  } else {
    printf("  models: (none)\n");
  }

  // data
  if (msg->data) {
    data_pb_debug_print(msg->data, "  ");
  } else {
    printf("  data: (null)\n");
  }

  // cars[]
  if (msg->n_cars > 0 && msg->cars) {
    printf("  cars (n=%zu):\n", msg->n_cars);
    for (size_t i = 0; i < msg->n_cars; ++i) {
      data_pb_debug_print(msg->cars[i], "    ");
    }
  } else {
    printf("  cars: (none)\n");
  }

  printf("}\n");
}

//////

int op_success(MessageT *msg, int opcode, int ctype) {
  return msg != NULL && msg->opcode == (opcode + 1) &&
         msg->opcode != MESSAGE_T__OPCODE__OP_BAD && msg->c_type == ctype;
}

struct rlist_t *rlist_connect(char *address_port) {
  if (address_port == NULL) {
    fprintf(stderr, "address_port é NULL (rlist_connect)\n");
    return NULL;
  }

  char *addr_copy = strdup(address_port);
  if (!addr_copy)
    return NULL;

  char *colon = strchr(addr_copy, ':');
  if (colon == NULL) {
    free(addr_copy);
    return NULL;
  }
  *colon = '\0';
  char *hostname = addr_copy;
  char *port_str = colon + 1;

  int port = (int)atoi(port_str);
  if (port <= 0) {
    fprintf(stderr, "Porta inválida\n");
    free(addr_copy);
    return NULL;
  }

  struct rlist_t *rlist = malloc(sizeof(struct rlist_t));
  if (!rlist) {
    free(addr_copy);
    return NULL;
  }

  rlist->server_address = strdup(hostname);
  free(addr_copy);
  if (!rlist->server_address) {
    free(rlist);
    return NULL;
  }

  rlist->server_port = port;
  int yes = network_connect(rlist);
  if (yes < 0) {
    free(rlist->server_address);
    free(rlist);
    return NULL;
  }

  return rlist;
}

int rlist_disconnect(struct rlist_t *rlist) {
  //printf("addr: %s, port: %d, socket: %d\n", rlist->server_address, rlist->server_port, rlist->sockfd);

  if (rlist == NULL) {
    fprintf(stderr, "rlist é NULL (rlist_disconnect)\n");
    return -1;
  }
  if (close(rlist->sockfd) < 0) {
    perror("Erro ao fechar o socket (rlist_disconnect)");
    free(rlist->server_address);
    free(rlist);
    return -1;
  }
  free(rlist->server_address);
  free(rlist);
  return 0;
}

/* Adiciona um novo carro à lista remota.
 * O carro é inserido na última posição da lista.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_add(struct rlist_t *rlist, struct data_t *car) {
  if (rlist == NULL || car == NULL) {
    return -1;
  }

  int marca = marca_to_proto(car->marca);
  int combustivel = combustivel_to_proto(car->combustivel);

  if (marca == -1 || combustivel == -1) {
    fprintf(stderr,
            "uso: add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
    return -1;
  }

  MessageT msg = MESSAGE_T__INIT;
  Data data = DATA__INIT;

  char *modelo_copy = strdup(car->modelo);
  if (modelo_copy == NULL) {
    return -1;
  }

  data.ano = car->ano;
  data.preco = car->preco;
  data.marca = marca;
  data.modelo = modelo_copy;
  data.combustivel = combustivel;

  msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
  msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;
  msg.data = &data;

  MessageT *res = network_send_receive(rlist, &msg);
  free(modelo_copy);
  if (!op_success(res, MESSAGE_T__OPCODE__OP_ADD, MESSAGE_T__C_TYPE__CT_NONE)) {
    message_t__free_unpacked(res, NULL);
    return -1;
  }
  message_t__free_unpacked(res, NULL);

  return 0;
}

int rlist_remove_by_model(struct rlist_t *rlist, const char *modelo) {
  if (rlist == NULL || modelo == NULL) {
    printf("Uso : remove <modelo>\n");
    return -1;
  }

  MessageT msg = MESSAGE_T__INIT;
  char *modelo_copy = strdup(modelo);
  if (modelo_copy == NULL) {
    return -1;
  }
  msg.models = &modelo_copy;
  msg.n_models = 1;
  msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
  msg.c_type = MESSAGE_T__C_TYPE__CT_MODEL;
  MessageT *res = network_send_receive(rlist, &msg);
  free(modelo_copy);
  if (res->opcode == MESSAGE_T__OPCODE__OP_DEL + 1 &&
      res->c_type == MESSAGE_T__C_TYPE__CT_RESULT) {
    int result = res->result;
    message_t__free_unpacked(res, NULL);
    return result; /* 0 => removed, 1 => not found */
  }
  message_t__free_unpacked(res, NULL);
  return -1;
}

struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca) {
  if (rlist == NULL) {
    return NULL;
  }

  int _marca = marca_to_proto(marca);

  if (_marca == -1) {
    perror("Uso: list_get_by_marca <marca:0-4>\n");
    return NULL;
  }

  MessageT msg = MESSAGE_T__INIT;

  msg.opcode = MESSAGE_T__OPCODE__OP_GET;
  msg.c_type = MESSAGE_T__C_TYPE__CT_MARCA;
  msg.result = _marca;

  MessageT *res = network_send_receive(rlist, &msg);

  if (!op_success(res, MESSAGE_T__OPCODE__OP_GET, MESSAGE_T__C_TYPE__CT_DATA)) {
    message_t__free_unpacked(res, NULL);
    return NULL;
  }

  Data *res_data = res->data;
  struct data_t *data_ = data_create(
      res_data->ano, res_data->preco, proto_to_marca(res_data->marca),
      res_data->modelo, proto_to_combustivel(res_data->combustivel));

  if (!data_) {
    message_t__free_unpacked(res, NULL);
    return NULL;
  }

  message_t__free_unpacked(res, NULL);

  return data_;
}

/* Obtém um array de ponteiros para carros de um determinado ano.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano) {
  if (rlist == NULL) {
    return NULL;
  }

  MessageT msg = MESSAGE_T__INIT;

  msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
  msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
  msg.result = ano;

  MessageT *res = network_send_receive(rlist, &msg);
  if (!op_success(res, MESSAGE_T__OPCODE__OP_GETLISTBYTEAR,
                  MESSAGE_T__C_TYPE__CT_LIST)) {
    message_t__free_unpacked(res, NULL);
    return NULL;
  }

  if (res->n_cars == 0) {
    message_t__free_unpacked(res, NULL);
    return NULL;
  }
  Data **res_cars = res->cars;
  struct data_t **res_data = malloc((res->n_cars + 1) * sizeof(void *));
  if (!res_data) {
    message_t__free_unpacked(res, NULL);
    return NULL;
  }
  for (int i = 0; i < res->n_cars; i++) {
    res_data[i] =
        data_create(res_cars[i]->ano, res_cars[i]->preco, res_cars[i]->marca,
                    res_cars[i]->modelo, res_cars[i]->combustivel);
    if (!res_data[i]) {
      for (int j = 0; j < i; j++) {
        data_destroy(res_data[j]);
      }
      free(res_data);
      message_t__free_unpacked(res, NULL);
      return NULL;
    }
  }
  res_data[res->n_cars] = NULL;
  message_t__free_unpacked(res, NULL);

  return res_data;
}

/* Ordena a lista remota de carros por ano de fabrico (crescente).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_order_by_year(struct rlist_t *rlist) {
  if (rlist == NULL) {
    return -1;
  }

  MessageT msg = MESSAGE_T__INIT;

  msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
  msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
  msg.result = -1;

  MessageT *res = network_send_receive(rlist, &msg);
  if (!op_success(res, MESSAGE_T__OPCODE__OP_GETLISTBYTEAR,
                  MESSAGE_T__C_TYPE__CT_LIST)) {
    message_t__free_unpacked(res, NULL);
    return -1;
  }

  if (res->n_cars == 0) {
    message_t__free_unpacked(res, NULL);
    return -1;
  }
  Data **res_cars = res->cars;
  struct data_t **res_data = malloc((res->n_cars + 1) * sizeof(void *));
  if (!res_data) {
    message_t__free_unpacked(res, NULL);
    return -1;
  }
  for (int i = 0; i < res->n_cars; i++) {
    res_data[i] =
        data_create(res_cars[i]->ano, res_cars[i]->preco, res_cars[i]->marca,
                    res_cars[i]->modelo, res_cars[i]->combustivel);
    if (!res_data[i]) {
      for (int j = 0; j < i; j++) {
        data_destroy(res_data[j]);
      }
      free(res_data);
      message_t__free_unpacked(res, NULL);
      return -1;
    }
  }
  res_data[res->n_cars] = NULL;
  message_t__free_unpacked(res, NULL);

  if (!res_data) {
    return -1;
  }
  for (int i = 0; res_data[i] != NULL; i++) {
    _print_car(res_data[i]);
    data_destroy(res_data[i]);
  }
  free(res_data);

  return 0;
}

/* Retorna o número de carros na lista remota ou -1 em caso de erro.
 */
int rlist_size(struct rlist_t *rlist) {
  if (rlist == NULL) {
    return -1;
  }

  MessageT msg = MESSAGE_T__INIT;
  // Data data = DATA__INIT;
  // msg.data = &data;

  msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
  msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

  MessageT *res = network_send_receive(rlist, &msg);
  if (!op_success(res, MESSAGE_T__OPCODE__OP_SIZE,
                  MESSAGE_T__C_TYPE__CT_RESULT)) {
    message_t__free_unpacked(res, NULL);
    return -1;
  }

  int result = res->result;

  message_t__free_unpacked(res, NULL);

  return result;
}

/* Constrói um array de strings com os modelos dos carros na lista remota.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
char **rlist_get_model_list(struct rlist_t *rlist) {
  if (rlist == NULL) {
    return NULL;
  }

  MessageT msg = MESSAGE_T__INIT;
  // Data data = DATA__INIT;

  msg.opcode = MESSAGE_T__OPCODE__OP_GETMODELS;
  msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

  MessageT *res = network_send_receive(rlist, &msg);
  if (!op_success(res, MESSAGE_T__OPCODE__OP_GETMODELS,
                  MESSAGE_T__C_TYPE__CT_MODEL)) {
    message_t__free_unpacked(res, NULL);
    return NULL;
  }

  char **res_models = res->models;
  char **res_data = malloc((res->n_models + 1) * sizeof(char *));
  for (int i = 0; i < res->n_models; i++) {
    res_data[i] = strdup(res_models[i]);
  }
  res_data[res->n_models] = NULL;
  message_t__free_unpacked(res, NULL);

  return res_data;
}

/* Liberta a memória ocupada pelo array de modelos.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int rlist_free_model_list(char **models) {
  return list_free_model_list(models);
}
