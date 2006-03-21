/** @file
 * @brief Functions to opearate with generic "struct sockaddr"
 *
 * Implementation of test API for working with struct sockaddr.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER      "TAPI SockAddr"

#include "te_config.h"

#include <stdio.h>
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "logger_api.h"
#include "conf_api.h"

#include "tapi_sockaddr.h"


/* See description in tapi_env.h */
te_errno
tapi_allocate_port(uint16_t *p_port)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    int             rc;
    cfg_val_type    val_type;
    int             port;
    
    pthread_mutex_lock(&mutex);

    val_type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&val_type, &port,
                              "/volatile:/sockaddr_port:");
    if (rc != 0)
    {
        pthread_mutex_unlock(&mutex);
        ERROR("Failed to get /volatile:/sockaddr_port:: %r", rc);
        return rc;
    }
    if (port < 0 || port > 0xffff)
    {
        pthread_mutex_unlock(&mutex);
        ERROR("Wrong value %d is got from /volatile:/sockaddr_port:", port);
        return TE_EINVAL;
    }
    if ((port < 20000) || (port >= 30000))
    {
        /* Random numbers generator should be initialized earlier */
        port = 20000 + rand_range(0, 10000);
    }
    else
    {
        port++;
    }
    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, port),
                              "/volatile:/sockaddr_port:");
    if (rc != 0)
    {
        pthread_mutex_unlock(&mutex);
        ERROR("Failed to set /volatile:/sockaddr_port:: %r", rc);
        return rc;
    }

    pthread_mutex_unlock(&mutex);

    *p_port = (uint16_t)port;

    return 0;
}
