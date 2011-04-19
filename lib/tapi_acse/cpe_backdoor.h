
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

struct cpe_id_s;
typedef struct cpe_id_s cpe_id_t;

/* TODO: invent actual and portable ID for box, which will 
   help background implementation to connect with it.
   This is temporary. */
struct cpe_id_s {
    const char              *ta;
    rcf_rpc_server          *pco;
    struct sockaddr_storage  addr;
};

/**
 * Initiate network configuration around CPE and its internal mirror.
 * Should be called at the start of TestSuite, before all other utils
 * from this library.
 */
extern te_errno cpe_network_cfg_init(int argc, char **argv);

/**
 * Init CPE ID.
 */
extern cpe_id_t *cpe_id_init(void);

/**
 * Get network address of the ACS address.
 * Operation depends only from TestSuite network configuration, 
 * and specific CPE options, and should not have any matter
 * to the TR client on the CPE.
 *
 * @param cpe           Internal ID of CPE
 * @param addr          Location for the address,
 *                      should have enough size.
 *
 * @return Status code.
 */
extern te_errno cpe_get_cfg_acs_addr(cpe_id_t *cpe, struct sockaddr *addr);


/**
 * Get URL for ConnectionRequest from CPE.
 *
 * @param cpe           ID of CPE.
 * @param cr_url        location for URL.
 * @param bufsize       size of location buffer.
 *
 * @return Status code.
 */
extern te_errno cpe_get_cr_url(cpe_id_t *cpe,
                               char *cr_url, size_t bufsize);

/**
 * Get ACS URL from CPE.
 *
 * @param cpe           ID of cpe with CPE.
 * @param acs_url       location for URL.
 * @param bufsize       size of location buffer.
 *
 * @return Status code.
 */
extern te_errno cpe_get_acs_url(cpe_id_t *cpe, char *acs_url,
                                size_t bufsize);

/**
 * Get URL of ACS from CPE.
 *
 * @param cpe           ID of CPE.
 * @param acs_url       location for URL.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_acs_url(cpe_id_t *cpe,
                                char *acs_url, size_t bufsize);

/**
 * Set URL of ACS on CPE.
 *
 * @param cpe           ID of CPE.
 * @param acs_url       string with URL.
 *
 * @return status
 */
extern te_errno cpe_set_acs_url(cpe_id_t *cpe, char *acs_url);

/**
 * Activate TR069 management protocol on CPE. 
 * Set URL of ACS on CPE, if specified.
 *
 * @param cpe           ID of CPE.
 * @param acs_url       string with URL, may be NULL.
 *
 * @return status
 */
extern te_errno cpe_activate_tr069_mgmt(cpe_id_t *cpe, char *acs_url);

/**
 * Dectivate TR069 management protocol on CPE. 
 *
 * @param cpe           ID of CPE.
 *
 * @return status
 */
extern te_errno cpe_deactivate_tr069_mgmt(cpe_id_t *cpe);

/**
 * Set ConnectionRequest login username on CPE, that is, login for
 * authenticate ConnectionRequest from ACS to CPE.
 *
 * @param cpe           ID of CPE.
 * @param cr_login      username.
 *
 * @return status
 */
extern te_errno cpe_set_cr_login(cpe_id_t *cpe,  const char *cr_login);

/**
 * Set ConnectionRequest login password on CPE.
 *
 * @param cpe           ID of CPE.
 * @param cr_passwd     password.
 *
 * @return status
 */
extern te_errno cpe_set_cr_passwd(cpe_id_t *cpe, const char *cr_passwd);

/**
 * Get ConnectionRequest login username on CPE.
 *
 * @param cpe           ID of CPE.
 * @param cr_login      location for username.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_cr_login(cpe_id_t *cpe,
                                 char *cr_login, size_t bufsize);

/**
 * Get ConnectionRequest login password on CPE.
 *
 * @param cpe           ID of CPE.
 * @param cr_passwd     location for password.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_cr_passwd(cpe_id_t *cpe,
                                  char *cr_passwd, size_t bufsize);

/**
 * Set ASC login username on CPE, that is login for authenticate
 * CPE while starting CWMP session (CPE to ACS).
 *
 * @param cpe           ID of CPE.
 * @param acs_login     username.
 *
 * @return status
 */
extern te_errno cpe_set_acs_login(cpe_id_t *cpe,  const char *acs_login);

/**
 * Set ACS password on CPE for CWMP sessions.
 *
 * @param cpe           ID of CPE.
 * @param acs_passwd    password.
 *
 * @return status
 */
extern te_errno cpe_set_acs_passwd(cpe_id_t *cpe, const char *acs_passwd);

/**
 * Get ACS login username on CPE.
 *
 * @param cpe           ID of CPE.
 * @param acs_login     location for username.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_acs_login(cpe_id_t *cpe,
                                  char *acs_login, size_t bufsize);

/**
 * Get ACS login password on CPE.
 *
 * @param cpe           ID of CPE.
 * @param acs_passwd    location for password.
 * @param bufsize       size of location buffer.
 *
 * @return status
 */
extern te_errno cpe_get_acs_passwd(cpe_id_t *cpe,
                                   char *acs_passwd, size_t bufsize);


/**
 * For standalone TR client tests respawn client, if necessary.
 * Do nothing for TR client on the real CPE. 
 *
 * @param cpe           ID of CPE.
 * @param acs_url       string with URL, may be NULL.
 *
 * @return status
 */
extern te_errno cpe_respawn_client(cpe_id_t *cpe, const char *acs_url);

/**
 * Check CPE is really just reboot. 
 * TODO: insert some timespec, that it is reboot during particular 
 * time interval?
 *
 * This routine do nothing for local TR client, because there is no
 * any real CPE under it. 
 *
 * @param cpe           ID of CPE.
 *
 * @return zero on success (i.e. check processed without problems and 
 *             box has just reboot) or error code.
 */ 
extern te_errno cpe_check_reboot(cpe_id_t *cpe);

/**
 * Check CPE is really just reboot (?) and ѕettings was dropped to 
 * the factory default. 
 * TODO: insert some timespec, that it is reboot during particular 
 * time interval?
 *
 * This routine do nothing for local TR client, because there is no
 * any real CPE under it. 
 *
 * @param cpe           ID of CPE.
 *
 * @return zero on success (i.e. check processed without problems and 
 *             box has factory default settings) or error code.
 */ 
extern te_errno cpe_check_factory_reset(cpe_id_t *cpe);


/**
 * Get directory path on the client TA controlling host where client
 * will put downloaded files. 
 * Return error if not applicable for current CPE.
 *
 * @param cpe           ID of CPE.
 * @param fs_path       Path to download directory (OUT).
 * @param bufsize       Size of location buffer.
 * 
 * @return status code.
 */
extern te_errno cpe_get_fs_path(cpe_id_t *cpe,
                                char *fs_path, size_t bufsize);

#endif /* __CPE_BACKDOOR__H__ */ 
