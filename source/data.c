/* Source file: data.c */

/* Função que cria um novo elemento de dados data_t. 
 * Retorna a nova estrutura ou NULL em caso de erro. 
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "data.h"

int ano_atual();
int validar_dados(int ano, float preco, enum marca_t marca, const char *modelo,
                  enum combustivel_t combustivel);


struct data_t *data_create(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel) {

    if (!validar_dados(ano, preco, marca, modelo, combustivel)) {
      return NULL;
    }

    // Aloca memória para a nova estrutura
    struct data_t *data = malloc(sizeof(struct data_t));
    if (data == NULL) {  // Verifica se a alocação falhou
        return NULL;
    }
    
    // Inicializa os campos da estrutura
    data->ano = ano;
    data->preco = preco;
    data->marca = marca;
    data->combustivel = combustivel;
    data->modelo = strdup(modelo);  

    // Verifica se a alocação para o modelo falhou
    if (data->modelo == NULL) {
        free(data);
        return NULL;
    }
    return data;
}

/* Função que elimina um bloco de dados, libertando toda a memória por ele ocupada. 
 * Retorna 0 (OK) ou -1 em caso de erro. 
 */ 
int data_destroy(struct data_t *data) {
    if (data == NULL) {
        fprintf(stderr, "Erro: Não é possível eliminar um elemento de dados nulo\n");
        return -1;
    }
    free(data->modelo); // Liberta a memória alocada para o modelo
    free(data);        // Liberta a memória alocada para a estrutura data_t
    return 0;        
}

/* Função que duplica uma estrutura data_t. 
 * Retorna a nova estrutura ou NULL em caso de erro. 
 */
struct data_t *data_dup(struct data_t *data) {
    if (data == NULL) {
        return NULL;
    }
    return data_create(data->ano, data->preco, data->marca, data->modelo, data->combustivel);
}

/* Função que substitui o conteúdo de um elemento de dados data_t. 
 * Retorna 0 (OK) ou -1 em caso de erro. 
 */
int data_replace(struct data_t *data, int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel) {

    if (data == NULL) {
        fprintf(stderr, "Erro: data é null (replace)\n");
        return -1;
    }
    if (!validar_dados(ano, preco, marca, modelo, combustivel)) {
        return -1;
    }
    char *novo_modelo = strdup(modelo);
    if (novo_modelo == NULL) {
        return -1;

    }

    free(data->modelo); 
    data->modelo = novo_modelo; 
    data->ano = ano;
    data->preco = preco;
    data->marca = marca;
    data->combustivel = combustivel;

    return 0;
}

int ano_atual() {
    time_t t = time(NULL);            
    struct tm *tm_info = localtime(&t); 
    return tm_info->tm_year + 1900;  
}


// Função que verifica os dados e evita duplicação de código
int validar_dados(int ano, float preco, enum marca_t marca, const char *modelo,
                  enum combustivel_t combustivel) {
    if (!modelo) {
        fprintf(stderr, "Erro: modelo nulo\n");
        return 0;
    }
    if (ano < 1886 || ano > ano_atual()) {
        fprintf(stderr, "Erro: ano inválido\n");
        return 0;
    }
    if (preco < 0.0f) {
        fprintf(stderr, "Erro: preço inválido\n");
        return 0;
    }
    if (marca < MARCA_TOYOTA || marca > MARCA_MERCEDES) {
        fprintf(stderr, "Erro: marca inválida\n");
        return 0;
    }
    if (combustivel < COMBUSTIVEL_GASOLINA || combustivel > COMBUSTIVEL_HIBRIDO) {
        fprintf(stderr, "Erro: combustível inválido\n");
        return 0;
    }
    return 1; 
}