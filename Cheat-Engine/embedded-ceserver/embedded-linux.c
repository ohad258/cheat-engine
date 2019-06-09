#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "porthelp.h"
#include "embedded.h"
#include "ceserver.h"

int embedded__server_socket_g = -1;
int embedded__client_socket_g = -1;

void EMBEDDED__log(char *message)
{
    printf("%s\n", message);
}

EMBEDDED__rc_t EMBEDDED__send(uint8_t *buffer, uint32_t buffer_size, uint32_t *sent_size)
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    ssize_t send_rc = -1;

    send_rc = send(embedded__client_socket_g, (unsigned char *)buffer, buffer_size, 0);
    if (-1 == send_rc) {
        rc = EMBEDDED_SEND_FAILED;
        goto Exit;
    }

    *sent_size = send_rc;
    rc = EMBEDDED_SUCCESS;

Exit:
    return rc;
}

EMBEDDED__rc_t EMBEDDED__recv(uint8_t *buffer, uint32_t max_buffer_size, uint32_t *received_size)
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    ssize_t recv_rc = -1;

    recv_rc = recv(embedded__client_socket_g, (unsigned char *)buffer, max_buffer_size, MSG_WAITALL);
    if (-1 == recv_rc) {
        rc = EMBEDDED_RECV_FAILED;
        goto Exit;
    }

    *received_size = recv_rc;
    rc = EMBEDDED_SUCCESS;

Exit:
    return rc;
}

EMBEDDED__rc_t EMBEDDED__main()
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    struct sockaddr_in addr = {0};
    struct sockaddr_in client_addr = {0};
    int commercial_rc = -1;
    int reuseaddr_value = 1;
    int no_delay_value = 0;
    socklen_t client_size = 0;

    printf("CE Server for linux\n");
    embedded__server_socket_g = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == embedded__server_socket_g) {
        rc = EMBEDDED_CREATE_SOCKET_FAILED;
        goto Exit;
    }

    commercial_rc = setsockopt(embedded__server_socket_g,
                              SOL_SOCKET,
                              SO_REUSEADDR,
                              &reuseaddr_value,
                              sizeof(reuseaddr_value));
    if (-1 == commercial_rc) {
        rc = EMBEDDED_SETSOCKOPT_FAILED;
        goto Cleanup;
    }

    addr.sin_family = AF_INET;
    /* TODO: get port by arg */
    addr.sin_port = htons(1337);
    addr.sin_addr.s_addr = INADDR_ANY;

    commercial_rc = bind(embedded__server_socket_g, (const struct sockaddr *)&addr, sizeof(addr));
    if (-1 == commercial_rc) {
        rc = EMBEDDED_BIND_FAILED;
        goto Cleanup;
    }

    commercial_rc = listen(embedded__server_socket_g, 32);
    if (-1 == commercial_rc) {
        rc = EMBEDDED_LISTEN_FAILED;
        goto Cleanup;
    }

    printf("Waiting for connections...\n");
    while (TRUE) {
        embedded__client_socket_g = accept(embedded__server_socket_g,
                                           (struct sockaddr * restrict)&client_addr,
                                           &client_size);
        if (-1 == embedded__client_socket_g) {
            rc = EMBEDDED_ACCEPT_FAILED;
            goto Cleanup;
        }

        printf("Connection has been accepted.\n");
        commercial_rc = setsockopt(embedded__client_socket_g,
                                   IPPROTO_TCP,
                                   TCP_NODELAY,
                                   &no_delay_value,
                                   sizeof(no_delay_value));
        if (-1 == commercial_rc) {
            rc = EMBEDDED_SETSOCKOPT_CLIENT_FAILED;
            goto Cleanup;
        }

        rc = CE_SERVER__handle_connection();
        ON_EMBEDDED_ERROR_GOTO(rc, Cleanup);
    }

Cleanup:
    /* TODO: handle rc */
    close(embedded__server_socket_g);

Exit:
    return rc;
}

int main(int argc, char **argv)
{
    /* TODO: usage */
    /* TODO: use argv */
    return EMBEDDED__main();
}

/* TODO: refactor function */
EMBEDDED__rc_t EMBEDDED__get_process_list(ProcessList *process_list)
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    DIR *proc_folder = NULL;
    struct dirent *current_file = NULL;
    int pid;
    char exe_path[266];
    char process_path[512];
    uint32_t i = -1;
    char extra_file[270];
    int f;

    /* TODO: error handling */
    proc_folder = opendir("/proc/");

    /* TODO: error handling */
    current_file = readdir(proc_folder);
    while (NULL != current_file) {
        if (strspn(current_file->d_name, "1234567890") != strlen(current_file->d_name))
        {
            current_file = readdir(proc_folder);
            continue;
        }

        snprintf(exe_path, 200, "/proc/%s/exe", current_file->d_name);
        exe_path[199] = 0;

        /* TODO: better name */
        i = readlink(exe_path, process_path, 254);
        if (i == -1)
        {
            current_file = readdir(proc_folder);
            continue;
        }

        if (i > 254) {
            i = 254;
        }

        process_path[i] = 0;

        snprintf(extra_file, 255, "/proc/%s/cmdline", current_file->d_name);
        extra_file[254] = 0;

        /* TODO: better name */
        f = open(extra_file, O_RDONLY);
        if (f != -1)
        {
            i = read(f, extra_file, 255);
            if (i >= 0) {
                extra_file[i] = 0;
            }
            else {
                extra_file[0] = 0;
            }

            strcat(process_path, " ");
            strcat(process_path, extra_file);

            close(f);
        }

        sscanf(current_file->d_name, "%d", &pid);
        // printf("%d - %s\n", pid, processpath);

        //add this process to the list
        process_list->processList[process_list->processCount].PID=pid;
        process_list->processList[process_list->processCount].ProcessName=strdup(process_path);

        process_list->processCount++;

        if (process_list->processCount >= CE_SERVER_MAX_PROCESS_AMOUNT)
        {
            rc = EMBEDDED_PROCESSES_AMOUNT_EXCEEDED_MAX;
            goto Exit;
        }

        /* TODO: error handling */
        current_file = readdir(proc_folder);
    }

    /* TODO: error handling */
    closedir(proc_folder);

    rc =  EMBEDDED_SUCCESS;

Exit:
    return rc;
}
