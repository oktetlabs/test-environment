/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief ARL table Configuration Model TAPI
 *
 * Definition of test API for ARL table configuration model
 * (storage/cm/cm_poesw.xml).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_ARL_H__
#define __TE_TAPI_CFG_ARL_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "te_queue.h"
#include "te_ethernet.h"
#include "tapi_cfg_arl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_arl ARL table configuration
 * @ingroup tapi_conf
 * @{
 */

/** Types of ARL entries */
typedef enum arl_entry_type {
    ARL_ENTRY_DYNAMIC,   /**< Dynamic ARL entry */
    ARL_ENTRY_STATIC,    /**< Static ARL entry */
} arl_entry_type;


/** ARL table entry structure */
typedef struct arl_entry_t {
    TAILQ_ENTRY(arl_entry_t)    links;      /**< Tail queue links */

    uint8_t         mac[ETHER_ADDR_LEN];    /**< MAC address */
    char           *vlan;                   /**< VLAN name */
    unsigned int    port;                   /**< Port number */
    arl_entry_type  type;                   /**< Entry type */
} arl_entry_t;

#ifndef DEFAULT_VLAN_NAME
/** Default VLAN name used on the Switch */
#define DEFAULT_VLAN_NAME "default"
#endif

/** ARL table head */
typedef TAILQ_HEAD(arl_table_t, arl_entry_t) arl_table_t;


/**
 * Update MAC address to sequntially next.
 *
 * @param mac   - MAC address
 */
static inline void
tapi_mac_next(uint8_t *mac)
{
    int i = ETHER_ADDR_LEN;

    do {
        ++(mac[--i]);
    } while ((mac[i] == 0) && (i > 0));
}


/**
 * Is MAC address broadcast?
 *
 * @param mac       - MAC address
 *
 * @retval TRUE     - broadcast
 * @retval FALSE    - not broadcast
 */
static inline te_bool
tapi_mac_is_broadcast(const uint8_t *mac)
{
    return ((mac[0] & mac[1] & mac[2] &
             mac[3] & mac[4] & mac[5]) == 0xFF) ? TRUE : FALSE;
}


/**
 * Is MAC address multicast?
 *
 * @param mac       - MAC address
 *
 * @retval TRUE     - multicast
 * @retval FALSE    - not multicast
 */
static inline te_bool
tapi_mac_is_multicast(const uint8_t *mac)
{
    return (mac[0] & 1) ? TRUE : FALSE;
}


/**
 * Get ARL table from TA.
 *
 * @param ta        - Test agent name
 * @param sync      - synchronize tree before get
 * @param p_table   - pointer to head of ARL table (initialized
 *                    as empty in the function)
 *
 * @return Status code.
 */
extern int tapi_cfg_arl_get_table(const char *ta, te_bool sync,
                                  arl_table_t *p_table);

/**
 * Delete an ARL entry from ARL table
 *
 * @param ta        - Test agent name
 * @param type      - Entry type (static/dynamic)
 * @param port_num  - Bridge port number of the new ARL entry
 * @param mac_addr  - MAC address of the ARL entry
 * @param vlan_name - VLAN name associated with the ARL entry
 *
 * @return Status of the operation
 */
extern int tapi_cfg_arl_del_entry(const char *ta,
                                  arl_entry_type type,
                                  unsigned int port_num,
                                  const uint8_t *mac_addr,
                                  const char *vlan_name);

/**
 * Add a new ARL entry in ARL table
 *
 * @param ta        - Test agent name
 * @param type      - Entry type (static/dynamic)
 * @param port_num  - Bridge port number of the new ARL entry
 * @param mac_addr  - MAC address of the ARL entry
 * @param vlan_name - VLAN name associated with the ARL entry
 *
 * @return Status of the operation
 */
extern int tapi_cfg_arl_add_entry(const char *ta,
                                  arl_entry_type type,
                                  unsigned int port_num,
                                  const uint8_t *mac_addr,
                                  const char *vlan_name);

/**
 * Get ARL table entry from TA.
 *
 * @param oid       - ARL table OID
 * @param p         - pointer to ARL entry
 *
 * @return Status code.
 */
extern int tapi_cfg_arl_get_entry(const char *oid, arl_entry_t *p);

/**
 * Free resources allocated inside ARL entry. DO NOT free memory
 * used by ARL entry itself.
 *
 * @param p         - pointer to ARL entry
 */
extern void tapi_arl_free_entry(arl_entry_t *p);

/**
 * Free resources allocated for ARL table. DO NOT free memory
 * used by ARL table head.
 *
 * @param p_table   - pointer to ARL table
 */
extern void tapi_arl_free_table(arl_table_t *p_table);

/**
 * Find ARL entry with specified fields.
 *
 * @param p_table   - ARL table
 * @param mac       - ARL MAC address
 * @param vlan      - name of VLAN
 * @param port      - ARL port
 * @param type      - type of ARL entry
 *
 * @return Pointer to ARL entry or NULL
 */
extern arl_entry_t * tapi_arl_find(const arl_table_t *p_table,
                                   const uint8_t *mac, const char *vlan,
                                   unsigned int port,
                                   enum arl_entry_type type);

/**
 * Dump the content of ARL table into log file.
 *
 * @param p_table  - ARL table
 */
extern void tapi_arl_print_table(const arl_table_t *p_table);

/**@} <!-- END tapi_conf_arl --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_ARL_H__ */
