
/** @file
 * @brief Common declarations for utilities, used in CWMP-related tests 
 *  for make some management of CPE behind CWMP. 
 *
 * Common definitions for RPC test suite.
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 * 
 * $Id$
 */
#ifndef __CPE_BACKDOOR__H__
#define __CPE_BACKDOOR__H__

/* TODO: invent actual and portable ID for box, which will 
   help background implementation to connect with it.
   This is temporary. */
typedef struct board_id_s {
    const char              *ta;
    rcf_rpc_server          *pco;
    struct sockaddr_storage  addr;
} board_id_t;

/**
 * Get URL for ConnectionRequest from CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param cr_url        location for URL.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_cr_url(board_id_t cpe,
                               char *cr_url, size_t bufsize);

/**
 * Get ACS URL from CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param acs_url       location for URL.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_acs_url(board_id_t cpe, char *acs_url,
                                size_t bufsize);

/**
 * Get URL of ACS from CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param acs_url       location for URL.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_acs_url(board_id_t cpe,
                                char *acs_url, size_t bufsize);

/**
 * Set URL of ACS on CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param acs_url       string with URL.
 *
 * @return status
 */
extern te_errno cpe_set_acs_url(board_id_t cpe, char *acs_url);

/**
 * Activate TR069 management protocol on CPE. 
 * Set URL of ACS on CPE, if specified.
 *
 * @param cpe           ID of board with CPE.
 * @param acs_url       string with URL, may be NULL.
 *
 * @return status
 */
extern te_errno cpe_activate_tr069_mgmt(board_id_t cpe, char *acs_url);

/**
 * Set ConnectionRequest login username on CPE, that is, login for
 * authenticate ConnectionRequest from ACS to CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param cr_login      username.
 *
 * @return status
 */
extern te_errno cpe_set_cr_login(board_id_t cpe,  const char *cr_login);

/**
 * Set ConnectionRequest login password on CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param cr_passwd     password.
 *
 * @return status
 */
extern te_errno cpe_set_cr_passwd(board_id_t cpe, const char *cr_passwd);

/**
 * Get ConnectionRequest login username on CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param cr_login      location for username.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_cr_login(board_id_t cpe,
                                 char *cr_login, size_t bufsize);

/**
 * Get ConnectionRequest login password on CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param cr_passwd     location for password.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_cr_passwd(board_id_t cpe,
                                  char *cr_passwd, size_t bufsize);

/**
 * Set ASC login username on CPE, that is login for authenticate
 * CPE while starting CWMP session (CPE to ACS).
 *
 * @param cpe           ID of board with CPE.
 * @param acs_login     username.
 *
 * @return status
 */
extern te_errno cpe_set_acs_login(board_id_t cpe,  const char *acs_login);

/**
 * Set ACS password on CPE for CWMP sessions.
 *
 * @param cpe           ID of board with CPE.
 * @param acs_passwd    password.
 *
 * @return status
 */
extern te_errno cpe_set_acs_passwd(board_id_t cpe, const char *acs_passwd);

/**
 * Get ACS login username on CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param acs_login     location for username.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_acs_login(board_id_t cpe,
                                  char *acs_login, size_t bufsize);

/**
 * Get ACS login password on CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param acs_passwd    location for password.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_acs_passwd(board_id_t cpe,
                                   char *acs_passwd, size_t bufsize);


#endif /* __CPE_BACKDOOR__H__ */ 
