/** @file
 * @brief Test API to configure ACSE.
 *
 * Definition of API to configure ACSE.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id:
 */

#ifndef __TE_TAPI_CFG_ACSE_H__
#define __TE_TAPI_CFG_ACSE_H__

#include "te_errno.h"
#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_acse TR-069 Auto Configuration Server Engine (ACSE) configuration
 * @ingroup tapi_conf
 * @{
 */

/** Session states */
typedef enum { session_no_state,
               session_disconnected,
               session_connected,
               session_authenticated,
               session_preinitiated,
               session_initiated,
               session_inside_transaction,
               session_outside_transaction
} session_state_t;

/**
 * Start ACSE.
 *
 * @param ta            Test Agent name
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_start(char const *ta);

/**
 * Stop ACSE.
 *
 * @param ta            Test Agent name
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_stop(char const *ta);

/**
 * Add ACS object.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_add_acs(char const *ta, char const *acs);

/**
 * Set ACS object url parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param url           ACS object url parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_acs_url(char const *ta,
                                          char const *acs,
                                          char const *url);

/**
 * Get ACS object url parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param url           ACS object url parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_acs_url(char const *ta,
                                          char const *acs,
                                          char const **url);

/**
 * Set ACS object cert parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cert          ACS object cert parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_acs_cert(char const *ta,
                                           char const *acs,
                                           char const *cert);

/**
 * Get ACS object cert parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cert          ACS object cert parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_acs_cert(char const *ta,
                                           char const *acs,
                                           char const **cert);

/**
 * Set ACS object user parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param user          ACS object user parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_acs_user(char const *ta,
                                           char const *acs,
                                           char const *user);

/**
 * Get ACS object user parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param user          ACS object user parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_acs_user(char const *ta,
                                           char const *acs,
                                           char const **user);

/**
 * Set ACS object pass parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param pass          ACS object pass parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_acs_pass(char const *ta,
                                           char const *acs,
                                           char const *pass);

/**
 * Get ACS object pass parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param pass          ACS object pass parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_acs_pass(char const *ta,
                                           char const *acs,
                                           char const **pass);

/**
 * Set ACS object enabled parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param enabled       ACS object enabled parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_acs_enabled(char const *ta,
                                              char const *acs,
                                              te_bool enabled);

/**
 * Get ACS object enabled parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param enabled       ACS object enabled parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_acs_enabled(char const *ta,
                                              char const *acs,
                                              te_bool *enabled);

/**
 * Set ACS object ssl parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param ssl           ACS object ssl parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_acs_ssl(char const *ta,
                                          char const *acs,
                                          te_bool ssl);

/**
 * Get ACS object ssl parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param ssl           ACS object ssl parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_acs_ssl(char const *ta,
                                          char const *acs,
                                          te_bool *ssl);

/**
 * Set ACS object port parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param port          ACS object port parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_acs_port(char const *ta,
                                           char const *acs,
                                           int port);

/**
 * Get ACS object port parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param port          ACS object port parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_acs_port(char const *ta,
                                           char const *acs,
                                           int *port);

/**
 * Add ACS object and then set its parameters.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param url           ACS object url parameter
 * @param cert          ACS object cert parameter
 * @param user          ACS object user parameter
 * @param pass          ACS object pass parameter
 * @param ssl           ACS object ssl parameter
 * @param port          ACS object port parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_add_acs_with_params(char const *ta,
                                                  char const *acs,
                                                  char const *url,
                                                  char const *cert,
                                                  char const *user,
                                                  char const *pass,
                                                  te_bool ssl,
                                                  int port);

/**
 * Delete ACS object.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_del_acs(char const *ta, char const *acs);

/**
 * Add CPE object.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_add_cpe(char const *ta,
                                      char const *acs,
                                      char const *cpe);

/**
 * Set CPE object url parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param url           CPE object url parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_cpe_url(char const *ta,
                                          char const *acs,
                                          char const *cpe,
                                          char const *url);

/**
 * Get CPE object url parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param url           CPE object url parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_cpe_url(char const *ta,
                                          char const *acs,
                                          char const *cpe,
                                          char const **url);

/**
 * Set CPE object cert parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param cert          CPE object cert parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_cpe_cert(char const *ta,
                                           char const *acs,
                                           char const *cpe,
                                           char const *cert);

/**
 * Get CPE object cert parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param cert          CPE object cert parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_cpe_cert(char const *ta,
                                           char const *acs,
                                           char const *cpe,
                                           char const **cert);

/**
 * Set CPE object user parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param user          CPE object user parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_cpe_user(char const *ta,
                                           char const *acs,
                                           char const *cpe,
                                           char const *user);

/**
 * Get CPE object user parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param user          CPE object user parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_cpe_user(char const *ta,
                                           char const *acs,
                                           char const *cpe,
                                           char const **user);

/**
 * Set CPE object pass parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param pass          CPE object pass parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_cpe_pass(char const *ta,
                                           char const *acs,
                                           char const *cpe,
                                           char const *pass);

/**
 * Get CPE object pass parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param pass          CPE object pass parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_cpe_pass(char const *ta,
                                           char const *acs,
                                           char const *cpe,
                                           char const **pass);

/**
 * Set CPE object IP address parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param url           CPE object url parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_cpe_ip_addr(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    struct sockaddr const *addr);

/**
 * Get CPE object IP address parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param url           CPE object url parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_cpe_ip_addr(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    struct sockaddr const **addr);

/**
 * Set CPE/session object enabled parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param enabled       CPE/session object enabled parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_session_enabled(char const *ta,
                                                  char const *acs,
                                                  char const *cpe,
                                                  te_bool enabled);

/**
 * Get CPE/session object enabled parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param enabled       CPE/session object enabled parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_session_enabled(char const *ta,
                                                  char const *acs,
                                                  char const *cpe,
                                                  te_bool *enabled);

/**
 * Set CPE/session object hold_requests parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param hold_requests CPE/session object hold_requests parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_session_hold_requests(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    te_bool hold_requests);

/**
 * Get CPE/session object hold_requests parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param hold_requests CPE/session object hold_requests parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_session_hold_requests(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    te_bool *hold_requests);

/**
 * Set CPE/session object target_state parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param target_state  CPE/session object target_state parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_set_session_target_state(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    session_state_t target_state);

/**
 * Get CPE/session object target_state parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param target_state  CPE/session object target_state parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_session_target_state(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    session_state_t *target_state);

/**
 * Get CPE/session object state parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param state         CPE/session object state parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_session_state(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    session_state_t *state);

/**
 * Get CPE/device_id object manufacturer parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param manufacturer  CPE/device_id object manufacturer parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_device_id_manufacturer(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    char const **manufacturer);

/**
 * Get CPE/device_id object oui parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param oui           CPE/device_id object oui parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_device_id_oui(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    char const **oui);

/**
 * Get CPE/device_id object product_class parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param product_class CPE/device_id object product_class parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_device_id_product_class(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    char const **product_class);

/**
 * Get CPE/device_id object serial_number parameter.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param serial_number CPE/device_id object serial_number parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_get_device_id_serial_number(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    char const **serial_number);

/**
 * Add CPE object and then set its parameters.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * @param url           CPE object url parameter
 * @param cert          CPE object cert parameter
 * @param user          CPE object user parameter
 * @param pass          CPE object pass parameter
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_add_cpe_with_params(
                    char const *ta,
                    char const *acs,
                    char const *cpe,
                    char const *url,
                    char const *cert,
                    char const *user,
                    char const *pass,
                    struct sockaddr const *addr);

/**
 * Delete CPE object.
 *
 * @param ta            Test Agent name
 * @param acs           ACS object name
 * @param cpe           CPE object name
 * 
 * @return              Status code
 */
extern te_errno tapi_cfg_acse_del_cpe(char const *ta,
                                      char const *acs,
                                      char const *cpe);

/**@} <!-- END tapi_conf_acse --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_CFG_ACSE_H__ */
