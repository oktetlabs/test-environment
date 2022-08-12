/** @file
 * @brief Logger subsystem API - TA side
 *
 * TA side Logger functionality for
 * forked TA processes and newly created threads
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Mamadou Ngom <Mamadou.Ngom@oktetlabs.ru>
 *
 */

#ifndef __TE_LIB_LOGFORK_H__
#define __TE_LIB_LOGFORK_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register process name and pid, so it would be
 * possible to know from which process or thread message
 * has been sent.
 *
 * If the process is already registered, change its name.
 *
 * @param name  process or thread name
 *
 * @retval  0 success
 * @retval -1 failure
 */
extern int logfork_register_user(const char *name);

/**
 * Delete user with a given pid and tid.
 *
 * @param pid  process id
 * @param tid  thread id
 *
 * @retval  0 success
 * @retval -1 failure
 */
extern int logfork_delete_user(pid_t pid, uint32_t tid);

/**
 * Enable or disable logging of process name and id in messages
 * (enabled by default)
 *
 * @param enabled  @c TRUE - enable, @c FALSE - disable
 *
 * @retval  0 success
 * @retval -1 failure
 */
extern int logfork_set_id_logging(te_bool enabled);

/**
 * Entry point for log gathering.
 */
extern void logfork_entry(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif /* !__TE_LIB_LOGFORK_H__ */
