/** @file
 * @brief TA Common Definitions
 *
 * Prototypes of functions to be implemented on all Test Agents.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TA_COMMON_H__
#define __TE_TA_COMMON_H__

#include "te_defs.h"

/**
 * Get OS identifier of the current thread.
 *
 * @return Thread identifier
 */
extern uint32_t thread_self(void);

/*-- These mutexes should be used in forked processes rather than on TA --*/

/**
 * Create a mutex.
 *
 * @return Mutex handle
 */
extern void *thread_mutex_create(void);

/**
 * Destroy a mutex.
 *
 * @param mutex     mutex handle
 */
extern void thread_mutex_destroy(void *mutex);

/**
 * Lock the mutex.
 *
 * @param mutex     mutex handle
 */
extern void thread_mutex_lock(void *mutex);

/**
 * Unlock the mutex.
 *
 * @param mutex     mutex handle
 */
extern void thread_mutex_unlock(void *mutex);


/**
 * Check that interface is locked for using of this TA
 *
 * @param ifname        name of network interface
 *
 * @retval 0      interface is not locked
 * @retval other  interface is locked
 */
extern te_bool ta_interface_is_mine(const char *ifname);

#endif /* !__TE_TA_COMMON_H__ */
