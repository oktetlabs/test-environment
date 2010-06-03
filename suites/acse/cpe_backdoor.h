
/** @file
 * @brief Common declarations for utilities, used in CWMP-related tests 
 *  for make some management of CPE behind CWMP. 
 *
 * Common definitions for RPC test suite.
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 * 
 * $Id: $
 */
#ifndef __CPE_BACKDOOR__H__
#define __CPE_BACKDOOR__H__

/* TODO: invent actual and portable ID for box, which will 
   help background implementation to connect with it.
   This is temporary. */
typedef const char * board_id_t;

extern te_errno cpe_get_cr_url(board_id_t cpe, char *buf, size_t bufsize);

extern te_errno cpe_get_acs_url(board_id_t cpe, char *buf, size_t bufsize);

extern te_errno cpe_set_acs_url(board_id_t cpe, char *acs_url);

extern te_errno cpe_activate_tr069_mgmt(board_id_t cpe, char *acs_url);

#endif /* __CPE_BACKDOOR__H__ */

