/**
 * @file list.h
 * @brief Interface para a estrutura da lista de automóveis.
 *
 * Este ficheiro define as funções necessárias para a
 * manipulação de uma lista encadeada de carros.
 *
#endif
#ifndef _LIST_H
#define _LIST_H /* Módulo list */
#define _LIST_H /* Módulo list */

#include "list-private.h"
#include "data.h"

struct list_t; /* Definida em list-private.h */

/* Cria e inicializa uma nova lista de carros.
 * Retorna a lista ou NULL em caso de erro.
 */
struct list_t *list_create();

/* Elimina a lista, libertando toda a memória ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_destroy(struct list_t *list);

/* Adiciona um novo carro à lista.
 * O carro é inserido na última posição da lista.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_add(struct list_t *list, struct data_t *car);

/* Remove da lista o primeiro carro que corresponda ao modelo indicado.
 * Retorna 0 se encontrou e removeu, 1 se não encontrou, ou -1 em caso de erro.
 */
int list_remove_by_model(struct list_t *list, const char *modelo);

/* Obtém o primeiro carro que corresponda à marca indicada.
 * Retorna ponteiro para os dados ou NULL se não encontrar ou em caso de erro.
 */
struct data_t *list_get_by_marca(struct list_t *list, enum marca_t marca);

/* Obtém um array de ponteiros para carros de um determinado ano.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
struct data_t **list_get_by_year(struct list_t *list, int ano);

/* Ordena a lista de carros por ano de fabrico (crescente).
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_order_by_year(struct list_t *list);

/* Retorna o número de carros na lista ou -1 em caso de erro.
 */
int list_size(struct list_t *list);

/* Constrói um array de strings com os modelos dos carros na lista.
 * O último elemento do array é NULL.
 * Retorna o array ou NULL em caso de erro.
 */
char **list_get_model_list(struct list_t *list);

/* Liberta a memória ocupada pelo array de modelos.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_free_model_list(char **models);

/* Devolve um array de ponteiros (terminado em NULL) para TODOS os carros.
 * Cada elemento do array aponta para dados internos da lista (não duplicados).
 * Cabe a quem chama libertar APENAS o array (free(result)), nunca os data_t*.
 * Retorna NULL em caso de erro.
 */
struct data_t **list_get_all(struct list_t *list);

#endif
