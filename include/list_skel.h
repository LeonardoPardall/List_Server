/* Header: list_skel.h */
#ifndef _LIST_SKEL_H
#define _LIST_SKEL_H

#include "sdmessage.pb-c.h"
#include "list.h"

/* Inicia o skeleton da lista.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * Retorna a tabela criada ou NULL em caso de erro.
 */
struct list_t *list_skel_init();

/* Liberta toda a memória ocupada pela lista e todos os recursos
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_skel_destroy(struct list_t *list);

/* Executa na lista a operação indicada pelo opcode contido em msg
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int invoke(MessageT *msg, struct list_t *list);

#endif
