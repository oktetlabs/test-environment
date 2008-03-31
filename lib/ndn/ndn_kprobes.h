/** @file
 * @brief KPROBES NDN
 *
 * Definitions of ASN.1 types for NDN for Kprobes.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Aleksandr Platonov <Aleksandr.Platonov@oktetlabs.ru>
 *
 * $Id:$
 */

#ifndef __TE_NDN_KPROBES_H__
#define __TE_NDN_KPROBES_H__

#if defined(__linux__)

#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

extern asn_type * ndn_kprobes_packet;

#define KPROBES_MAX_FUNC_NAME   64

typedef enum {
    TE_KPROBES_ACTION_SKIP,
    TE_KPROBES_ACTION_FAIL,
    TE_KPROBES_ACTION_BLOCK,
    TE_KPROBES_ACTION_UNBLOCK,
} te_kprobes_action_t;

/* Possible failure results */
/** Driver is not loaded */
#define KPROBES_FAULTS_DRV_LOAD_FAIL     1
/** Interface is not created */
#define KPROBES_FAULTS_IF_CREATE_FAIL    2
/** No failures */
#define KPROBES_FAULTS_NO_FAIL           4

/**
 * Kprobes info structure
 */
typedef struct kprobes_info_s {
    char function_name[KPROBES_MAX_FUNC_NAME]; /**< Intercepted function
                                                    name */
    te_kprobes_action_t action;
    /*char action[KPROBES_MAX_FUNC_NAME]; */       /**< Action to be done with
                                                    call of function_name
                                                    function */
    int intercept_count;
    int retval;                               /**< Value to which will be 
                                                    replaced returned value 
                                                    of funcion_name function;
                                                    unused if action is not 
                                                    equals to "fail" */
    int block_timeout;
    int scenario_index;
    int scenario_item_index;
} kprobes_info_t;

/**
 * Function parses the kprobes info asn string, fills given
 * array of kprobes_info_t structures.
 *
 * @param kprobes_info_str     Kprobes info in asn string representation
 * @param kprobes_info         (OUT) Array of structures, 
 *                             which represents kptobes info
 * @param number_of_structures (OUT) Number of structures in kprobes_info array
 *
 * @return 0 on success or -1
 */
extern int ndn_kprobes_parse_info(const char *kprobes_info_str, 
                                  kprobes_info_t **kprobes_info,
                                  int *number_of_structures);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* defined (__linux__) */

#endif /* __TE_NDN_KPROBES_H__ */
