/** @file
 * @brief Linux Test Agent
 *
 * Linux TA definitions
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_TA_LINUX_INTERNAL_H__
#define __TE_TA_LINUX_INTERNAL_H__


/** Fast conversion of the network mask to prefix */
#define MASK2PREFIX(mask, prefix)            \
    switch (mask)                            \
    {                                        \
        case 0x0: prefix = 0; break;         \
        case 0x80000000: prefix = 1; break;  \
        case 0xc0000000: prefix = 2; break;  \
        case 0xe0000000: prefix = 3; break;  \
        case 0xf0000000: prefix = 4; break;  \
        case 0xf8000000: prefix = 5; break;  \
        case 0xfc000000: prefix = 6; break;  \
        case 0xfe000000: prefix = 7; break;  \
        case 0xff000000: prefix = 8; break;  \
        case 0xff800000: prefix = 9; break;  \
        case 0xffc00000: prefix = 10; break; \
        case 0xffe00000: prefix = 11; break; \
        case 0xfff00000: prefix = 12; break; \
        case 0xfff80000: prefix = 13; break; \
        case 0xfffc0000: prefix = 14; break; \
        case 0xfffe0000: prefix = 15; break; \
        case 0xffff0000: prefix = 16; break; \
        case 0xffff8000: prefix = 17; break; \
        case 0xffffc000: prefix = 18; break; \
        case 0xffffe000: prefix = 19; break; \
        case 0xfffff000: prefix = 20; break; \
        case 0xfffff800: prefix = 21; break; \
        case 0xfffffc00: prefix = 22; break; \
        case 0xfffffe00: prefix = 23; break; \
        case 0xffffff00: prefix = 24; break; \
        case 0xffffff80: prefix = 25; break; \
        case 0xffffffc0: prefix = 26; break; \
        case 0xffffffe0: prefix = 27; break; \
        case 0xfffffff0: prefix = 28; break; \
        case 0xfffffff8: prefix = 29; break; \
        case 0xfffffffc: prefix = 30; break; \
        case 0xfffffffe: prefix = 31; break; \
        case 0xffffffff: prefix = 32; break; \
         /* Error indication */              \
        default: prefix = 33; break;         \
    }

/** Fast conversion of the prefix to network mask */
#define PREFIX2MASK(prefix) (prefix == 0 ? 0 : (~0) << (32 - (prefix)))


/** TA name pointer */
extern char *ta_name;

/** Command line */ 
extern char *ta_execname;

/* TA pid */
extern int ta_pid;

extern int ftp_open(char *uri, int flags, int passive, int offset);

/* Work-around for system/waitpid problem */
extern int ta_system(char *cmd);

#define PRINT(msg...) \
    do {                                                \
       printf(msg); printf("\n"); fflush(stdout);       \
    } while (0)

#endif /* __TE_TA_LINUX_INTERNAL_H__ */
