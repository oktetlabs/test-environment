/** @file
 * @brief Test API to manage agent keys
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Keys TAPI"

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_cfg_key.h"
#include "tapi_cfg_base.h"
#include "tapi_file.h"
#include "te_enum.h"

te_bool
tapi_cfg_key_exists(const char *ta, const char *key_name)
{
    return cfg_find_fmt(NULL, "/agent:%s/key:%s", ta, key_name) == 0;
}

static te_bool
check_key_params(const char *ta, const char *key_name,
                 const char *exp_manager, const char *exp_type,
                 unsigned exp_bit_size)
{
    char *result;
    int val;
    te_errno rc;

    rc = cfg_get_instance_string_fmt(&result, "/agent:%s/key:%s",
                                     ta, key_name);
    if (rc != 0)
    {
        ERROR("Cannot get key manager: %r", rc);
        return FALSE;
    }

    if (strcmp(result, exp_manager) != 0)
    {
        RING("Key '%s' on '%s': expected manager '%s', got '%s'",
             key_name, ta, exp_manager, result);
        free(result);
        return FALSE;
    }
    free(result);

    rc = cfg_get_instance_string_fmt(&result, "/agent:%s/key:%s/type:",
                                     ta, key_name);
    if (rc != 0)
    {
        ERROR("Cannot get key type: %r", rc);
        return FALSE;
    }

    if (strcmp(result, exp_type) != 0)
    {
        RING("Key '%s' on '%s': expected type '%s', got '%s'",
             key_name, ta, exp_type, result);
        free(result);
        return FALSE;
    }
    free(result);

    rc = cfg_get_instance_int_fmt(&val, "/agent:%s/key:%s/bitsize:",
                                  ta, key_name);
    if (rc != 0)
    {
        ERROR("Cannot get key bit size: %r", rc);
        return FALSE;
    }

    if ((unsigned)val != exp_bit_size)
    {
        RING("Key '%s' on '%s': expected bit size %u, got %d",
             key_name, ta, exp_bit_size, val);
        return FALSE;
    }

    return TRUE;
}

te_errno
tapi_cfg_key_add(const char *ta, const char *key_name,
                 tapi_cfg_key_manager manager, tapi_cfg_key_type type,
                 tapi_cfg_key_size size, tapi_cfg_key_mode mode)
{
    static const te_enum_map key_managers[] =
        {
            {.name = "ssh", .value = TAPI_CFG_KEY_MANAGER_SSH},
            TE_ENUM_MAP_END
        };
    static const te_enum_map key_types[] =
        {
            {.name = "rsa", .value = TAPI_CFG_KEY_TYPE_SSH_RSA},
            {.name = "dsa", .value = TAPI_CFG_KEY_TYPE_SSH_DSA},
            {.name = "ecdsa", .value = TAPI_CFG_KEY_TYPE_SSH_ECDSA},
            {.name = "ed25519", .value = TAPI_CFG_KEY_TYPE_SSH_ED25519},
            TE_ENUM_MAP_END
        };
    static const unsigned key_sizes[][TAPI_CFG_KEY_SIZE_MAX + 1] =
        {
            [TAPI_CFG_KEY_TYPE_SSH_RSA] =
            {
                [TAPI_CFG_KEY_SIZE_MIN] = 1024,
                [TAPI_CFG_KEY_SIZE_RECOMMENDED] = 3072,
                [TAPI_CFG_KEY_SIZE_MAX] = 4096,
            },
            [TAPI_CFG_KEY_TYPE_SSH_DSA] =
            {
                [TAPI_CFG_KEY_SIZE_MIN] = 1024,
                [TAPI_CFG_KEY_SIZE_RECOMMENDED] = 1024,
                [TAPI_CFG_KEY_SIZE_MAX] = 1024,
            },
            [TAPI_CFG_KEY_TYPE_SSH_ECDSA] =
            {
                [TAPI_CFG_KEY_SIZE_MIN] = 256,
                [TAPI_CFG_KEY_SIZE_RECOMMENDED] = 384,
                [TAPI_CFG_KEY_SIZE_MAX] = 521,
            },
            [TAPI_CFG_KEY_TYPE_SSH_ED25519] =
            {
                [TAPI_CFG_KEY_SIZE_MIN] = 1,
                [TAPI_CFG_KEY_SIZE_RECOMMENDED] = 1,
                [TAPI_CFG_KEY_SIZE_MAX] = 1,
            },
        };
    bool existing = tapi_cfg_key_exists(ta, key_name);
    const char *manager_str = te_enum_map_from_value(key_managers, manager);
    const char *type_str = te_enum_map_from_value(key_types, type);
    unsigned bit_size = key_sizes[type][size];
    te_errno rc = 0;

    if (!existing)
    {
        rc = cfg_add_instance_local_fmt(NULL, CFG_VAL(STRING, manager_str),
                                        "/agent:%s/key:%s", ta, key_name);
        if (rc != 0)
            return rc;
    }
    else
    {
        switch (mode)
        {
            case TAPI_CFG_KEY_MODE_NEW:
                ERROR("Key '%s' already exists on '%s'", key_name, ta);
                return TE_RC(TE_TAPI, TE_EEXIST);

            case TAPI_CFG_KEY_MODE_REUSE:
            {
                if (!check_key_params(ta, key_name, manager_str, type_str,
                                      bit_size))
                {
                    ERROR("Cannot reuse key '%s' on '%s' with "
                          "different parameters", key_name, ta);
                    return TE_RC(TE_TAPI, TE_EBADSLT);
                }
                RING("Reusing existing key '%s' on '%s'", key_name, ta);
                return 0;
            }

            case TAPI_CFG_KEY_MODE_REPLACE:
                RING("Regenerating key '%s' on '%s' with type %s and size %u",
                     key_name, ta, type_str, bit_size);
                break;
        }
    }

    rc = cfg_set_instance_local_fmt(CFG_VAL(STRING, type_str),
                                    "/agent:%s/key:%s/type:", ta, key_name);
    if (rc != 0)
        goto fail;

    rc = cfg_set_instance_local_fmt(CFG_VAL(INTEGER, bit_size),
                                    "/agent:%s/key:%s/bitsize:", ta, key_name);
    if (rc != 0)
        goto fail;

    rc = cfg_commit_fmt("/agent:%s/key:%s", ta, key_name);
    if (rc != 0)
        goto fail;

    return 0;

fail:
    if (!existing)
        cfg_del_instance_local_fmt(FALSE, "/agent:%s/key:%s", ta, key_name);
    return rc;
}

unsigned
tapi_cfg_key_get_bitsize(const char *ta, const char *key_name)
{
    int val;
    te_errno rc;

    rc = cfg_get_instance_int_sync_fmt(&val, "/agent:%s/key:%s/bitsize:",
                                       ta, key_name);
    if (rc != 0)
    {
        ERROR("Cannot determine the key size of '%s' on '%s': %r",
              key_name, ta, rc);
        return 0;
    }

    return (unsigned)val;
}

char *
tapi_cfg_key_get_private_key_path(const char *ta, const char *key_name)
{
    char *result;
    te_errno rc;

    rc = cfg_get_instance_string_sync_fmt(&result,
                                          "/agent:%s/key:%s/private_file:",
                                          ta, key_name);
    if (rc != 0)
    {
        ERROR("Cannot determine the private key path of '%s' on '%s': %r",
              key_name, ta, rc);
        return NULL;
    }

    return result;
}

char *
tapi_cfg_key_get_public_key(const char *ta, const char *key_name)
{
    char *result;
    te_errno rc;

    rc = cfg_get_instance_string_sync_fmt(&result,
                                          "/agent:%s/key:%s/public:",
                                          ta, key_name);
    if (rc != 0)
    {
        ERROR("Cannot determine the public key of '%s' on '%s': %r",
              key_name, ta, rc);
        return NULL;
    }

    return result;
}

te_errno
tapi_cfg_key_del(const char *ta, const char *key_name)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/key:%s", ta, key_name);
}

te_errno
tapi_cfg_key_append_public(const char *ta, const char *key_name,
                           const char *dst_ta, const char *list_name)
{
    te_string full_path = TE_STRING_INIT;
    const char *full_list_name;
    char *public_key = tapi_cfg_key_get_public_key(ta, key_name);
    te_errno rc = 0;

    if (public_key == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_EBADSLT);
        goto cleanup;
    }

    if (*list_name == '/')
        full_list_name = list_name;
    else
    {
        char *tmp_dir = tapi_cfg_base_get_ta_dir(dst_ta, TAPI_CFG_BASE_TA_DIR_TMP);

        if (tmp_dir == NULL)
        {
            rc = TE_RC(TE_TAPI, TE_ENOCONF);
            goto cleanup;
        }
        rc = te_string_append(&full_path, "%s/%s", tmp_dir, list_name);
        free(tmp_dir);
        if (rc != 0)
            goto cleanup;
        full_list_name = full_path.ptr;
    }

    rc = tapi_file_append_ta(dst_ta, full_list_name, "%s\n", public_key);

cleanup:
    te_string_free(&full_path);
    free(public_key);

    return rc;
}
