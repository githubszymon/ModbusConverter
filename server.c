/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdlib.h>
#include <errno.h>

#include "/home/wojtek/Studia/libmodbus/src/modbus.h"

int main(void)
{
    int s = -1;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;

	int rc;
    ctx = modbus_new_rtu("/dev/ttyS0", 115200,'N',8,1);
    modbus_set_slave(ctx,17);
	modbus_set_debug(ctx, TRUE);
    rc = modbus_connect(ctx);

    mb_mapping = modbus_mapping_new(500, 500, 500, 500);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

//    s = modbus_tcp_listen(ctx, 1);
  //  modbus_tcp_accept(ctx, &s);

    for (;;) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        

        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            /* rc is the query size */
            modbus_reply(ctx, query, rc, mb_mapping);
        } else if (rc == -1) {
            /* Connection closed by the client or error */
            break;
        }
    }

    printf("Quit the loop: %s\n", modbus_strerror(errno));

    if (s != -1) {
        close(s);
    }
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
