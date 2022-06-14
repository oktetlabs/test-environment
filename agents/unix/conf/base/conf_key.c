/** @file
 * @brief Crypto keys support
 *
 * Crypto key configuration tree support
 *
 * Copyright (C) 2003-2022 OKTET Labs. All rights reserved.
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

#define TE_LGR_USER     "Conf Keys"

#include "te_config.h"
#include "config.h"

#include "te_defs.h"

#include "te_errno.h"
#include "conf_oid.h"
#include "logger_api.h"
#include "te_str.h"
#include "rcf_pch.h"
#include "unix_internal.h"
#include "te_vector.h"
#include "te_enum.h"
#include "te_file.h"

/** Generated key info */
typedef struct key_info {
    /** Name of the key */
    char *name;

    /** If TRUE, the key should be regenerated on commit */
    te_bool need_generation;

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
key_get(unsigned int gid, const char *oid, char *value, const char *id)
{
    const key_info *key = find_key(id);

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return TE_RC_UPSTREAM(TE_TA_UNIX,
                          te_strlcpy_safe(value,
                                          te_enum_map_from_value(
                                              key_managers,
                                              AGENT_KEY_MANAGER_SSH),
                                          RCF_MAX_VAL));
}


/*
 * This function is intentionally a no-op, it only checks the correctness of
 * its argument. It exists only because objects with commit method do not
 * behave well if they have no set method as well
 */
static te_errno
key_set(unsigned int gid, const char *oid, const char *value, const char *id)
{
    const key_info *key = find_key(id);

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (te_enum_map_from_str(key_managers, value, -1) != AGENT_KEY_MANAGER_SSH)
        return TE_RC(TE_TA_UNIX, TE_EPROTONOSUPPORT);

    return 0;
}


static te_errno
key_add(unsigned int gid, const char *oid, const char *value, const char *id)
{
    te_errno rc;
    key_info new_key = {
        .need_generation = FALSE
    };

    UNUSED(gid);
    UNUSED(oid);

    if (te_enum_map_from_str(key_managers, value, -1) < 0)
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
key_del(unsigned int gid, const char *oid, const char *id)
{
    key_info *found = find_key(id);

    if (found == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    free_key_data(found);
    te_vec_remove_index(&known_keys, te_vec_get_index(&known_keys, found));
    return 0;
}

static te_errno
key_list(unsigned int gid, const char *oid, const char *sub_id, char **list)
{
    te_string buf = TE_STRING_INIT;
    const key_info *iter;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    TE_VEC_FOREACH(&known_keys, iter)
    {
        te_errno rc;

        rc = te_string_append(&buf, "%s%s",
                              buf.len == 0 ? "" : " ", iter->name);
        if (rc != 0)
        {
            te_string_free(&buf);
            return TE_RC_UPSTREAM(TE_TA_UNIX, rc);
        }
    }

    te_string_move(list, &buf);
    return 0;
}

static te_errno
key_commit(unsigned int gid, const cfg_oid *p_oid)
{
    key_info *key = find_key(CFG_OID_GET_INST_NAME(p_oid, 2));
    te_errno rc;

    UNUSED(gid);

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

    key->need_generation = FALSE;

    return 0;
}

static te_errno
key_type_get(unsigned int gid, const char *oid, char *value, const char *id)
{
    const key_info *key = find_key(id);

    UNUSED(gid);
    UNUSED(oid);

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return TE_RC_UPSTREAM(TE_TA_UNIX,
                          te_strlcpy_safe(value, key->type == NULL ? "" :
                                          key->type, RCF_MAX_VAL));
}

static te_errno
key_type_set(unsigned int gid, const char *oid, const char *value,
             const char *id)
{
    key_info *key = find_key(id);

    UNUSED(gid);
    UNUSED(oid);

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (key->type != NULL && strcmp(key->type, value) == 0)
        return 0;

    free(key->type);
    key->type = strdup(value);
    key->need_generation = TRUE;

    return 0;
}

static te_errno
key_bitsize_get(unsigned int gid, const char *oid, char *value, const char *id)
{
    const key_info *key = find_key(id);

    UNUSED(gid);
    UNUSED(oid);

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SNPRINTF(value, RCF_MAX_VAL, "%u", key->bitsize);
    return 0;
}

static te_errno
key_bitsize_set(unsigned int gid, const char *oid, const char *value,
                const char *id)
{
    key_info *key = find_key(id);
    unsigned new_bitsize;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoui(value, 10, &new_bitsize);
    if (rc != 0)
        return TE_RC_UPSTREAM(TE_TA_UNIX, rc);

    key->need_generation = (key->bitsize != new_bitsize);
    key->bitsize = new_bitsize;

    return 0;
}

static te_errno
key_private_file_get(unsigned int gid, const char *oid, char *value,
                     const char *id)
{
    const key_info *key = find_key(id);

    UNUSED(gid);
    UNUSED(oid);

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    assert(key->private_file != NULL);
    return TE_RC_UPSTREAM(TE_TA_UNIX,
                          te_strlcpy_safe(value, key->private_file,
                                          RCF_MAX_VAL));
}

static te_errno
key_public_get(unsigned int gid, const char *oid, char *value,
               const char *id)
{
    const key_info *key = find_key(id);
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if (key == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    assert(key->public_file != NULL);
    rc = te_file_read_text(key->public_file, value, RCF_MAX_VAL);

    return TE_RC_UPSTREAM(TE_TA_UNIX, rc);
}

RCF_PCH_CFG_NODE_RO(node_key_public, "public",
                    NULL, NULL,
                    key_public_get);

RCF_PCH_CFG_NODE_RO(node_key_private_file, "private_file",
                    NULL, &node_key_public,
                    key_private_file_get);


RCF_PCH_CFG_NODE_RW(node_key_bitsize, "bitsize",
                    NULL, &node_key_private_file,
                    key_bitsize_get, key_bitsize_set);

RCF_PCH_CFG_NODE_RW(node_key_type, "type",
                    NULL, &node_key_bitsize,
                    key_type_get, key_type_set);


RCF_PCH_CFG_NODE_RW_COLLECTION(node_key, "key",
                               &node_key_type, NULL,
                               key_get, key_set, key_add, key_del,
                               key_list, key_commit);

te_errno
ta_unix_conf_key_init()
{
    return rcf_pch_add_node("/agent", &node_key);
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
