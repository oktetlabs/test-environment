/** @file
 * @brief Test API to configure ACSE
 *
 * Implementation of API to configure ACSE.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id:
 */

#define TE_LGR_USER     "TAPI CFG ACSE"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>

/* Ensure PATH_MAX is defined */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <limits.h>

/* Nevertheless PATH_MAX can still be undefined here yet */
#ifndef PATH_MAX
#define PATH_MAX 108
#endif

#endif /* STDC_HEADERS */

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_sockaddr.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_acse.h"

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_start(char const *ta)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                "/agent:%s/acse:", ta);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_stop(char const *ta)
{
    te_errno rc;
    char     buf[256];

    TE_SPRINTF(buf, "/agent:%s/acse:", ta);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0), "%s", buf)) == 0)
        return cfg_synchronize(buf, TRUE);

    return rc;
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_add_acs(char const *ta, char const *acs)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(NONE, 0),
                                "/agent:%s/acse:/acs:%s", ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_acs_url(char const *ta, char const *acs,
                          char const *url)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, url),
                                "/agent:%s/acse:/acs:%s/url:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_acs_url(char const *ta, char const *acs,
                          char const **url)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, url,
                                "/agent:%s/acse:/acs:%s/url:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_acs_cert(char const *ta, char const *acs,
                           char const *cert)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, cert),
                                "/agent:%s/acse:/acs:%s/cert:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_acs_cert(char const *ta, char const *acs,
                           char const **cert)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, cert,
                                "/agent:%s/acse:/acs:%s/cert:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_acs_user(char const *ta, char const *acs,
                           char const *user)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, user),
                                "/agent:%s/acse:/acs:%s/user:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_acs_user(char const *ta, char const *acs,
                           char const **user)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, user,
                                "/agent:%s/acse:/acs:%s/user:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_acs_pass(char const *ta, char const *acs,
                           char const *pass)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, pass),
                                "/agent:%s/acse:/acs:%s/pass:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_acs_pass(char const *ta, char const *acs,
                           char const **pass)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, pass,
                                "/agent:%s/acse:/acs:%s/pass:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_acs_enabled(char const *ta, char const *acs,
                              te_bool enabled)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, enabled ? 1 : 0),
                                "/agent:%s/acse:/acs:%s/enabled:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_acs_enabled(char const *ta, char const *acs,
                              te_bool *enabled)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, enabled,
                                "/agent:%s/acse:/acs:%s/enabled:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_acs_ssl(char const *ta, char const *acs,
                          te_bool ssl)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, ssl ? 1 : 0),
                                "/agent:%s/acse:/acs:%s/ssl:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_acs_ssl(char const *ta, char const *acs,
                          te_bool *ssl)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, ssl,
                                "/agent:%s/acse:/acs:%s/ssl:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_acs_port(char const *ta, char const *acs,
                           int port)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, port),
                                "/agent:%s/acse:/acs:%s/port:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_acs_port(char const *ta, char const *acs,
                           int *port)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, port,
                                "/agent:%s/acse:/acs:%s/port:",
                                ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_add_acs_with_params(char const *ta, char const *acs,
                                  char const *url, char const *cert,
                                  char const *user, char const *pass,
                                  te_bool ssl, int port)
{
    te_errno rc;

    if ((rc =      tapi_cfg_acse_add_acs(ta, acs))       == 0 &&
        (rc =  tapi_cfg_acse_set_acs_url(ta, acs, url))  == 0 &&
        (rc = tapi_cfg_acse_set_acs_cert(ta, acs, cert)) == 0 &&
        (rc = tapi_cfg_acse_set_acs_user(ta, acs, user)) == 0 &&
        (rc = tapi_cfg_acse_set_acs_pass(ta, acs, pass)) == 0 &&
        (rc = tapi_cfg_acse_set_acs_ssl(ta, acs, ssl))  == 0)
    {
        return tapi_cfg_acse_set_acs_port(ta, acs, port);
    }

    return rc;
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_del_acs(char const *ta, char const *acs)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/acse:/acs:%s", ta, acs);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_add_cpe(char const *ta, char const *acs, char const *cpe)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(NONE, 0),
                                "/agent:%s/acse:/acs:%s/cpe:%s",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_cpe_url(char const *ta, char const *acs,
                          char const *cpe, char const *url)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, url),
                                "/agent:%s/acse:/acs:%s/cpe:%s/url:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_cpe_url(char const *ta, char const *acs,
                          char const *cpe, char const **url)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, url,
                                "/agent:%s/acse:/acs:%s/cpe:%s/url:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_cpe_cert(char const *ta, char const *acs,
                           char const *cpe, char const *cert)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, cert),
                                "/agent:%s/acse:/acs:%s/cpe:%s/cert:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_cpe_cert(char const *ta, char const *acs,
                           char const *cpe, char const **cert)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, cert,
                                "/agent:%s/acse:/acs:%s/cpe:%s/cert:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_cpe_user(char const *ta, char const *acs,
                           char const *cpe, char const *user)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, user),
                                "/agent:%s/acse:/acs:%s/cpe:%s/user:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_cpe_user(char const *ta, char const *acs,
                           char const *cpe, char const **user)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, user,
                                "/agent:%s/acse:/acs:%s/cpe:%s/user:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_cpe_pass(char const *ta, char const *acs,
                           char const *cpe, char const *pass)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, pass),
                                "/agent:%s/acse:/acs:%s/cpe:%s/pass:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_cpe_pass(char const *ta, char const *acs,
                           char const *cpe, char const **pass)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, pass,
                                "/agent:%s/acse:/acs:%s/cpe:%s/pass:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_cpe_ip_addr(char const *ta, char const *acs,
                              char const *cpe,
                              struct sockaddr const *addr)
{
    return cfg_set_instance_fmt(CFG_VAL(ADDRESS, addr),
                                "/agent:%s/acse:/acs:%s/cpe:%s/ip_addr:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_cpe_ip_addr(char const *ta, char const *acs,
                              char const *cpe,
                              struct sockaddr const **addr)
{
    cfg_val_type type = CVT_ADDRESS;

    return cfg_get_instance_fmt(&type, addr,
                                "/agent:%s/acse:/acs:%s/cpe:%s/ip_addr:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_session_enabled(char const *ta, char const *acs,
                                  char const *cpe, te_bool enabled)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, enabled ? 1 : 0),
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "session:/enabled:", ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_session_enabled(char const *ta, char const *acs,
                                  char const *cpe, te_bool *enabled)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, enabled,
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "session:/enabled:", ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_session_hold_requests(char const *ta, char const *acs,
                                        char const *cpe,
                                        te_bool hold_requests)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, hold_requests ? 1 : 0),
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "session:/hold_requests:", ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_session_hold_requests(char const *ta, char const *acs,
                                        char const *cpe,
                                        te_bool *hold_requests)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, hold_requests,
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "session:/hold_requests:", ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_set_session_target_state(char const *ta, char const *acs,
                                       char const *cpe,
                                       session_state_t target_state)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, target_state),
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "session:/target_state:", ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_session_target_state(char const *ta, char const *acs,
                                       char const *cpe,
                                       session_state_t *target_state)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, target_state,
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "session:/target_state:", ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_session_state(char const *ta, char const *acs,
                                char const *cpe,
                                session_state_t *state)
{
    cfg_val_type type = CVT_INTEGER;

    return cfg_get_instance_fmt(&type, state,
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "session:/state:", ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_device_id_manufacturer(char const *ta, char const *acs,
                                         char const *cpe,
                                         char const **manufacturer)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, manufacturer,
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "device_id:/manufacturer:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_device_id_oui(char const *ta, char const *acs,
                                char const *cpe,
                                char const **oui)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, oui,
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "device_id:/oui:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_device_id_product_class(char const *ta, char const *acs,
                                          char const *cpe,
                                          char const **product_class)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, product_class,
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "device_id:/product_class:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_device_id_serial_number(char const *ta, char const *acs,
                                          char const *cpe,
                                          char const **serial_number)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance_fmt(&type, serial_number,
                                "/agent:%s/acse:/acs:%s/cpe:%s/"
                                "device_id:/serial_number:",
                                ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_add_cpe_with_params(char const *ta, char const *acs,
                                  char const *cpe,
                                  char const *url, char const *cert,
                                  char const *user, char const *pass,
                                  struct sockaddr const *addr)
{
    te_errno rc;

    if ((rc =      tapi_cfg_acse_add_cpe(ta, acs, cpe))       == 0 &&
        (rc =  tapi_cfg_acse_set_cpe_url(ta, acs, cpe, url))  == 0 &&
        (rc =  tapi_cfg_acse_set_cpe_url(ta, acs, cpe, url))  == 0 &&
        (rc = tapi_cfg_acse_set_cpe_cert(ta, acs, cpe, cert)) == 0 &&
        (rc = tapi_cfg_acse_set_cpe_user(ta, acs, cpe, user)) == 0 &&
        (rc = tapi_cfg_acse_set_cpe_pass(ta, acs, cpe, pass)) == 0)
    {
        return tapi_cfg_acse_set_cpe_ip_addr(ta, acs, cpe, addr);
    }

    return rc;
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_del_cpe(char const *ta, char const *acs, char const *cpe)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/acse:/acs:%s/cpe:%s",
                                ta, acs, cpe);
}
