/** @file
 * @brief RCF Portable Command Handler
 *
 * Default vread, vwrite and execute commands handlers.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "te_errno.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "comm_agent.h"
#include "rcf_common.h"
#include "rcf_pch.h"
#include "rcf_ch_api.h"
#include "rcf_pch_internal.h"

/* See description in rch_pch.h */
int
rcf_pch_vread(struct rcf_comm_connection *conn,
              char *cbuf, size_t buflen, size_t answer_plen,
              rcf_var_type_t type, const char *var)
{
    void *addr;


    ENTRY("type=%d var='%s'", type, var);
    VERB("Default vread handler is executed");

#ifdef HAVE_TIME_H
    if (strcmp(var, "time") == 0)
    {
        struct timeval tv;
        time_t         t;
        struct tm      tm;

        gettimeofday(&tv, NULL);
        t = (time_t)(tv.tv_sec);
        localtime_r(&t, &tm);

        SEND_ANSWER("0 %02d:%02d:%02d",
                    tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
#endif

    if ((addr = rcf_ch_symbol_addr(var, 0)) == NULL)
    {
#ifdef HAVE_STDLIB_H
        if (type == RCF_STRING)
        {
            /* Try to find in the environment */
            char *env_val = getenv(var);

            if (env_val != NULL)
            {
                if (strlen(env_val) < RCF_MAX_VAL)
                {
                    char  val[RCF_MAX_VAL * 2 + 2];

                    write_str_in_quotes(val, env_val, RCF_MAX_VAL);
                    SEND_ANSWER("0 %s", val);
                }
                else
                {
                    SEND_ANSWER("%d", E2BIG);
                }
            }
        }
#endif
        SEND_ANSWER("%d", TE_ENOENT);
    }

    switch (type)
    {
        case RCF_INT8:
        {
            int8_t val;

            memcpy((void *)&val, addr, sizeof(int8_t));
            SEND_ANSWER("0 %d", (int)val);
        }

        case RCF_UINT8:
        {
            uint8_t val;

            memcpy((void *)&val, addr, sizeof(uint8_t));
            SEND_ANSWER("0 %u", (unsigned int)val);
        }

        case RCF_INT16:
        {
            int16_t val;

            memcpy((void *)&val, addr, sizeof(int16_t));
            SEND_ANSWER("0 %d", (int)val);
        }

        case RCF_UINT16:
        {
            uint16_t val;

            memcpy((void *)&val, addr, sizeof(uint16_t));
            SEND_ANSWER("0 %u", (unsigned int)val);
        }

        case RCF_INT32:
        {
            int32_t val;

            memcpy((void *)&val, addr, sizeof(int32_t));
            /* FIXME */
            SEND_ANSWER("0 %d", (int)val);
        }

        case RCF_UINT32:
        {
            uint32_t val;

            memcpy((void *)&val, addr, sizeof(uint32_t));
            /* FIXME */
            SEND_ANSWER("0 %u", (unsigned int)val);
        }

        case RCF_INT64:
        {
            int64_t val;

            memcpy((void *)&val, addr, sizeof(int64_t));
            /* FIXME */
            SEND_ANSWER("0 %lld", (long long int)val);
        }

        case RCF_UINT64:
        {
            uint64_t val;

            memcpy((void *)&val, addr, sizeof(uint64_t));
            /* FIXME */
            SEND_ANSWER("0 %llu", (unsigned long long int)val);
        }

        case RCF_STRING:
            if (strlen(*(char **)addr) < RCF_MAX_VAL)
            {
                char val[RCF_MAX_VAL * 2 + 2];

                write_str_in_quotes(val, *(char **)addr, RCF_MAX_VAL);
                SEND_ANSWER("0 %s", val);
            }
            else
            {
                SEND_ANSWER("%d", E2BIG);
            }

        default:
            assert(FALSE);
    }

    /* Unreachable */
    assert(FALSE);
    return 0;
}


/* See description in rcf_ch_api.h */
int
rcf_pch_vwrite(struct rcf_comm_connection *conn,
               char *cbuf, size_t buflen, size_t answer_plen,
               rcf_var_type_t type, const char *var, ...)
{
    va_list     ap;
    void       *addr;


    ENTRY("type=%d var='%s'", type, var);
    VERB("Default vwrite handler is executed");

    va_start(ap, var);
#ifdef HAVE_TIME_H
    if (strcmp(var, "time") == 0)
    {
        unsigned sec, usec;
        struct timeval tv;

        VERB("synchronizing time");
        if (sscanf(va_arg(ap, const char *), "%u:%u", &sec, &usec) != 2)
        {
            va_end(ap);
            SEND_ANSWER("%d", TE_EFMT);
        }
       
        tv.tv_sec  = sec;
        tv.tv_usec = usec;
#ifndef VGDEBUG /* Valgrind debugging */
        if (settimeofday(&tv, NULL) != 0)
        {
            va_end(ap);
            SEND_ANSWER("%d", errno);
        }
#else
        ERROR("Ignore set of time since Valgrind debugging is enabled "
              "in build");
#endif
        va_end(ap);
        SEND_ANSWER("0");
    }
#endif

    if ((addr = rcf_ch_symbol_addr(var, 0)) == NULL)
    {
#ifdef HAVE_STDLIB_H
        if (type == RCF_STRING)
        {
            setenv(var, va_arg(ap, const char *), 1);
            va_end(ap);
            SEND_ANSWER("0");
        }
#endif
        va_end(ap);
        SEND_ANSWER("%d", TE_ENOENT);
    }

    switch (type)
    {
        case RCF_INT8:
            *(int8_t *)addr = va_arg(ap, int);
            break;

        case RCF_UINT8:
            *(uint8_t *)addr = va_arg(ap, unsigned int);
            break;

        case RCF_INT16:
            *(int16_t *)addr = va_arg(ap, int);
            break;

        case RCF_UINT16:
            *(uint16_t *)addr = va_arg(ap, unsigned int);
            break;

        case RCF_INT32:
            *(int32_t *)addr = va_arg(ap, int32_t);
            break;

        case RCF_UINT32:
            *(uint32_t *)addr = va_arg(ap, uint32_t);
            break;

        case RCF_INT64:
            *(int64_t *)addr = va_arg(ap, int64_t);
            break;

        case RCF_UINT64:
            *(uint64_t *)addr = va_arg(ap, uint64_t);
            break;

        case RCF_STRING:
            strcpy(*(char **)addr, va_arg(ap, const char *));
            break;

        default:
            assert(FALSE);
    }

    va_end(ap);
    SEND_ANSWER("0");
}

/* See description in rcf_ch_api.h */
int
rcf_pch_call(struct rcf_comm_connection *conn,
             char *cbuf, size_t buflen, size_t answer_plen,
             const char *rtn, te_bool is_argv, int argc,
             void **params)
{
    void *addr;
    int   rc;

    ENTRY("rtn='%s' is_argv=%d argc=%d params=%x",
          rtn, is_argv, argc, params);
    VERB("Default call handler is executed");

    addr = rcf_ch_symbol_addr(rtn, 1);
    if (addr == NULL)
    {
        ERROR("The routine '%s' is not found", rtn);
        SEND_ANSWER("%d", TE_ENOENT);
    }

    if (is_argv)
        rc = ((rcf_argv_rtn)(addr))(argc, (char **)params);
    else
        rc = ((rcf_rtn)(addr))(params[0], params[1], params[2],
                               params[3], params[4], params[5],
                               params[6], params[7], params[8],
                               params[9]);

    SEND_ANSWER("0 %d", rc);
}
