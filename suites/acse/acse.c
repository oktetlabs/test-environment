/** @file
 * @brief ACSE Test Suite
 *
 * ACSE Test Suite
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 * 
 * $Id: acse.c $
 */


#define TE_TEST_NAME "acse/acse"

#include "acse_suite.h"

int
main(int argc, char *argv[])
{
    unsigned int u;

    rcf_rpc_server *pco_iut = NULL;
    rcf_rpc_server *pco_aux = NULL;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_aux);

    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/acse:", pco_iut->ta));
    CHECK_RC(tapi_cfg_acse_start(pco_iut->ta));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/acse:", pco_iut->ta));

    for (u = 0; u < 2; u++)
    {
        char         buf[6][64];
        unsigned int uu;

        TE_SPRINTF(buf[0], "acs_%u", u);
        TE_SPRINTF(buf[2], "http://oktetlabs.ru/%u", u);
        TE_SPRINTF(buf[3], "Certificate: %u", u);
        TE_SPRINTF(buf[4], "User_%u", u);
        TE_SPRINTF(buf[5], "Pass_%u", u);
        CHECK_RC(tapi_cfg_acse_add_acs_with_params(pco_iut->ta, buf[0], buf[2], buf[3], buf[4], buf[5], u != 0 ? TRUE : FALSE, u != 0 ? 443 : 80));

        {
            char const *str = NULL;
            int         i;
            te_bool     b;

            CHECK_RC(tapi_cfg_acse_get_acs_url(pco_iut->ta, buf[0], &str));
            RING("URL: '%s'", str);
            free((void *)str);
            CHECK_RC(tapi_cfg_acse_get_acs_cert(pco_iut->ta, buf[0], &str));
            RING("CERT: '%s'", str);
            free((void *)str);
            CHECK_RC(tapi_cfg_acse_get_acs_user(pco_iut->ta, buf[0], &str));
            RING("USER: '%s'", str);
            free((void *)str);
            CHECK_RC(tapi_cfg_acse_get_acs_pass(pco_iut->ta, buf[0], &str));
            RING("PASS: '%s'", str);
            free((void *)str);
            CHECK_RC(tapi_cfg_acse_get_acs_enabled(pco_iut->ta, buf[0], &b));
            RING("ENABLED: '%d'", b);
            CHECK_RC(tapi_cfg_acse_get_acs_ssl(pco_iut->ta, buf[0], &b));
            RING("SSL: '%d'", b);
            CHECK_RC(tapi_cfg_acse_get_acs_port(pco_iut->ta, buf[0], &i));
            RING("PORT: '%d'", i);
        }

        for (uu = 0; uu < 2; uu++)
        {
            struct sockaddr addr = { .sa_family = AF_INET };

            TE_SPRINTF(buf[1], "cpe_%u", uu);
            TE_SPRINTF(buf[2], "http://oktetlabs.ru/%u/%u", u, uu);
            TE_SPRINTF(buf[3], "Certificate: %u/%u", u, uu);
            TE_SPRINTF(buf[4], "User_%u_%u", u, uu);
            TE_SPRINTF(buf[5], "Pass_%u_%u", u, uu);
            SIN(&addr)->sin_addr.s_addr = htonl((192 << 24) | (168 << 16) | (u << 8) | uu);
            CHECK_RC(tapi_cfg_acse_add_cpe_with_params(pco_iut->ta, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], &addr));
            CHECK_RC(tapi_cfg_acse_set_session_enabled(pco_iut->ta, buf[0], buf[1], (u + uu) % 2));
            CHECK_RC(tapi_cfg_acse_set_session_hold_requests(pco_iut->ta, buf[0], buf[1], 1 - (u + uu) % 2));
            CHECK_RC(tapi_cfg_acse_set_session_target_state(pco_iut->ta, buf[0], buf[1], u + u + uu));

            {
                char const            *str  = NULL;
                struct sockaddr const *addr = NULL;
                te_bool                b;
                session_state_t        s;

                CHECK_RC(tapi_cfg_acse_get_cpe_url(pco_iut->ta, buf[0], buf[1], &str));
                RING("URL: '%s'", str);
                free((void *)str);
                CHECK_RC(tapi_cfg_acse_get_cpe_cert(pco_iut->ta, buf[0], buf[1], &str));
                RING("CERT: '%s'", str);
                free((void *)str);
                CHECK_RC(tapi_cfg_acse_get_cpe_user(pco_iut->ta, buf[0], buf[1], &str));
                RING("USER: '%s'", str);
                free((void *)str);
                CHECK_RC(tapi_cfg_acse_get_cpe_pass(pco_iut->ta, buf[0], buf[1], &str));
                RING("PASS: '%s'", str);
                free((void *)str);
                CHECK_RC(tapi_cfg_acse_get_cpe_ip_addr(pco_iut->ta, buf[0], buf[1], &addr));
                RING("IP_ADDR: '%s'", inet_ntoa(SIN(addr)->sin_addr));
                free((void *)addr);
                CHECK_RC(tapi_cfg_acse_get_session_enabled(pco_iut->ta, buf[0], buf[1], &b));
                RING("ENABLED: %d", b);
                CHECK_RC(tapi_cfg_acse_get_session_hold_requests(pco_iut->ta, buf[0], buf[1], &b));
                RING("HOLD_REQUESTS: %d", b);
                CHECK_RC(tapi_cfg_acse_get_session_target_state(pco_iut->ta, buf[0], buf[1], &s));
                RING("TARGET_STATE: %d", s);
                CHECK_RC(tapi_cfg_acse_get_session_state(pco_iut->ta, buf[0], buf[1], &s));
                RING("STATE: %d", s);
                CHECK_RC(tapi_cfg_acse_get_device_id_manufacturer(pco_iut->ta, buf[0], buf[1], &str));
                RING("MANUFACTURER: '%s'", str);
                free((void *)str);
                CHECK_RC(tapi_cfg_acse_get_device_id_oui(pco_iut->ta, buf[0], buf[1], &str));
                RING("OUI: '%s'", str);
                free((void *)str);
                CHECK_RC(tapi_cfg_acse_get_device_id_product_class(pco_iut->ta, buf[0], buf[1], &str));
                RING("PRODUCT_CLASS: '%s'", str);
                free((void *)str);
                CHECK_RC(tapi_cfg_acse_get_device_id_serial_number(pco_iut->ta, buf[0], buf[1], &str));
                RING("SERIAL_NUMBER: '%s'", str);
                free((void *)str);
            }

        }

        CHECK_RC(tapi_cfg_acse_set_acs_enabled(pco_iut->ta, buf[0], TRUE));
    }

    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/acse:", pco_iut->ta));

    for(u = 0; u < 2; u++)
    {
        char buf[2][64];
        unsigned int uu;

        TE_SPRINTF(buf[0], "acs_%u", u);

        for (uu = 0; uu < 2; uu++)
        {
            TE_SPRINTF(buf[1], "cpe_%u", uu);
            tapi_cfg_acse_del_cpe(pco_iut->ta, buf[0], buf[1]);
        }

        CHECK_RC(tapi_cfg_acse_set_acs_enabled(pco_iut->ta, buf[0], FALSE));
        tapi_cfg_acse_del_acs(pco_iut->ta, buf[0]);
    }

    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/acse:", pco_iut->ta));
    CHECK_RC(tapi_cfg_acse_stop(pco_iut->ta));
    CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, "/agent:%s/acse:", pco_iut->ta));

    CHECK_RC(tapi_cfg_acse_start(pco_iut->ta));

    RPC_AWAIT_IUT_ERROR(pco_iut);
    {
        cpe_get_rpc_methods_response_t resp = { .method_list = NULL, .n = 0 };

        int rc = rpc_cpe_get_rpc_methods(pco_iut, &resp);

        if (rc == 0)
        {
            for (u = 0; u < resp.n; u++)
            {
                RING("method_list[%u] = '%s'", u, resp.method_list[u]);
            }
        }
        else
            RING("rc = %d, %r", rc, rc);
    }

#define DO_CALL(_fun) \
    RPC_AWAIT_IUT_ERROR(pco_iut);                                    \
    {                                                                \
        _fun##_t req; \
        _fun##_response_t resp;  \
                                                   \
        rpc_##_fun(pco_iut, &req, &resp); \
    }

    DO_CALL(cpe_set_parameter_values)
    DO_CALL(cpe_get_parameter_values)
    DO_CALL(cpe_get_parameter_names)
    DO_CALL(cpe_set_parameter_attributes)
    DO_CALL(cpe_get_parameter_attributes)
    DO_CALL(cpe_add_object)
    DO_CALL(cpe_delete_object)
    DO_CALL(cpe_reboot)
    DO_CALL(cpe_download)
    DO_CALL(cpe_upload)
    DO_CALL(cpe_factory_reset)
    DO_CALL(cpe_get_queued_transfers)
    DO_CALL(cpe_get_all_queued_transfers)
    DO_CALL(cpe_schedule_inform)
    DO_CALL(cpe_set_vouchers)
    DO_CALL(cpe_get_options)

    CHECK_RC(tapi_cfg_acse_stop(pco_iut->ta));

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
