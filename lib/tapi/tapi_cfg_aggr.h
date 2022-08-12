/** @file
 * @brief Test API to configure bonding and bridging.
 *
 * @defgroup tapi_conf_aggr Bonding and bridging configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of API to configure linux trunks (IEEE 802.3ad) and bridges.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_AGGR_H__
#define __TE_TAPI_CFG_AGGR_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create bondN interface.
 *
 * @param ta        Test Agent name
 * @param name      Name of aggregation node
 * @param ifname    Where to place the name of created
 *                  interface
 * @param type      Typy of team or bonding
 *
 * @return Status code
 */
extern int tapi_cfg_aggr_create_bond(const char *ta, const char *name,
                                     char **ifname, const char *type);

/**
 * Destroy bondN interface.
 *
 * @param ta        Test Agent name
 * @param name      Name of aggregation node
 *
 * @return Status code
 */
extern int tapi_cfg_aggr_destroy_bond(const char *ta, const char *name);

/**
 * Add a slave interface to a bondN interface.
 *
 * @param ta        Test Agent name
 * @param name      Name of aggregation node
 * @param slave_if  Name of interface to be enslaved
 *
 * @return Status code
 */
extern int tapi_cfg_aggr_bond_enslave(const char *ta, const char *name,
                                      const char *slave_if);

/**
 * Release a slave interface from a bondN interface.
 *
 * @param ta        Test Agent name
 * @param name      Name of aggregation node
 * @param slave_if  Name of interface to be freed
 *
 * @return Status code
 */
extern int tapi_cfg_aggr_bond_free_slave(const char *ta, const char *name,
                                         const char *slave_if);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_AGGR_H__ */

/**@} <!-- END tapi_conf_aggr --> */
