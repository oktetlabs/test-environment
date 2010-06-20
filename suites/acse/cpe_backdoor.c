/** @file
 * @brief Functions, used in CWMP-related tests for make 
 * some management of CPE behind CWMP. 
 * 
 * @author Marina Maslova <Marina.Maslova@oktetlabs.ru>
 * 
 * $Id: $
 */

#include "cpe_backdoor.h"
#include "platform-ts.h"
#include "tapi_rpc_crm.h"
#include "tapi_webui.h"

#define ACS_CONNREQ_URL         "/tr069/acs/conn_req/url"
#define ACS_CONNREQ_LOGIN       "/tr069/acs/conn_req/username"

/**
 * Get URL for ConnectionRequest from CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param cr_url        location for URL.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
te_errno
cpe_get_cr_url(board_id_t cpe, char *cr_url, size_t bufsize)
{
    rpc_ptr             mapi;
    crm_tc_id           tid;
    const char         *tmp;

    rpc_http_webui_login(cpe.pco, (struct sockaddr *)&cpe.addr, "root",
                         tapi_cfg_get_webui_passwd("root"));
    tapi_http_webui_access_on(cpe.pco, (struct sockaddr *)&cpe.addr);

    RPC_TRANSACTION_OPEN(cpe.pco, (struct sockaddr *)&cpe.addr,
                         CRM_TC_RO, TC_TIMEO);
    rpc_crm_get_string(cpe.pco, mapi, TC_USER, tid, &tmp,
                       ACS_CONNREQ_URL);
    RPC_TRANSACTION_CLOSE(cpe.pco);

    if (bufsize < strlen(tmp) + 1)
        return TE_ENOBUFS;

    strcpy(cr_url, tmp);

    return 0;
}

/**
 * Set ConnectionRequest login username on CPE, that is, login for
 * authenticate ConnectionRequest from ACS to CPE.
 *
 * @param cpe           ID of board with CPE.
 * @param cr_login      username.
 *
 * @return status
 */
te_errno
cpe_set_cr_login(board_id_t cpe,  const char *cr_login)
{
    rpc_ptr             mapi;
    crm_tc_id           tid;
    const char         *tmp;

    rpc_http_webui_login(cpe.pco, (struct sockaddr *)&cpe.addr, "root",
                         tapi_cfg_get_webui_passwd("root"));
    tapi_http_webui_access_on(cpe.pco, (struct sockaddr *)&cpe.addr);

    RPC_TRANSACTION_OPEN(cpe.pco, (struct sockaddr *)&cpe.addr,
                         CRM_TC_RW, TC_TIMEO);
    rpc_crm_set_string(cpe.pco, mapi, TC_USER, tid, cr_login,
                       ACS_CONNREQ_LOGIN);
    RPC_TRANSACTION_CLOSE(cpe.pco);

    return 0;
}
#if 0
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

#endif
