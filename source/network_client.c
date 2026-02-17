/* 
 * Grupo nº 07
 * Afonso Henriques - 61826
 * Leonardo Pardal - 61836
 * Pedro Carvalho - 61800
 */

#include "network_client.h"
#include "message-private.h"    
#include "sdmessage.pb-c.h"  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include "client_stub-private.h"

int network_connect(struct rlist_t *rlist) {
    if (rlist == NULL) {
        fprintf(stderr, "rlist é null (network_connect)\n");
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "Falha ao criar socket (network_connect)\n");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(rlist->server_port);
    if (inet_pton(AF_INET, rlist->server_address, &server_addr.sin_addr) < 1) {
      printf("Erro ao converter IP (network_connect)\n");
      close(sock);
      return -1;
    }

    if (connect(sock,(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        //fprintf(stderr, "Falha ao conectar com o servidor (network_connect)\n");
        close(sock);
        return -1;
    }

    rlist->sockfd = sock;

    /* 4) receber uint16_t com comprimento da resposta */
    uint16_t resp_len_net;
    if (read_all(rlist->sockfd, &resp_len_net, sizeof(resp_len_net)) != (ssize_t)sizeof(resp_len_net)) {
        fprintf(stderr, "network_send_receive: read_all(len)\n");
        return -1;
    }

    uint16_t resp_len = ntohs(resp_len_net);
    if (resp_len == 0) {
        fprintf(stderr, "network_send_receive: resposta com comprimento 0\n");
        return -1;
    }

    /* 5) receber buffer com resp_len bytes */
    uint8_t *resp_buf = malloc(resp_len);
    if (resp_buf == NULL) return 0;

    if (read_all(rlist->sockfd, resp_buf, resp_len) != (ssize_t)resp_len) {
        fprintf(stderr, "network_send_receive: read_all(buf)\n");
        free(resp_buf);
        return -1;
    }

    /* 6) desserializar */
    MessageT *resp = message_t__unpack(NULL, resp_len, resp_buf);
    free(resp_buf);

    if (resp == NULL) {
        fprintf(stderr, "network_send_receive: message_t__unpack falhou\n");
        return -1;
    }
    
    if (resp->opcode == MESSAGE_T__OPCODE__OP_BUSY) {
        free(resp);
        if (rlist->sockfd >= 0) {
                close(rlist->sockfd);
                rlist->sockfd = -1;
        }
        errno = EBUSY;
        return -1;
    }

    if (resp->opcode == MESSAGE_T__OPCODE__OP_READY) {
      free(resp);
      return 0;
    }
    
    free(resp);

    return -1;
}

MessageT *network_send_receive(struct rlist_t *rlist, MessageT *msg) {
    if (rlist == NULL || msg == NULL) {
        fprintf(stderr, "rlist ou msg é null (network_send_receive)\n");
        return NULL;
    }

    if (rlist->sockfd < 0) {
        fprintf(stderr, "socket é inválido (network_send_receive)\n");
        return NULL;
    }

    /* 1) obter tamanho e serializar */
    size_t packed_size = message_t__get_packed_size(msg);
    if (packed_size <= 0) {
        fprintf(stderr, "tamanho do pacote é menor ou igual a zero (network_send_receive)\n");
        return NULL;
    }

    uint8_t *outbuf = malloc(packed_size);
    if (outbuf == NULL) return NULL;

    message_t__pack(msg, outbuf);

    /* 2) enviar uint16_t com comprimento (network byte order) */
    if (packed_size > 0xFFFF) {
        free(outbuf);
        fprintf(stderr, "network_send_receive: mensagem demasiado grande (%zu bytes)\n", packed_size);
        return NULL;
    }

    uint16_t len16 = (uint16_t)packed_size;
    uint16_t len_net = htons(len16);

    if (write_all(rlist->sockfd, &len_net, sizeof(len_net)) != (ssize_t)sizeof(len_net)) {
        fprintf(stderr, "network_send_receive: write_all(len)\n");
        free(outbuf);
        return NULL;
    }

    /* 3) enviar buffer serializado */
    if (write_all(rlist->sockfd, outbuf, packed_size) != (ssize_t)packed_size) {
        fprintf(stderr, "network_send_receive: write_all(buf)\n");
        free(outbuf);
        return NULL;
    }

    free(outbuf);

    /* 4) receber uint16_t com comprimento da resposta */
    uint16_t resp_len_net;
    if (read_all(rlist->sockfd, &resp_len_net, sizeof(resp_len_net)) != (ssize_t)sizeof(resp_len_net)) {
        fprintf(stderr, "network_send_receive: read_all(len)\n");
        return NULL;
    }

    uint16_t resp_len = ntohs(resp_len_net);
    if (resp_len == 0) {
        fprintf(stderr, "network_send_receive: resposta com comprimento 0\n");
        return NULL;
    }

    /* 5) receber buffer com resp_len bytes */
    uint8_t *resp_buf = malloc(resp_len);
    if (resp_buf == NULL) return NULL;

    if (read_all(rlist->sockfd, resp_buf, resp_len) != (ssize_t)resp_len) {
        fprintf(stderr, "network_send_receive: read_all(buf)\n");
        free(resp_buf);
        return NULL;
    }

    /* 6) desserializar */
    MessageT *resp = message_t__unpack(NULL, resp_len, resp_buf);
    free(resp_buf);

    if (resp == NULL) {
        fprintf(stderr, "network_send_receive: message_t__unpack falhou\n");
        return NULL;
    }

    return resp;
}

int network_close(struct rlist_t *rlist) {
    if (rlist == NULL) {
        fprintf(stderr, "rlist é nulo (network_close)\n");
        return -1;
    }
    
    if (rlist->sockfd >= 0) {
        if (close(rlist->sockfd) < 0) {
            fprintf(stderr, "network_close: close\n");
            rlist->sockfd = -1;
            return -1;
        }
        rlist->sockfd = -1;
    }
    return 0;
}