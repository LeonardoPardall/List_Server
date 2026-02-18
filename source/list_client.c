/* Source file: list_client.c */

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

#include "chain_client.h"
#include "client_stub.h"

// Lista de comandos
#define CMD_add "add"
#define CMD_remove "remove"
#define CMD_get_by_year "get_by_year"
#define CMD_get_by_marca "get_by_marca"
#define CMD_get_list_ordered_by_year "get_list_ordered_by_year"
#define CMD_size "size"
#define CMD_get_model_list "get_model_list"
#define CMD_quit "quit"
#define CMD_help "help"

#define BUFFER_SIZE 1024

static volatile sig_atomic_t g_client_shutdown = 0;

static void handle_sigint(int sig) {
  (void)sig;
  g_client_shutdown = 1;
}

static int parse_int_str(const char *s, int *out) {
  if (!s)
    return -1;
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0')
    return -1;
  if (v < INT_MIN || v > INT_MAX)
    return -1;
  *out = (int)v;
  return 0;
}

void cmd_help() {
  printf("Comandos disponíveis: \n");
  printf("  add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3> \n");
  printf("  remove <modelo> \n");
  printf("  get_by_marca <marca:0-4> \n");
  printf("  get_by_year <ano> \n");
  printf("  get_list_ordered_by_year \n");
  printf("  size \n");
  printf("  get_model_list \n");
  printf("  help \n");
  printf("  quit \n");
}

void cmd_size(struct rlist_t *remote_list) {
  int res = rlist_size(remote_list);

  if (res == -1) {
    printf("Erro ao obter número de carros na lista\n");
    return;
  }

  printf("List size: %d\n", res);
}

void print_car(struct data_t *car) {
  if (!car)
    return;
  printf("Modelo: %s \nAno: %d \nPreco: %.2f \nMarca: %u \nCombustivel: %u\n",
         car->modelo ? car->modelo : "(null)", car->ano, car->preco, car->marca,
         car->combustivel);
}

int main(int argc, char *argv[]) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) < 0) {
    printf("sigaction\n");
    return -1;
  }

  if (argc != 2) {
    printf("Argumentos inválidos!\nUso: list_client <zk_host:port> \n");
    return -1;
  }

  struct rlist_t *rlh = NULL;
  struct rlist_t *rlt = NULL;
  pthread_mutex_t rls_lock = PTHREAD_MUTEX_INITIALIZER;

  // Conectar e tratar do zookeeper
  if (zoo_init_client(&rlh, &rlt, argv[1], &rls_lock) < 0) {
    printf("Erro ao conectar ao zookeeper a terminar\n");
    zoo_clean_client();
    return EXIT_FAILURE;
  }

  // buffer de input tamanho BUFFER_SIZE
  char input[BUFFER_SIZE];

  cmd_help();

  while (1) {
    printf("Command: ");

    if (!fgets(input, BUFFER_SIZE, stdin)) {
      if (g_client_shutdown) {
        break;
      }
      continue;
    }

    if (g_client_shutdown) {
      break;
    }

    input[strcspn(input, "\n")] = 0;

    if (strcmp(CMD_quit, input) == 0) {
      break;
    }

    char *cmd = strtok(input, " ");
    if (!cmd)
      continue;

    if (strcmp(cmd, CMD_help) == 0) {
      cmd_help();
      continue;
    }
    if (strcmp(cmd, CMD_size) == 0) {
      pthread_mutex_lock(&rls_lock);
      cmd_size(rlt);
      pthread_mutex_unlock(&rls_lock);
      continue;
    }

    if (strcmp(cmd, CMD_add) == 0) {
      char *modelo = strtok(NULL, " ");
      char *ano_s = strtok(NULL, " ");
      char *preco_s = strtok(NULL, " ");
      char *marca_s = strtok(NULL, " ");
      char *comb_s = strtok(NULL, " ");
      if (!modelo || !ano_s || !preco_s || !marca_s || !comb_s) {
        printf(
            "Uso: add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
        continue;
      }
      int ano = atoi(ano_s);
      float preco = strtof(preco_s, NULL);
      int marca = atoi(marca_s);
      int comb = atoi(comb_s);
      struct data_t *car = data_create(ano, preco, (enum marca_t)marca, modelo,
                                       (enum combustivel_t)comb);
      if (!car) {
        printf(
            "Uso: add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
        continue;
      }
      pthread_mutex_lock(&rls_lock);
      if (rlist_add(rlh, car) == 0)
        printf("Carro adicionado com sucesso.\n");
      else
        printf("Falha ao adicionar carro\n");
      pthread_mutex_unlock(&rls_lock);

      data_destroy(car);

      continue;
    }

    if (strcmp(cmd, CMD_remove) == 0) {
      char *modelo = strtok(NULL, " ");
      if (!modelo) {
        printf("Uso: remove <modelo>\n");
        continue;
      }
      pthread_mutex_lock(&rls_lock);
      int r = rlist_remove_by_model(rlh, modelo);
      pthread_mutex_unlock(&rls_lock);
      if (r == 0)
        printf("Removido com sucesso\n");
      else if (r == 1)
        printf("Modelo nao encontrado\n");
      else
        printf("Erro a remover\n");
      continue;
    }

    if (strcmp(cmd, CMD_get_by_marca) == 0) {
      char *marca_s = strtok(NULL, " ");
      if (!marca_s) {
        printf("Uso: get_by_marca <marca:0-4>\n");
        continue;
      }
      int marca;
      if (parse_int_str(marca_s, &marca) != 0) {
        printf("Uso: get_by_marca <marca:0-4>\n");
        continue;
      }
      pthread_mutex_lock(&rls_lock);
      struct data_t *car = rlist_get_by_marca(rlt, (enum marca_t)marca);
      pthread_mutex_unlock(&rls_lock);
      if (!car)
        printf("Carro não encontrado.\n");
      else {
        print_car(car);
        data_destroy(car);
      }
      continue;
    }

    if (strcmp(cmd, CMD_get_by_year) == 0) {
      char *ano_s = strtok(NULL, " ");
      if (!ano_s) {
        printf("Uso: get_by_year <ano>\n");
        continue;
      }
      int ano;
      if (parse_int_str(ano_s, &ano) != 0) {
        printf("Uso: get_by_year <ano>\n");
        continue;
      }
      pthread_mutex_lock(&rls_lock);
      struct data_t **cars = rlist_get_by_year(rlt, ano);
      pthread_mutex_unlock(&rls_lock);
      if (!cars) {
        printf("Nenhum carro encontrado com o ano especificado.\n");
        continue;
      }
      for (int i = 0; cars[i] != NULL; i++) {
        print_car(cars[i]);
        data_destroy(cars[i]);
      }
      free(cars);
      continue;
    }

    if (strcmp(cmd, CMD_get_list_ordered_by_year) == 0) {
      pthread_mutex_lock(&rls_lock);
      if (rlist_order_by_year(rlt) != 0) {
        printf("Nenhum carro encontrado.\n");
      }
      pthread_mutex_unlock(&rls_lock);
      continue;
    }

    if (strcmp(cmd, CMD_get_model_list) == 0) {
      pthread_mutex_lock(&rls_lock);
      char **models = rlist_get_model_list(rlt);
      pthread_mutex_unlock(&rls_lock);
      if (!models) {
        printf("Erro a obter lista de modelos\n");
        continue;
      }
      for (int i = 0; models[i] != NULL; i++) {
        printf("Modelo: %s\n", models[i]);
        free(models[i]);
      }
      free(models);
      continue;
    }

    printf("Comando inválido. Escreve 'help' para ajuda. \n");
  }

  if (rlh != NULL) {
    if (rlist_disconnect(rlh) == -1) {
      printf("Erro ao desconectar do último servidor\n");
    }
  }

  if (rlt != NULL) {
    if (rlist_disconnect(rlt) == -1) {
      printf("Erro ao desconectar do último servidor\n");
    }
  }

  // Limpar zookeeper
  zoo_clean_client();

  return 0;
}