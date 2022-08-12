/** @file
 * @brief Mapping of unix signal name->number and number->name
 *
 * @defgroup te_tools_te_sigmap Mapping of unix signal names and numbers
 * @ingroup te_tools
 * @{
 *
 * Definition of the mapping functions.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifndef __TE_SIGMAP_H__
#define __TE_SIGMAP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Map signal name to the number
 *
 * @param name  Name of the signal
 *
 * @return Signal number or @c -1 in case of error
 */
extern int map_name_to_signo(const char *name);


/**
 * Map signal number to the name
 *
 * @param name  Number of the signal
 *
 * @return Signal name or NULL
 */
extern char * map_signo_to_name(int signo);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ndef __TE_SIGMAP_H__ */
/**@} <!-- END te_tools_te_sigmap --> */
