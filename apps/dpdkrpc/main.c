/** @file
 * @brief DPDK RPC server
 *
 * DPDK-enabled RPC server
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 */

#include <pthread.h>
#include <string.h>

#include <rte_eal.h>
#include <rte_debug.h>

#define TE_USE_SPECIFIC_ASPRINTF 1

#include <ta_common.h>
#include <rpc_server.h>

DEFINE_LGR_ENTITY("DPDK/RPC");

/* FIXME: that is a strictly temporary hack to make linker happy */
static char ta_execname_storage[RCF_MAX_PATH];
const char *ta_execname = ta_execname_storage;
char ta_dir[RCF_MAX_PATH];

/**
 * Get identifier of the current thread.
 *
 * @return Thread identifier
 */
uint32_t
thread_self(void)
{
    return (unsigned int)pthread_self();
}

/**
 * Create a mutex.
 *
 * @return Mutex handle
 */
void *
thread_mutex_create(void)
{
    static pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_t *mutex = (pthread_mutex_t *)calloc(1, sizeof(*mutex));

    if (mutex != NULL)
        *mutex = init;

    return (void *)mutex;
}

/**
 * Destroy a mutex.
 *
 * @param mutex     mutex handle
 */
void
thread_mutex_destroy(void *mutex)
{
    free(mutex);
}

/**
 * Lock the mutex.
 *
 * @param mutex     mutex handle
 *
 */
void
thread_mutex_lock(void *mutex)
{
    if (mutex == NULL)
        ERROR("%s: try to lock NULL mutex", __FUNCTION__);
    else
        pthread_mutex_lock(mutex);
}

/**
 * Unlock the mutex.
 *
 * @param mutex     mutex handle
 */
void
thread_mutex_unlock(void *mutex)
{
    if (mutex == NULL)
        ERROR("%s: try to unlock NULL mutex", __FUNCTION__);
    else
        pthread_mutex_unlock(mutex);
}


/* See description in ta_common.h */
int
rcf_rpc_server_init(void)
{
    return 0;
}

/* See description in ta_common.h */
int
rcf_rpc_server_finalize(void)
{
    return 0;
}

/*
 * TCE support
 */

int (*tce_stop_function)(void);
int (*tce_notify_function)(void);
int (*tce_get_peer_function)(void);
const char *(*tce_get_conn_function)(void);

static pthread_mutex_t rcf_lock = PTHREAD_MUTEX_INITIALIZER;

/* See description in rcf_ch_api.h */
void
rcf_ch_lock(void)
{
    int rc = pthread_mutex_lock(&rcf_lock);

    if (rc != 0)
    {
        LOG_PRINT("%s(): pthread_mutex_lock() failed - rc=%d, errno=%d",
                  __FUNCTION__, rc, errno);
    }
}

/* See description in rcf_ch_api.h */
void
rcf_ch_unlock(void)
{
    int rc;

    rc = pthread_mutex_trylock(&rcf_lock);
    if (rc == 0)
    {
        WARN("rcf_ch_unlock() without rcf_ch_lock()!\n"
             "It may happen in the case of asynchronous cancellation.");
    }
    else if (rc != EBUSY)
    {
        LOG_PRINT("%s(): pthread_mutex_trylock() failed - rc=%d, errno=%d",
                  __FUNCTION__, rc, errno);
    }

    rc = pthread_mutex_unlock(&rcf_lock);
    if (rc != 0)
    {
        LOG_PRINT("%s(): pthread_mutex_unlock() failed - rc=%d, errno=%d",
                  __FUNCTION__, rc, errno);
    }
}

/* END OF HACK */

int
main(int argc, char **argv)
{
	char *sep;

	strcpy(ta_execname_storage, argv[0]);
	strcpy(ta_dir, ta_execname_storage);
	sep = strrchr(ta_dir, '/');
	if (sep == NULL)
		*ta_dir = '\0';
	else
		*sep = '\0';

	if (argc == 1)
		rte_panic("RPC server name must be supplied\n");

	rcf_pch_rpc_server(argv[1]);
	return 0;
}
