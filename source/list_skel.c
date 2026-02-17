/* 
 * Grupo nº 07
 * Afonso Henriques - 61826
 * Leonardo Pardal - 61836
 * Pedro Carvalho - 61800
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list_skel.h"
#include "list.h"
#include "list-private.h"
#include "sdmessage.pb-c.h"
#include "../include/data.h"




Data *data_to_protobuf(struct data_t *car) {
    if (!car) return NULL;

    Data *d = malloc(sizeof(Data));
    if (!d) return NULL;
    data__init(d);

    d->ano = car->ano;
    d->preco = car->preco;
    d->marca = (Marca) car->marca;
    d->modelo = strdup(car->modelo);
    d->combustivel = (Combustivel) car->combustivel;
    return d;
}
Data **data_array_to_protobuf_array(struct data_t **cars, size_t *k) {
    if (!cars) {
        *k = 0;
        return NULL;
    }

    size_t count = 0;
    while (cars[count] != NULL) count++;

    Data **arr = malloc(sizeof(Data*) * count);
    if (!arr) {
        *k = 0;
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        arr[i] = data_to_protobuf(cars[i]);
    }

    *k = count;
    return arr;
}
struct data_t **list_get_all(struct list_t *list) {
    if (list == NULL)
        return NULL;

    struct car_t *current = list->head;
    struct data_t **array = malloc((list->size + 1) * sizeof(struct data_t *));
    if (!array)
        return NULL;

    int i = 0;
    while (current != NULL) {
        array[i++] = current->data;
        current = current->next;
    }
    array[i] = NULL; // termina com NULL
    return array;
}



/* Inicia o skeleton da lista.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * Retorna a tabela criada ou NULL em caso de erro.
 */
struct list_t *list_skel_init(){
    return list_create();
}


/* Liberta toda a memória ocupada pela lista e todos os recursos
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int list_skel_destroy(struct list_t *list){
    if(list == NULL) 
        return -1;
    list_destroy(list);
    return 0;
}


/* Executa na lista a operação indicada pelo opcode contido em msg
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int invoke(MessageT *msg, struct list_t *list) {
    if (!msg || !list) return -1;

    switch(msg->opcode) {

        case MESSAGE_T__OPCODE__OP_SIZE:
            msg->result = list_size(list);
            if (msg->result == -1) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
            msg->opcode += 1; 
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            break;

        case MESSAGE_T__OPCODE__OP_ADD:
            if (!msg->data) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                msg->result = -1;
                return -1;
            }
            struct data_t *car = data_create(
                msg->data->ano,
                msg->data->preco,
                msg->data->marca,
                msg->data->modelo,
                msg->data->combustivel
            );
            if (!car) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                msg->result = -1;
                return -1;
            }
            int res = list_add(list, car);
            data_destroy(car);
            if (res == 0) {
                msg->opcode += 1; 
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                msg->result = 0;
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                msg->result = -1;
            }
            break;

        case MESSAGE_T__OPCODE__OP_GET:
            if (msg->c_type == MESSAGE_T__C_TYPE__CT_MARCA) {
                struct data_t *car = list_get_by_marca(list, msg->result);

                if (!car) {
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    msg->result = -1;
                    break;
                }
                
                msg->data = data_to_protobuf(car);
                if (!msg->data) {
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    msg->result = -1;
                    break;
                }

                msg->opcode += 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_DATA;
            }
            break;

        case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR: 
            if (msg->result != -1) {
                struct data_t **cars = list_get_by_year(list,msg->result);
                if (!cars) {
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    msg->result = -1;
                    break;
                }
                Data **converted_cars = data_array_to_protobuf_array(cars,&msg->n_cars);
                msg->cars = converted_cars;
                msg->opcode += 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
                free(cars);
                break;
            }
            else {
                list_order_by_year(list);
                struct data_t **cars = list_get_all(list);
                if (!cars) {
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    msg->result = -1;
                    break;
                }
                msg->cars = data_array_to_protobuf_array(cars,&msg->n_cars);
                msg->opcode += 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
                free(cars);
            }            
            break;

        case MESSAGE_T__OPCODE__OP_GETMODELS:
            msg->models = list_get_model_list(list);
            msg->n_models = list_size(list);
            if (!msg->models) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                msg->result = -1;
                break;
            } else {
                msg->opcode += 1; 
                msg->c_type = MESSAGE_T__C_TYPE__CT_MODEL;
            }
            break;

        case MESSAGE_T__OPCODE__OP_DEL:
            if (!msg|| msg->n_models != 1) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                msg->result = -1;
            } else {
                int res = list_remove_by_model(list, msg->models[0]);
                msg->opcode += 1;
                msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
                msg->result = res;
            }
            break;
        default:
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            msg->result = -1;
    }

    return 0;
}

