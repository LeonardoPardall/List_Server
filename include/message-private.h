/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 4/10/2025
 */
#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

int write_all(int sock, void *buf, int len);

int read_all(int sock, void *buf, int len);

#endif