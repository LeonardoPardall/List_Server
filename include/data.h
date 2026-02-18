/**
 * @file data.h
 * @brief Interface para a estrutura de dados.
 *
 * Este ficheiro define as funções e estruturas necessárias para a
 * manipulação de um bloco de dados representando um automóvel.
 *
 */
#ifndef _DATA_H
#define _DATA_H /* Módulo data */

enum marca_t
{
	MARCA_TOYOTA,
	MARCA_BMW,
	MARCA_RENAULT,
	MARCA_AUDI,
	MARCA_MERCEDES
};

enum combustivel_t
{
	COMBUSTIVEL_GASOLINA,
	COMBUSTIVEL_GASOLEO,
	COMBUSTIVEL_ELETRICO,
	COMBUSTIVEL_HIBRIDO
};

/* Estrutura que define os dados de um automóvel */
struct data_t
{
	int ano;
	float preco;
	enum marca_t marca;
	char *modelo;
	enum combustivel_t combustivel;
};

/* Função que cria um novo elemento de dados data_t.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct data_t *data_create(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel);

/* Função que elimina um bloco de dados, libertando toda a memória por ele ocupada.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int data_destroy(struct data_t *data);

/* Função que duplica uma estrutura data_t.
 * Retorna a nova estrutura ou NULL em caso de erro.
 */
struct data_t *data_dup(struct data_t *data);

/* Função que substitui o conteúdo de um elemento de dados data_t.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int data_replace(struct data_t *data, int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel);

#endif
