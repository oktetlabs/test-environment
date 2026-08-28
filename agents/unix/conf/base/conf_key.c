/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Crypto keys support
 *
 * Crypto key configuration tree support
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Keys"

#include "te_config.h"
#include "config.h"

#include <limits.h>

#include "te_defs.h"

#include "te_errno.h"
#include "logger_api.h"
#include "te_str.h"
#include "te_alloc.h"
#include "rcf_pch_tree.h"
#include "unix_internal.h"
#include "te_vector.h"
#include "te_enum.h"
#include "te_file.h"

/** Generated key info */
typedef struct key_info {
    /** Name of the key */
    char *name;

    /** If @c true, the key should be regenerated on commit */
    bool need_generation;

    /** Key type */
    char *type;

    /** Key size */
    unsigned bitsize;

    /** Private key file name */
    char *private_file;

    /** Public key file name */
    char *public_file;
} key_info;

static te_vec known_keys = TE_VEC_INIT(key_info);

/*
 * So far this is a dummy mapping, since only one type of
 * key managers is supported
 */
static te_enum_map key_managers[] = {
    {.name = "ssh", .value = AGENT_KEY_MANAGER_SSH},
    TE_ENUM_MAP_END
};

static void
free_key_data(const key_info *key)
{
    free(key->name);
    free(key->type);

    if (key->private_file != NULL)
        remove(key->private_file);
    free(key->private_file);

    if (key->public_file != NULL)
        remove(key->public_file);
    free(key->public_file);
}

static key_info *
find_key(const char *id)
{
    key_info *iter;

    TE_VEC_FOREACH(&known_keys, iter)
    {
        if (strcmp(iter->name, id) == 0)
            return iter;
    }
    return NULL;
}

static te_errno
key_get(const ta_conf_ctx *ctx, te_string *val)
{
    const key_info *key = find_key(ta_conf_ctx_inst(ctx, "key"));

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%s",
                     te_enum_map_from_value(key_managers,
                                            AGENT_KEY_MANAGER_SSH));
    return 0;
}

/*
 * This function is intentionally a no-op, it only checks the correctness of
 * its argument. It exists only because objects with commit method do not
 * behave well if they have no set method as well
 */
static te_errno
key_set(const ta_conf_ctx *ctx, const char *val)
{
    const key_info *key = find_key(ta_conf_ctx_inst(ctx, "key"));

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (te_enum_map_from_str(key_managers, val, -1) != AGENT_KEY_MANAGER_SSH)
        return TE_RC(TE_TA_UNIX, TE_EPROTONOSUPPORT);

    return 0;
}

static te_errno
key_add(const ta_conf_ctx *ctx, const char *val)
{
    const char *id = ta_conf_ctx_inst(ctx, "key");
    te_errno rc;
    key_info new_key = {
        .need_generation = false
    };

    if (te_enum_map_from_str(key_managers, val, -1) < 0)
        return TE_RC(TE_TA_UNIX, TE_EPROTONOSUPPORT);
    if (find_key(id) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    new_key.private_file = te_file_create_unique("%ste_ssh_key_%s", NULL,
                                                 ta_tmp_dir, id);
    if (new_key.private_file == NULL)
    {
        ERROR("Cannot create a private key file");
        return TE_RC(TE_TA_UNIX, TE_EIO);
    }
    new_key.public_file = te_str_concat(new_key.private_file, ".pub");
    new_key.name = strdup(id);

    rc = TE_VEC_APPEND(&known_keys, new_key);
    if (rc != 0)
    {
        free_key_data(&new_key);
        return TE_RC_UPSTREAM(TE_TA_UNIX, rc);
    }

    return 0;
}

static te_errno
key_del(const ta_conf_ctx *ctx)
{
    key_info *found = find_key(ta_conf_ctx_inst(ctx, "key"));

    if (found == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    free_key_data(found);
    te_vec_remove_index(&known_keys, te_vec_get_index(&known_keys, found));
    return 0;
}

static te_errno
key_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const key_info *iter;

    UNUSED(ctx);

    TE_VEC_FOREACH(&known_keys, iter)
    {
        char *name = TE_STRDUP(iter->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static te_errno
key_commit(const ta_conf_ctx *ctx)
{
    key_info *key = find_key(ta_conf_ctx_inst(ctx, "key"));
    te_errno rc;

    /* if the key is not found, it has been deleted, nothing to commit */
    if (key == NULL)
        return 0;

    /* nothing to do, key is in up to date state */
    if (!key->need_generation)
    {
        RING("The key '%s' is up to date, no need to regenerate",
             key->name);
        return 0;
    }

    if (key->type == NULL)
    {
        ERROR("Type of key '%s' is not known", key->name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = agent_key_generate(AGENT_KEY_MANAGER_SSH,
                            key->type, key->bitsize, NULL,
                            key->private_file);
    if (rc != 0)
        return TE_RC_UPSTREAM(TE_TA_UNIX, rc);

    if (access(key->public_file, R_OK) != 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Public key file '%s' of key '%s' is unreadable: %r",
              key->public_file, key->name, rc);
        return rc;
    }

    key->need_generation = false;

    return 0;
}

static te_errno
key_type_get(const ta_conf_ctx *ctx, te_string *val)
{
    const key_info *key = find_key(ta_conf_ctx_inst(ctx, "key"));

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%s", key->type == NULL ? "" : key->type);
    return 0;
}

static te_errno
key_type_set(const ta_conf_ctx *ctx, const char *val)
{
    key_info *key = find_key(ta_conf_ctx_inst(ctx, "key"));

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (key->type != NULL && strcmp(key->type, val) == 0)
        return 0;

    free(key->type);
    key->type = strdup(val);
    key->need_generation = true;

    return 0;
}

static te_errno
key_bitsize_get(const ta_conf_ctx *ctx, int32_t *val)
{
    const key_info *key = find_key(ta_conf_ctx_inst(ctx, "key"));

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = key->bitsize;
    return 0;
}

static te_errno
key_bitsize_set(const ta_conf_ctx *ctx, int32_t val)
{
    key_info *key = find_key(ta_conf_ctx_inst(ctx, "key"));

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (val < 0 || val > UINT_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    key->need_generation = (key->bitsize != val);
    key->bitsize = val;

    return 0;
}

static te_errno
key_private_file_get(const ta_conf_ctx *ctx, te_string *val)
{
    const key_info *key = find_key(ta_conf_ctx_inst(ctx, "key"));

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    assert(key->private_file != NULL);
    te_string_append(val, "%s", key->private_file);
    return 0;
}

static te_errno
key_public_get(const ta_conf_ctx *ctx, te_string *val)
{
    const key_info *key = find_key(ta_conf_ctx_inst(ctx, "key"));
    te_errno rc;

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    assert(key->public_file != NULL);
    rc = te_file_read_string(val, false, RCF_MAX_VAL - 1, "%s",
                             key->public_file);

    return TE_RC_UPSTREAM(TE_TA_UNIX, rc);
}

static const ta_conf_node *const node_key =
    TA_CONF_COLL_STR_RW_COMMIT("key", key_get, key_set, key_add,
                               key_del, key_list, key_commit,
        TA_CONF_RW_STR("type", key_type_get, key_type_set),
        TA_CONF_RW_INT32("bitsize", key_bitsize_get, key_bitsize_set),
        TA_CONF_RO_STR("private_file", key_private_file_get),
        TA_CONF_RO_STR("public", key_public_get));

te_errno
ta_unix_conf_key_init()
{
    return ta_conf_register("/agent", node_key);
}

void
ta_unix_conf_key_fini()
{
    key_info *iter;

    TE_VEC_FOREACH(&known_keys, iter)
    {
        free_key_data(iter);
    }
    te_vec_free(&known_keys);
}
