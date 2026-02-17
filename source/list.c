/* 
 * Grupo nº 07
 * Afonso Henriques - 61826
 * Leonardo Pardal - 61836
 * Pedro Carvalho - 61800
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "list-private.h"

struct list_t *list_create() {
    struct list_t *list = (struct list_t *) malloc(sizeof(struct list_t));
    if (list == NULL) {
      perror("Erro ao alocar memória para a lista (list_create)");
      return NULL;
    }
    
    list->size = 0;
    list->head = NULL;
    return list;
}

int list_destroy(struct list_t *list) {
    if (list == NULL) {
        perror("Erro: lista nula (list_destroy)\n");
        return -1;
    }

    struct car_t *current = list->head;
    while (current != NULL) {
        struct car_t *next = current->next;
        data_destroy(current->data);
        free(current);
        current = next;
    }
    free(list);
    return 0;
}


int list_add(struct list_t *list, struct data_t *car) {
    if (list == NULL || car == NULL) {
      perror("Erro: list ou car é nulo (list_add)\n");
      return -1;
    }

    struct car_t *new_node = (struct car_t *) malloc(sizeof(struct car_t));
    if (new_node == NULL) {
      perror("Erro ao alocar memória para o nó (list_add)");
      return -1;
    }

    new_node->data = data_dup(car);
    if (new_node->data == NULL) { 
        free(new_node);
        return -1;
    }
    new_node->next = NULL;
    if (list->head == NULL) {
        list->head = new_node;
    } else {
        struct car_t *current = list->head;
        while (current->next != NULL) current = current->next;
        current->next = new_node;
    }
    list->size++;
    return 0;
}

int list_remove_by_model(struct list_t *list, const char *modelo) {
    if (list == NULL || modelo == NULL) {
      perror("Erro: list ou modelo nulo (list_remove_by_model)\n");
      return -1;
    }

    struct car_t *current = list->head;
    struct car_t *prev = NULL;  

    while (current != NULL) {
        if (strcmp(current->data->modelo, modelo) == 0) {
            if (prev == NULL) { 
                list->head = current->next;
            } else {
                prev->next = current->next;
            }
            data_destroy(current->data);
            free(current);
            list->size--;
            return 0;
        }
        prev = current;
        current = current->next;
    }
    return 1; 
}


struct data_t *list_get_by_marca(struct list_t *list, enum marca_t marca) {
    if (list == NULL) {
      perror("Erro: list é nulo (list_get_by_marca)\n");
      return NULL;  
    }

    struct car_t *current = list->head;
    while (current != NULL) {
        if (current->data->marca == marca) {
            return current->data;
        }
        current = current->next;
    }
    return NULL;
}

struct data_t **list_get_by_year(struct list_t *list, int ano) {
    if (list == NULL) {
      perror("Erro: list (list_get_by_year)\n");
      return NULL;
    }

    
    int count = 0;
    struct car_t *current = list->head;
    while (current != NULL) {
        if (current->data->ano == ano) count++;
        current = current->next;
    }

    struct data_t **array = (struct data_t **) malloc((count + 1) * sizeof(struct data_t *));
    if (array == NULL){
      perror("Erro ao alocar memória para o data_t array (list_get_by_year)");
      return NULL;
    }
    
    current = list->head;
    int i = 0;
    while (current != NULL) {
        if (current->data &&  current->data->ano == ano) {
            array[i++] = current->data;
        }
        current = current->next;
    }
    array[i] = NULL; 
    return array;
}


int list_order_by_year(struct list_t *list) {
    if (list == NULL) {
      perror("Erro: list nulo (list_order_by_year)\n");
      return -1;
    }

    int swapped;
    do {
        swapped = 0;
        struct car_t *current = list->head;
        while (current != NULL && current->next != NULL) {
            if (current->data->ano > current->next->data->ano) {
                struct data_t *temp = current->data;
                current->data = current->next->data;
                current->next->data = temp;
                swapped = 1;
            }
            current = current->next;
        }
    } while (swapped);

    return 0;
}

int list_size(struct list_t *list) {
    if (list == NULL) {
      perror("Erro: list nulo (list_size)\n");
      return -1;
    }
    return list->size;
}


char **list_get_model_list(struct list_t *list) {
    if (list == NULL) {
      perror("Erro: list (list_get_model_list)\n");
      return NULL;
    }

    char **models = (char **) malloc((list->size + 1) * sizeof(char *));
    if (models == NULL) {
      perror("Erro ao alocar memória para o buffer (car_to_buffer)");
      return NULL;
    }

    struct car_t *current = list->head;
    int i = 0;
    while (current != NULL) {
        models[i] = strdup(current->data->modelo);
        if (models[i] == NULL) {
            // erro → limpar
            for (int j = 0; j < i; j++) free(models[j]);
            free(models);
            return NULL;
        }
        i++;
        current = current->next;
    }
    models[i] = NULL;
    return models;
}


int list_free_model_list(char **models) {
    if (models == NULL) {
      perror("Erro: modelo nulo (list_free_model_list)\n");
      return -1;
    }
    
    int i = 0;
    while (models[i] != NULL) {
        free(models[i]);
        i++;
    }
    free(models);
    return 0;
}
