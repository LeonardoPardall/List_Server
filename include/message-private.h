/* Header: message-private.h */
#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

int write_all(int sock, void *buf, int len);

int read_all(int sock, void *buf, int len);

#endif