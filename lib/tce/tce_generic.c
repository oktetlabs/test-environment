/** @file
 * Generic part of TCE retrieval procedures
 * 
 *
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 *
 */

#define _GNU_SOURCE 1

#include "te_config.h"

#include <stdarg.h>
#include <stdio.h>

#include "tapi_rpc_unistd.h"
#include "tce_internal.h"

#define SYS_TCE_PREFIX "/sys/tce/"

static int
open_tce_info(rcf_rpc_server *rpcs, 
              int progno, int objno, const char *functr, int functrno, int arcno,
              const char *attrname)
{
    char path[32] = SYS_TCE_PREFIX;
    int pos = strlen(path);
    int size = sizeof(path) - pos - 1;
    char *ptr = path + pos;
#define ADJPOS if (pos < 0 || pos > size) return -1; else ptr += pos, size -= pos

    pos = snprintf(ptr, size, "%d/", progno);
    ADJPOS;

    if (objno >= 0)
    {
        pos = snprintf(ptr, size, "%d/", objno);
        ADJPOS;
        if (functr != NULL)
        {
            pos = strlen(functr);
            if (pos > size) 
                return -1;
            memcpy(ptr, functr, pos);
            ptr += pos;
            size -= pos;
            
            pos = snprintf(ptr, size, "%d/", functrno);
            ADJPOS;
            if (arcno >= 0)
            {
                pos = snprintf(ptr, size, "%d/", arcno);
                ADJPOS;
            }
        }
    }
    strncpy(ptr, attrname, size);

#undef ADJPOS
    
    return rpc_open(rpcs, path, RPC_O_RDONLY, 0);
}


int
tce_read_value(rcf_rpc_server *rpcs, 
               int progno, int objno, const char *functr, int functrno, int arcno, 
               const char *attrname, const char *fmt, ...)
{
    va_list args;
    int fd;
    int n_fields = 0;
    char buffer[128];
    
    va_start(args, fmt);
    fd = open_tce_info(rpcs, progno, objno, functr, functrno, arcno, attrname);
    if (fd >= 0)
    {
        int len;
        
        len = rpc_read(rpcs, fd, buffer, sizeof(buffer) - 1);
        if (len > 0)
        {
            buffer[len] = '\0';
            n_fields = vsscanf(buffer, fmt, args);
        }
        rpc_close(rpcs, fd);
    }
    return n_fields;
}

int
tce_read_counters(rcf_rpc_server *rpcs, int progno, int objno, int ctrno, tce_counter *dest)
{
    unsigned i;
    unsigned n_pages;
    int total;
    int len;
    char *iter;
    int fd;
    
    tce_read_value(rpcs, TCE_CTR(progno, objno, ctrno), "n_counters", "%u", &dest->num);
    tce_read_value(rpcs, TCE_CTR(progno, objno, ctrno), "n_pages", "%u", &n_pages);

    total = sizeof(*dest->values) * dest->num;
    dest->values = malloc(total);
    if (dest->values == NULL)
        return TE_OS_RC(TE_TAPI, errno);
    iter = (void *)dest->values;

    for (i = 0; i < n_pages; i++)
    {
        fd = open_tce_info(rpcs, TCE_VAL(progno, objno, ctrno, i), "data");
        len = rpc_read(rpcs, fd, iter, total);
        RING("Read %d bytes", len);
        rpc_close(rpcs, fd);
        iter += len;
        total -= len;
    }
    return 0;
}

te_errno
tce_retrieve_data(rcf_rpc_server *rpcs)
{
    int progno = 1;
    char buf[32];
    unsigned version;

    for (;;)
    {
        snprintf(buf, sizeof(buf) - 1, SYS_TCE_PREFIX "%d", progno);
        RPC_AWAIT_IUT_ERROR(rpcs);
        if (rpc_access(rpcs, buf, 0))
            break;
        tce_read_value(rpcs, TCE_GLOBAL(progno), "version", "%x", &version);

        if (version == 0)
            tce_save_data_gcc33(rpcs, progno);
        else
            tce_save_data_gcc34(rpcs, progno, version);
        
        progno++;
    }
    return 0;
}


