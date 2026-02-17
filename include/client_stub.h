/**
* @file list.h
* @brief Interface para a estrutura da lista de automóveis.
*
* Este ficheiro define as funções necessárias para a
* manipulação de uma lista encadeada de carros.
*
* Projeto: Sistemas Distribuídos 2025/2026
* Autor: José Cecílio
* Data: 13/09/2025
*/
#ifndef _CLIENT_STUB_H
#define _CLIENT_STUB_H

#include "data.h"
#include "list.h"

/* Remote list, deve conter as informações necessárias para comunicar
* com o servidor. A definir pelo grupo em client_stub-private.h
*/

struct rlist_t; /* Definida em list-private.h */

/* Função para estabelecer uma associação entre o cliente e o servidor,
* em que address_port é uma string no formato <hostname>:<port>.
* Retorna a estrutura rlist preenchida, ou NULL em caso de erro.
*/
struct rlist_t *rlist_connect(char *address_port);

/* Termina a associação entre o cliente e o servidor, fechando a
* ligação com o servidor e libertando toda a memória local.
* Retorna 0 se tudo correr bem, ou -1 em caso de erro.
*/
int rlist_disconnect(struct rlist_t *rlist);

/* Adiciona um novo carro à lista remota.
* O carro é inserido na última posição da lista.
* Retorna 0 (OK) ou -1 em caso de erro.
*/
int rlist_add(struct rlist_t *rlist, struct data_t *car);

/* Remove da lista remota o primeiro carro que corresponda ao modelo indicado.
* Retorna 0 se encontrou e removeu, 1 se não encontrou, ou -1 em caso de erro.
*/
int rlist_remove_by_model(struct rlist_t *rlist, const char *modelo);

/* Obtém o primeiro carro que corresponda à marca indicada.
* Retorna ponteiro para os dados ou NULL se não encontrar ou em caso de erro.
*/
struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca);

/* Obtém um array de ponteiros para carros de um determinado ano.
* O último elemento do array é NULL.
* Retorna o array ou NULL em caso de erro.
*/
struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano);

/* Ordena a lista remota de carros por ano de fabrico (crescente).
* Retorna 0 (OK) ou -1 em caso de erro.
*/
int rlist_order_by_year(struct rlist_t *rlist);

/* Retorna o número de carros na lista remota ou -1 em caso de erro.
*/
int rlist_size(struct rlist_t *rlist);

/* Constrói um array de strings com os modelos dos carros na lista remota.
* O último elemento do array é NULL.
* Retorna o array ou NULL em caso de erro.
*/
char **rlist_get_model_list(struct rlist_t *rlist);

/* Liberta a memória ocupada pelo array de modelos.
* Retorna 0 (OK) ou -1 em caso de erro.
*/
int rlist_free_model_list(char **models);

#endif