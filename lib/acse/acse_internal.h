/** @file
 * @brief ACSE API
 *
 * ACSE declarations.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id: acse_internal.h 31892 2006-09-26 07:12:12Z arybchik $
 */

#ifndef __TE_LIB_ACSE_INTERNAL_H__
#define __TE_LIB_ACSE_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_errno.h"
#include "acse.h"

typedef enum { lrpc_type, listenning_type, connection_type } item_type_t;

/** Abstraction structure for the 'channel' object */
typedef struct channel_t {
    item_type_t type;           /**< The type of an item                */
    void       *data;           /**< Channel-specific private data      */
    te_errno  (*before_select)( /**< Called before 'select' syscall     */
        void   *data,           /**< Channel-specific private data      */
        fd_set *rd_set,         /**< Descriptor set checked for reading */
        fd_set *wr_set,         /**< Descriptor set checked for writing */
        int    *fd_max);        /**< Storage for maximal of descriptors */
    te_errno  (*after_select)(  /**< Called after 'select' syscall      */
        void   *data,           /**< Channel-specific private data      */
        fd_set *rd_set,         /**< Descriptor set checked for reading */
        fd_set *wr_set);        /**< Descriptor set checked for writing */
    te_errno  (*destroy)(       /**< Called on destroy                  */
        void   *data);          /**< Channel-specific private data      */
    te_errno  (*error_destroy)( /**< Called on error and destroy        */
        void   *data);          /**< Channel-specific private data      */
} channel_t;

typedef struct channel_item_t
{
    STAILQ_ENTRY(channel_item_t) link;
    channel_t                    channel;
} channel_item_t;

extern te_errno
acse_lrpc_create(channel_t *channel, params_t *params,
                 int rd_fd, int wr_fd);

extern te_errno
acse_conn_create(channel_t *channel);

extern te_errno
acse_sreq_create(channel_t *channel);

extern te_errno
acse_cwmp_create(channel_t *channel);

#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_INTERNAL_H__ */
