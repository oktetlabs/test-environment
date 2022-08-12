/** @file
 * @brief Substitution in NDN ASN.1 data units
 *
 * Implementation of API for environment and test parameters
 * substitution in ASN.1 data units
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#define TE_LGR_USER     "TAPI NDN SUBST"

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "te_sockaddr.h"
#include "tapi_sockaddr.h"
#include "tapi_ndn.h"
#include "asn_impl.h"
#include "ndn.h"
#ifdef HAVE_TAPI_ENV_H
#include "tapi_env.h"
#endif


#ifdef HAVE_TAPI_ENV_H

static te_errno
tapi_ndn_subst_tapi_env(asn_value *container, char *tapi_env_ref,
                        struct tapi_env *env)
{
    const struct sockaddr     *addr     = NULL;
    char                      *ptr      = NULL;
    socklen_t                  addrlen;
    te_errno                   rc;

    if (env == NULL || container == NULL)
    {
        ERROR("%s(): env and container have to be specified",
        __FUNCTION__);
        return TE_EINVAL;
    }

    if (strcmp_start("addr.", tapi_env_ref) != 0)
    {
        ERROR("Expected parameter 'addr', but it is %s", tapi_env_ref);
        return TE_EINVAL;
    }

    tapi_env_ref += strlen("addr.");
    ptr = strchr(tapi_env_ref, '.');

    if (ptr != NULL)
        *ptr = '\0';

    addr = tapi_env_get_addr(env, tapi_env_ref, &addrlen);
    if (addr == NULL)
    {
        ERROR("'%s' is not found in environment addresses",
              tapi_env_ref);
        return TE_EINVAL;
    }

    if (ptr == NULL)
    {
        rc = asn_write_value_field(container, te_sockaddr_get_netaddr(addr),
                                   te_netaddr_get_size(addr->sa_family),
                                   "#plain");
        if (rc != 0)
        {
            ERROR("rc from asn write %r\n", rc);
            return rc;
        }
    }
    else if (strcmp(ptr + 1, "port") == 0)
    {
        *ptr = '.';

        if (strcmp(++ptr, "port") != 0)
        {
            ERROR("Expected parameter 'port', but it is %s", ptr);
            return TE_EINVAL;
        }

        if (ntohs(te_sockaddr_get_port(addr)) == 0)
        {
            rc = tapi_allocate_set_port(NULL, addr);
            if (rc != 0)
            {
                ERROR("Failed to allocate port for address: %r", rc);
                return rc;
            }
        }
        rc = asn_write_int32(container, ntohs(te_sockaddr_get_port(addr)),
                             "#plain");
        if (rc != 0)
        {
            ERROR("rc from asn write %r\n", rc);
            return rc;
        }
    }
    else
    {
        ERROR("%s(): Unexpected parameter: %s", __FUNCTION__, ptr);
        return TE_EINVAL;
    }

    return 0;
}

#else /* ndef HAVE_TAPI_ENV_H */

static te_errno
tapi_ndn_subst_tapi_env(asn_value *container, char *tapi_env_ref,
                        struct tapi_env *env)
{
    UNUSED(container);
    UNUSED(tapi_env_ref);
    UNUSED(env);

    ERROR("tapi_env support is compiled out");
    return TE_EINVAL;
}

#endif /* ndef HAVE_TAPI_ENV_H */

static te_errno
tapi_ndn_subst_test_param(asn_value *container, te_kvpair_h *params,
                          const char *name_str)
{
    asn_value       *plain      = NULL;
    const asn_type  *plain_type = NULL;
    const char      *value_str  = NULL;
    int              syms_parsed;
    te_errno         rc;

    if (container == NULL || params == NULL || name_str == NULL)
    {
        ERROR("%s(): params and container have to be specified",
              __FUNCTION__);
        return TE_EINVAL;
    }

    value_str = te_kvpairs_get(params, name_str);
    if (value_str == NULL)
    {
        ERROR("%s(): key '%s' not found", __FUNCTION__, name_str);
        return TE_EINVAL;
    }

    rc = asn_get_child_type(container->asn_type, &plain_type, PRIVATE,
                            NDN_DU_PLAIN);
    if (rc != 0)
        return rc;

    rc = asn_parse_value_text(value_str, plain_type, &plain, &syms_parsed);
    if ((rc != 0) || (syms_parsed != (int)strlen(value_str)))
        return ((rc != 0) ? rc : TE_EINVAL);

    rc = asn_put_child_value(container, plain, PRIVATE, NDN_DU_PLAIN);

    return rc;
}

/* See description in tapi_test.h */
te_errno
tapi_ndn_subst_env(asn_value *value, te_kvpair_h *params,
                   struct tapi_env *env)
{
    unsigned int    n_layers;
    unsigned int    i;
    char           *params_str = NULL;
    char           *tmp_params = NULL;
    te_errno        rc         = 0;

    if (value == NULL)
        return TE_EINVAL;

    n_layers = (unsigned int)value->len;

    for (i = 0; i < n_layers; i++)
    {
        asn_value     *cur_level        = NULL;
        asn_value     *data_unit_choice = NULL;
        asn_tag_value  tag_val;

        if (value->syntax == CHOICE)
            rc = asn_get_choice_value(value, &cur_level, NULL, NULL);
        else
            rc = asn_get_child_by_index(value, &cur_level, i);

        if (rc != 0)
            continue;

        if (strncmp(asn_get_type(cur_level)->name, "DATA-UNIT",
                                 strlen("DATA-UNIT")) == 0)
        {
            rc = asn_get_choice_value(cur_level, &data_unit_choice,
                                      NULL, &tag_val);
            if (rc != 0)
                return rc;

            if (tag_val == NDN_DU_ENV)
            {
                rc = asn_read_string(data_unit_choice, &params_str, "name");
                if (rc != 0)
                    return rc;

                tmp_params = params_str;

                if (strcmp_start("env.", tmp_params) == 0)
                {
                    tmp_params += strlen("env.");

                    rc = asn_free_child_value(cur_level, PRIVATE, NDN_DU_ENV);
                    if (rc != 0)
                        goto cleanup;

                    rc = tapi_ndn_subst_tapi_env(cur_level, tmp_params, env);
                    if (rc != 0)
                    {
                        ERROR("Failed to substitution of env params");
                        goto cleanup;
                    }
                }
                else if (strcmp_start("param.", tmp_params) == 0)
                {
                    tmp_params += strlen("param.");

                    rc = tapi_ndn_subst_test_param(cur_level, params,
                                                   tmp_params);
                    if (rc != 0)
                    {
                        ERROR("Failed to substitution of test params");
                        goto cleanup;
                    }
                }
                else
                {
                    rc = TE_EINVAL;
                    ERROR("Failed to substitution: Unexpected parameter: %s",
                          tmp_params);
                    goto cleanup;
                }
cleanup:
                free(params_str);

                if (rc != 0)
                    return rc;
            }

            continue;
        }

        rc = tapi_ndn_subst_env(cur_level, params, env);
        if (rc != 0)
        {
            ERROR("Failed to substitution with rc = %r", rc);
            return rc;
        }
    }

    return 0;
}
