/** @file
 * @brief Test API to configure sniffers.
 *
 * Definition of API to configure sniffers.
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id: 
 */

#ifndef __TE_TAPI_SNIFFER_H__
#define __TE_TAPI_SNIFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_queue.h"
#include "te_defs.h"

 /** 
  * Sniffer identifier
  */
typedef struct tapi_sniffer_id {
    char  ta[CFG_SUBID_MAX];        /**< Test Agent name */
    char  ifname[CFG_SUBID_MAX];    /**< Interface name */
    char  snifname[CFG_SUBID_MAX];  /**< Sniffer name */
    int   ssn;                      /**< Sniffer session sequence number */
} tapi_sniffer_id;

/**
 * Sniffers list
 */
typedef struct tapi_sniff_list_s {
    tapi_sniffer_id                   *sniff;     /**< Sniffer ID */
    SLIST_ENTRY(tapi_sniff_list_s)     tapi_sniff_ent;
} tapi_sniff_list_s;
SLIST_HEAD(sniffl_h_t, tapi_sniff_list_s);
typedef struct sniffl_h_t sniffl_h_t;

/**
 * Create and start new sniffer.
 * 
 * @param ta            Agent name
 * @param iface         Interface name
 * @param name          Sniffer name
 * @c     NULL          Name will be generated automatically
 * @param filter        Filter expression
 * @c     NULL          If NULL is specified - the filter is not applied
 * @param ofill         If FALSE is specified, than overfill handling method
 *                      is rotation. Otherwise - tail drop
 *
 * @return Pointer to identifier structure of the sniffer. In case of
 *         failure NULL will be returned.
 */
extern tapi_sniffer_id * tapi_sniffer_add(const char *ta, const char *iface,
                                          const char *name,
                                          const char *filter,
                                          te_bool ofill);

/**
 * Create and start one or more new sniffers on the agent.
 * 
 * @param ta            Agent name
 * @param iface         Interface name
 * @c     NULL          Start sniffers on all interfaces expect loopback
 * @param name          Sniffer name
 * @c     NULL          Name will be generated automatically
 * @param filter        Filter expression
 * @c     NULL          If NULL is specified - the filter is not applied
 * @param ofill         If FALSE is specified, than overfill handling method
 *                      is rotation. Otherwise - tail drop
 * @param snif_h        Pointer to head of sniffer list
 *
 * @return Status code
 * @retval 0            success
 */
extern te_errno tapi_sniffer_add_mult(const char *ta, const char *iface,
                                      const char *name, const char *filter,
                                      te_bool ofill, sniffl_h_t *snif_h);

/**
 * Stop and destroy the sniffer.
 * 
 * @param id            ID of the sniffer
 *
 * @return Status code 
 * @retval 0            success
 */
extern te_errno tapi_sniffer_del(tapi_sniffer_id *id);

/**
 * Disable the sniffer.
 * 
 * @param id            ID of the sniffer
 *
 * @return Status code 
 * @retval 0            success
 */
extern te_errno tapi_sniffer_stop(tapi_sniffer_id *id);

/**
 * Enable the sniffer.
 * 
 * @param id            ID of the sniffer
 *
 * @return Status code 
 * @retval 0            success
 */
extern te_errno tapi_sniffer_start(tapi_sniffer_id *id);

/**
 * Insert a mark packet into the capture file for a certain
 * sniffer.
 * 
 * @param ta             Agent name used to request mark for all sniffers.
 *                       It is not used if the ID not NULL
 * @c     NULL           The sniffer ID should not be NULL
 * @param id             ID of the sniffer. If ID not NULL is queried mark
 *                       for one sniffer
 * @c     NULL           If ID is NULL is queried mark for all sniffers on
 *                       the agent. The ta param should not be NULL
 * @param description    Mark description in human-readable form
 * @c     NULL           NULL may be used as description for empty
 *                       description string
 * 
 * @return Status code 
 * @retval 0            success
 */
extern te_errno tapi_sniffer_mark(const char *ta, tapi_sniffer_id *id,
                                  const char *description);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef __TE_TAPI_SNIFFER_H__ */
