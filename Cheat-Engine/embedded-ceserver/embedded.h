#ifndef EMBEDDED_H
#define EMBEDDED_H

#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#include "api.h"

#define ON_EMBEDDED_ERROR_GOTO(__rc, __label) \
    do { \
        if (EMBEDDED_SUCCESS != __rc) { \
            goto __label; \
        } \
    } while (FALSE)

typedef enum {
    EMBEDDED_SUCCESS = 0,
    EMBEDDED_UNINITIALIZED,

    /* Custom return codes */
    /* TODO: add ifdef linux or something */
    EMBEDDED_CREATE_SOCKET_FAILED,
    EMBEDDED_BIND_FAILED,
    EMBEDDED_LISTEN_FAILED,
    EMBEDDED_ACCEPT_FAILED,
    EMBEDDED_SETSOCKOPT_FAILED,
    EMBEDDED_SETSOCKOPT_CLIENT_FAILED,
    EMBEDDED_RECV_FAILED,
    EMBEDDED_SEND_FAILED,
    EMBEDDED_BAD_COMMAND_ID,
    EMBEDDED_PROCESSES_AMOUNT_EXCEEDED_MAX,
    EMBEDDED_FAILED_TO_CHDIR,
} EMBEDDED__rc_t;

/* Logging functions */
void EMBEDDED__log(char *message);

/* Server socket implementation */
EMBEDDED__rc_t EMBEDDED__main();

/* Socket handling functions implemenations */
EMBEDDED__rc_t EMBEDDED__send(uint8_t *buffer, uint32_t buffer_size, uint32_t *sent_size);
EMBEDDED__rc_t EMBEDDED__recv(uint8_t *buffer, uint32_t max_buffer_size, uint32_t *received_size);

/* WINAPI & commands implementations */
EMBEDDED__rc_t EMBEDDED__get_process_list(ProcessList *process_list);
#endif
