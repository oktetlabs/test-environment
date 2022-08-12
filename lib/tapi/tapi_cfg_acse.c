/** @file
 * @brief Test API to configure ACSE
 *
 * Implementation of API to configure ACSE.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
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
#include "te_str.h"
#include "logger_api.h"
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
                          char **url)
{
    return cfg_get_instance_string_fmt(url, "/agent:%s/acse:/acs:%s/url:",
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
                           char **cert)
{
    return cfg_get_instance_string_fmt(cert, "/agent:%s/acse:/acs:%s/cert:",
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
                           char **user)
{
    return cfg_get_instance_string_fmt(user, "/agent:%s/acse:/acs:%s/user:",
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
                           char **pass)
{
    return cfg_get_instance_string_fmt(pass, "/agent:%s/acse:/acs:%s/pass:",
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
    te_errno rc;
    int enabled_int;

    rc = cfg_get_instance_int_fmt(&enabled_int,
                                  "/agent:%s/acse:/acs:%s/enabled:", ta, acs);
    if (rc == 0)
        *enabled = (enabled_int != 0);

    return rc;
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
    te_errno rc;
    int ssl_enabled;

    rc = cfg_get_instance_int_fmt(&ssl_enabled,
                                  "/agent:%s/acse:/acs:%s/ssl:", ta, acs);
    if (rc == 0)
        *ssl = (ssl_enabled != 0);

    return rc;
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
    return cfg_get_instance_int_fmt(port, "/agent:%s/acse:/acs:%s/port:",
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
                          char const *cpe, char **url)
{
    return cfg_get_instance_string_fmt(url,
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
                           char const *cpe, char **cert)
{
    return cfg_get_instance_string_fmt(cert,
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
                           char const *cpe, char **user)
{
    return cfg_get_instance_string_fmt(user,
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
                           char const *cpe, char **pass)
{
    return cfg_get_instance_string_fmt(pass,
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
                              struct sockaddr **addr)
{
    return cfg_get_instance_addr_fmt(addr,
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
    te_errno rc;
    int enabled_int;

    rc = cfg_get_instance_int_fmt(&enabled_int,
                                  "/agent:%s/acse:/acs:%s/cpe:%s/"
                                  "session:/enabled:", ta, acs, cpe);
    if (rc == 0)
        *enabled = (enabled_int != 0);

    return rc;
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
    te_errno rc;
    int hold_requests_int;

    rc = cfg_get_instance_int_fmt(&hold_requests_int,
                                  "/agent:%s/acse:/acs:%s/cpe:%s/"
                                  "session:/hold_requests:", ta, acs, cpe);
    if (rc == 0)
        *hold_requests = (hold_requests_int != 0);

    return rc;
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
    te_errno rc;
    int target_state_int;

    rc = cfg_get_instance_int_fmt(&target_state_int,
                                  "/agent:%s/acse:/acs:%s/cpe:%s/"
                                  "session:/target_state:", ta, acs, cpe);
    if (rc == 0)
    {
        /* Here we hope that Agent returned a value compatible with the enum */
        *target_state = target_state_int;
    }

    return rc;
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_session_state(char const *ta, char const *acs,
                                char const *cpe,
                                session_state_t *state)
{
    te_errno rc;
    int state_int;

    return cfg_get_instance_int_fmt(&state_int,
                                    "/agent:%s/acse:/acs:%s/cpe:%s/"
                                    "session:/state:", ta, acs, cpe);
    if (rc == 0)
    {
        /* Here we hope that Agent returned a value compatible with the enum */
        *state = state_int;
    }

    return rc;
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_device_id_manufacturer(char const *ta, char const *acs,
                                         char const *cpe,
                                         char **manufacturer)
{
    return cfg_get_instance_string_fmt(manufacturer,
                                       "/agent:%s/acse:/acs:%s/cpe:%s/"
                                       "device_id:/manufacturer:",
                                       ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_device_id_oui(char const *ta, char const *acs,
                                char const *cpe,
                                char **oui)
{
    return cfg_get_instance_string_fmt(oui,
                                       "/agent:%s/acse:/acs:%s/cpe:%s/"
                                       "device_id:/oui:",
                                       ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_device_id_product_class(char const *ta, char const *acs,
                                          char const *cpe,
                                          char **product_class)
{
    return cfg_get_instance_string_fmt(product_class,
                                       "/agent:%s/acse:/acs:%s/cpe:%s/"
                                       "device_id:/product_class:",
                                       ta, acs, cpe);
}

/* See description in tapi_cfg_acse.h */
extern te_errno
tapi_cfg_acse_get_device_id_serial_number(char const *ta, char const *acs,
                                          char const *cpe,
                                          char **serial_number)
{
    return cfg_get_instance_string_fmt(serial_number,
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
