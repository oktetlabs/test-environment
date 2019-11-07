/** @file
 * @brief nginx Test Suite
 *
 * Check nginx ssl settings can be configured.
 *
 * Copyright (C) 2019 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Marina Maslova <Marina.Maslova@oktetlabs.ru>
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/* Logging subsystem entity name */
#define TE_TEST_NAME  "ssl"

#include "te_config.h"

#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_file.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_nginx.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"

#define NGINX_NAME      "webserver"
#define DFLT_NAME       "1"
#define ADDR_SPEC       "localhost:8443"

#define CERT_PATH       "/tmp/nginx_selfsigned.crt"
#define KEY_PATH        "/tmp/nginx_selfsigned.key"
#define CIPHERS         "ALL:!aNULL:RC4+RSA:+HIGH:+MEDIUM:+LOW:+SSLv2:+EXP"
#define PROTOCOLS       "TLSv1 TLSv1.1 TLSv1.2"

#define OPENSSL_CMD     "openssl req -x509 -nodes -newkey rsa:2048 " \
                        "-subj=\"/C=RU/ST=SPb/L=SPb/O=OKTET Labs/OU=IT" \
                        "/CN=localhost/emailAddress=marsik@oktetlabs.ru\" " \
                        "-keyout " KEY_PATH " -out " CERT_PATH

#define HTTPS_GET_CMD    "curl -k https://" ADDR_SPEC

int
main(int argc, char **argv)
{
    const char         *ta = "Agt_A";
    tarpc_pid_t         pid;
    rpc_wait_status     status;
    rcf_rpc_server     *pco = NULL;

    TEST_START;

    TEST_GET_RPCS(ta, "pco", pco);

    TEST_STEP("Prepare self-signed certificate");
    pid = rpc_te_shell_cmd(pco, "%s", -1, NULL, NULL, NULL, OPENSSL_CMD);

    RPC_AWAIT_IUT_ERROR(pco);
    if (rpc_waitpid(pco, pid, &status, 0) != pid)
        TEST_FAIL("Failed to execute '%s' command on agent '%s'",
                  OPENSSL_CMD, ta);
    else if (status.value != 0)
        TEST_FAIL("Command '%s' failed on agent '%s'", OPENSSL_CMD, ta);

    TEST_STEP("Add nginx instance");
    CHECK_RC(tapi_cfg_nginx_add(ta, NGINX_NAME));

    TEST_STEP("Add SSL settings");
    CHECK_RC(tapi_cfg_nginx_ssl_add(ta, NGINX_NAME, DFLT_NAME));
    CHECK_RC(tapi_cfg_nginx_ssl_cert_set(ta, NGINX_NAME, DFLT_NAME,
                                         CERT_PATH));
    CHECK_RC(tapi_cfg_nginx_ssl_key_set(ta, NGINX_NAME, DFLT_NAME,
                                        KEY_PATH));
    CHECK_RC(tapi_cfg_nginx_ssl_ciphers_set(ta, NGINX_NAME, DFLT_NAME,
                                            CIPHERS));
    CHECK_RC(tapi_cfg_nginx_ssl_protocols_set(ta, NGINX_NAME, DFLT_NAME,
                                              PROTOCOLS));

    TEST_STEP("Add nginx HTTP server");
    CHECK_RC(tapi_cfg_nginx_http_server_add(ta, NGINX_NAME, DFLT_NAME));
    CHECK_RC(tapi_cfg_nginx_http_server_ssl_name_set(ta, NGINX_NAME,
                                                     DFLT_NAME, DFLT_NAME));

    TEST_STEP("Add nginx listening entry");
    CHECK_RC(tapi_cfg_nginx_http_listen_entry_add(ta, NGINX_NAME, DFLT_NAME,
                                                  DFLT_NAME, ADDR_SPEC));
    CHECK_RC(tapi_cfg_nginx_http_listen_entry_ssl_enable(ta, NGINX_NAME,
                                                         DFLT_NAME,
                                                         DFLT_NAME));
    TEST_STEP("Start nginx");
    CHECK_RC(tapi_cfg_nginx_enable(ta, NGINX_NAME));

    TEST_STEP("Check that nginx replies on HTTPS GET");
    pid = rpc_te_shell_cmd(pco, HTTPS_GET_CMD, -1, NULL, NULL, NULL);

    RPC_AWAIT_IUT_ERROR(pco);
    if (rpc_waitpid(pco, pid, &status, 0) != pid)
        TEST_FAIL("Failed to execute '%s' command on agent '%s'",
                  HTTPS_GET_CMD, ta);
    else if (status.value != 0)
        TEST_FAIL("Command '%s' failed on agent '%s'", HTTPS_GET_CMD, ta);

    TEST_STEP("Stop nginx");
    CHECK_RC(tapi_cfg_nginx_disable(ta, NGINX_NAME));

    TEST_SUCCESS;

cleanup:
    tapi_file_ta_unlink_fmt(ta, "%s", CERT_PATH);
    tapi_file_ta_unlink_fmt(ta, "%s", KEY_PATH);
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
