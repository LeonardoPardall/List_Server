/* Source file: message-private.c */

#include "message-private.h"
#include <unistd.h>
#include <errno.h>

int write_all(int sock, void *buf, int len) {
    int total = 0;
    while (total < len) {
        ssize_t n = write(sock, buf + total, len - total);
        if (n <= 0) {
            if (errno == EINTR) continue;
            return -1; 
        }
        total += n;
    }
    return total;
}

int read_all(int sock, void *buf, int len) {
    int total = 0;
    while (total < len) {
        ssize_t n = read(sock, buf + total, len - total);
        if (n <= 0) {
            if (errno == EINTR) continue;
            return -1; 
        }
        total += n;
    }
    return total;
}
