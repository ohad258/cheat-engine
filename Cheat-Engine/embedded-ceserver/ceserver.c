#include <stdio.h>
#include "embedded.h"
#include "api.h"
#include "ceserver.h"

#define CE_SERVER__RECEIVE_BUFFER_SIZE (1000)

/* TODO: command handlers */
/*void *ce_server__command_handlers_g[] = {{CMD_GETVERSION, ce_server__handle_get_version},
                                         {CMD_CREATETOOLHELP32SNAPSHOT, ce_server__handle_create_tool_help_32_snapshot},
     };*/
ProcessList CE_SERVER__process_list_g = {0};
/* TODO: add define to header */
ProcessListEntry CE_SERVER__process_list_entries_g[CE_SERVER_MAX_PROCESS_AMOUNT] = {0};

void mylog(char *b, uint32_t s)
{
    for(uint32_t i = 0; i < s; i++)
        printf("%02X ", b[i]);
    printf("\n");
}

EMBEDDED__rc_t CE_SERVER__recv_all(unsigned char *buffer, uint32_t size)
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    uint32_t total_received = 0;
    uint32_t current_received = 0;

    while (total_received < size) {
        rc = EMBEDDED__recv(&buffer[total_received], (size - total_received), &current_received);
        ON_EMBEDDED_ERROR_GOTO(rc, Exit);

        total_received += current_received;
    }

    mylog((char *)buffer, total_received);
Exit:
    return rc;
}

EMBEDDED__rc_t CE_SERVER__handle_create_toolhelp32_snapshot()
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;
    CeCreateToolhelp32Snapshot params = {0};

    rc = CE_SERVER__recv_all((unsigned char *)&params, sizeof(params));
    ON_EMBEDDED_ERROR_GOTO(rc, Exit);

    if (params.dwFlags & TH32CS_SNAPPROCESS)
    {
        CE_SERVER__process_list_g.ReferenceCount = 1;
        CE_SERVER__process_list_g.processCount = 0;

        /* TODO: send back the handle */
        rc = EMBEDDED__get_process_list(&CE_SERVER__process_list_g);
        ON_EMBEDDED_ERROR_GOTO(rc, Exit);
    }

    /* TODO: handle snapmodule */

    rc = EMBEDDED_SUCCESS;
Exit:
    return rc;
}

EMBEDDED__rc_t CE_SERVER__handle_command(uint8_t command_id)
{
    EMBEDDED__rc_t rc = EMBEDDED_UNINITIALIZED;

    switch(command_id) {
        case CMD_CREATETOOLHELP32SNAPSHOT:
            EMBEDDED__log("Handling CMD_CREATETOOLHELP32SNAPSHOT");
            rc = CE_SERVER__handle_create_toolhelp32_snapshot();
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
