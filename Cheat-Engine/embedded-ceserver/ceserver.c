#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "embedded.h"
#include "api.h"
#include "ceserver.h"

#define CE_SERVER__RECEIVE_BUFFER_SIZE (1000)

/* TODO: command handlers */
/*void *ce_server__command_handlers_g[] = {{CMD_GETVERSION, ce_server__handle_get_version},
                                         {CMD_CREATETOOLHELP32SNAPSHOT, ce_server__handle_create_tool_help_32_snapshot},
     };*/
/* TODO: add define to header */
ProcessListEntry CE_SERVER__process_list_entries_g[CE_SERVER_MAX_PROCESS_AMOUNT] = {0};
ProcessList CE_SERVER__process_list_g = {0, 0, 0, CE_SERVER__process_list_entries_g};
ProcessData CE_SERVER__current_process_g = {0};


void mylog(char *b, uint32_t s)
{
    for(uint32_t i = 0; i < s; i++)
        printf("%02X ", b[i]);
    printf("\n");
}

EMBEDDED__rc_t CE_SERVER__send_all(uint8_t *buffer, uint32_t size)
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    uint32_t total_sent = 0;
    uint32_t current_sent = 0;

    while (total_sent < size) {
        rc = EMBEDDED__send(&buffer[total_sent], (size - total_sent), &current_sent);
        ON_EMBEDDED_ERROR_GOTO(rc, Exit);

        total_sent += current_sent;
    }

    printf("[SEND] ");
    mylog((char *)buffer, total_sent);
Exit:
    return rc;
}

EMBEDDED__rc_t CE_SERVER__recv_all(uint8_t *buffer, uint32_t size)
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    uint32_t total_received = 0;
    uint32_t current_received = 0;

    while (total_received < size) {
        rc = EMBEDDED__recv(&buffer[total_received], (size - total_received), &current_received);
        ON_EMBEDDED_ERROR_GOTO(rc, Exit);

        total_received += current_received;
    }

    printf("[RECV] ");
    mylog((char *)buffer, total_received);
Exit:
    return rc;
}

EMBEDDED__rc_t ce_server__send_next_process()
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    bool result = FALSE;
    CeProcessEntry process_entry = {0};
    char *process_name = NULL;
    uint32_t size_to_send = 0;


    if (CE_SERVER__process_list_g.processListIterator >= CE_SERVER__process_list_g.processCount)
    {
        process_entry.processnamesize = 0;
        process_entry.pid = 0;
    }
    else
    {
        process_name = CE_SERVER__process_list_g.processList[CE_SERVER__process_list_g.processListIterator].ProcessName;
        process_entry.pid = CE_SERVER__process_list_g.processList[CE_SERVER__process_list_g.processListIterator].PID;
        process_entry.processnamesize = strlen(process_name);
        /* TODO: currently sending a lot of zeros as part of the name (can be optimized) */
        memcpy((unsigned char *)process_entry.processname, process_name, process_entry.processnamesize);

        CE_SERVER__process_list_g.processListIterator++;
        result = TRUE;
    }

    process_entry.result = result;

    size_to_send = sizeof(process_entry) - (CE_SERVER_MAX_PROCESS_NAME_LENGTH * sizeof(uint8_t)) + process_entry.processnamesize;
    rc = CE_SERVER__send_all((uint8_t *)&process_entry, size_to_send);
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    rc = EMBEDDED_SUCCESS;
Exit:
    return rc;
}

EMBEDDED__rc_t ce_server__handle_open_process()
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    uint32_t process_id = 0;
    uint32_t process_handle = -1;

    rc = CE_SERVER__recv_all((uint8_t *)&process_id, sizeof(process_id));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    memset(&CE_SERVER__current_process_g, 0, sizeof(ProcessData));
    CE_SERVER__current_process_g.ReferenceCount = 1;
    CE_SERVER__current_process_g.pid = process_id;
    CE_SERVER__current_process_g.threadlist=NULL;

    rc = EMBEDDED__open_process(process_id, &CE_SERVER__current_process_g);
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    process_handle = CreateHandleFromPointer(&CE_SERVER__current_process_g, htProcesHandle);
    rc = CE_SERVER__send_all((uint8_t *)&process_handle, sizeof(process_handle));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    rc = EMBEDDED_SUCCESS;
Exit:
    return rc;
}

EMBEDDED__rc_t ce_server__handle_get_architecture()
{
#ifdef __i386__
      uint8_t arch = 0;
#endif
#ifdef __x86_64__
      uint8_t arch = 1;
#endif
#ifdef __arm__
      uint8_t arch = 2;
#endif
#ifdef __aarch64__
      uint8_t arch = 3;
#endif
    return CE_SERVER__send_all((uint8_t *)&arch, sizeof(arch));
}

EMBEDDED__rc_t ce_server__handle_create_toolhelp32_snapshot()
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    CeCreateToolhelp32Snapshot params = {0};
    uint32_t process_list_handle = -1;

    rc = CE_SERVER__recv_all((uint8_t *)&params, sizeof(params));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    if (params.dwFlags & TH32CS_SNAPPROCESS)
    {
        CE_SERVER__process_list_g.ReferenceCount = 1;
        CE_SERVER__process_list_g.processCount = 0;

        rc = EMBEDDED__get_process_list(&CE_SERVER__process_list_g);
        ON_EMBEDDED_ERROR_GOTO(rc, Exit);

        /* TODO: error handling */
        process_list_handle = CreateHandleFromPointer(&CE_SERVER__process_list_g, htTHSProcess);
        CE_SERVER__send_all((uint8_t *)&process_list_handle, sizeof(process_list_handle));
    }

    if (params.dwFlags & TH32CS_SNAPMODULE)
    {
        /* TODO: currently placeholder */
        process_list_handle = CreateHandleFromPointer(0xDDDDDDDD, htTHSProcess);
        /* TODO: error handling */
        CE_SERVER__send_all((uint8_t *)&process_list_handle, sizeof(process_list_handle));
    }

    rc = EMBEDDED_SUCCESS;
Exit:
    return rc;
}

EMBEDDED__rc_t ce_server__handle_process_32_first()
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    uint32_t process_list_handle = -1;

    rc = CE_SERVER__recv_all((uint8_t *)&process_list_handle, sizeof(process_list_handle));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    CE_SERVER__process_list_g.processListIterator = 0;

    rc = ce_server__send_next_process();
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    rc = EMBEDDED_SUCCESS;
Exit:
    return rc;
}

EMBEDDED__rc_t ce_server__handle_process_32_next()
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    uint32_t process_list_handle = -1;

    rc = CE_SERVER__recv_all((uint8_t *)&process_list_handle, sizeof(process_list_handle));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    rc = ce_server__send_next_process();
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    rc = EMBEDDED_SUCCESS;
Exit:
    return rc;
}

EMBEDDED__rc_t ce_server__handle_close_handle()
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    uint32_t handle = -1;
    /* TODO: initialize to unsuccess */
    uint32_t result = 1;

    rc = CE_SERVER__recv_all((uint8_t *)&handle, sizeof(handle));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    /* TODO: not really closing */
    //CloseHandle(handle);
    rc = CE_SERVER__send_all((uint8_t *)&result, sizeof(result));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    rc = EMBEDDED_SUCCESS;
Exit:
    return rc;
}

EMBEDDED__rc_t ce_server__handle_module_32_first()
{
    /* TODO: place holder - currently send none */
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    uint32_t module_list_handle = -1;
    CeModuleEntry module_entry = {0};

    rc = CE_SERVER__recv_all((uint8_t *)&module_list_handle, sizeof(module_list_handle));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    rc = CE_SERVER__send_all((uint8_t *)&module_entry, sizeof(module_entry));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    rc = EMBEDDED_SUCCESS;
Exit:
    return rc;
}

EMBEDDED__rc_t CE_SERVER__handle_command(uint8_t command_id)
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;

    switch(command_id) {
        case CMD_OPENPROCESS:
            EMBEDDED__log("Handling CMD_OPENPROCESS");
            rc = ce_server__handle_open_process();
            ON_EMBEDDED_ERROR_GOTO(rc, Exit);
            break;

        case CMD_CREATETOOLHELP32SNAPSHOT:
            EMBEDDED__log("Handling CMD_CREATETOOLHELP32SNAPSHOT");
            /* TODO: lower case it? */
            rc = ce_server__handle_create_toolhelp32_snapshot();
            ON_EMBEDDED_ERROR_GOTO(rc, Exit);
            break;

        case CMD_PROCESS32FIRST:
            EMBEDDED__log("Handling CMD_PROCESS32FIRST");
            rc = ce_server__handle_process_32_next();
            ON_EMBEDDED_ERROR_GOTO(rc, Exit);
            break;

        case CMD_PROCESS32NEXT:
            EMBEDDED__log("Handling CMD_PROCESS32NEXT");
            rc = ce_server__handle_process_32_next();
            ON_EMBEDDED_ERROR_GOTO(rc, Exit);
            break;

        case CMD_CLOSEHANDLE:
            EMBEDDED__log("Handling CMD_CLOSEHANDLE");
            rc = ce_server__handle_close_handle();
            ON_EMBEDDED_ERROR_GOTO(rc, Exit);
            break;

        case CMD_MODULE32FIRST:
            EMBEDDED__log("Handling CMD_MODULE32FIRST");
            rc = ce_server__handle_module_32_first();
            ON_EMBEDDED_ERROR_GOTO(rc, Exit);
            break;

        case CMD_MODULE32NEXT:
            EMBEDDED__log("Handling CMD_MODULE32NEXT");
            /* TODO: should call next */
            rc = ce_server__handle_module_32_first();
            ON_EMBEDDED_ERROR_GOTO(rc, Exit);
            break;

        case CMD_GETARCHITECTURE:
            EMBEDDED__log("Handling CMD_GETARCHITECTURE");
            rc = ce_server__handle_get_architecture();
            ON_EMBEDDED_ERROR_GOTO(rc, Exit);
            break;

        default:
            rc = EMBEDDED_BAD_COMMAND_ID;
    }

Exit:
    return rc;
}

EMBEDDED__rc_t CE_SERVER__handle_connection()
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    uint8_t command_id = 0;

    while (TRUE) {
        rc = CE_SERVER__recv_all(&command_id, 1);
        ON_EMBEDDED_ERROR_GOTO(rc, Exit);

        rc = CE_SERVER__handle_command(command_id);
        ON_EMBEDDED_ERROR_GOTO(rc, Exit);
    }

    rc = EMBEDDED_SUCCESS;
Exit:
    return rc;
}

/* TODO: remove */
int CheckForAndDispatchCommand(int currentsocket)
{
    return 0xdd;
}

/* TODO: remove */
ssize_t sendall (int s, void *buf, size_t size, int flags)
{
    return 0x11;
}

ssize_t recvall (int s, void *buf, size_t size, int flags)
{
    return 0xbb;
}
