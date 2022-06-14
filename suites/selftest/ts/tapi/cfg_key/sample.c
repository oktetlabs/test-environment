/** @file
 * @brief TAPI Key management testing
 *
 * Key management testing
 *
 * Copyright (C) 2022 OKTET Labs Ltd., St.-Petersburg, Russia
 */

/** @page cfg_key-sample A sample of key management TAPI
 *
 * @objective Check that key management routines work correctly
 *
 * @par Scenario:
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

#define TE_TEST_NAME "cfg_key/sample"


#ifndef TEST_START_VARS
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include "te_config.h"
#include "tapi_cfg_key.h"
#include "tapi_test.h"
#include "tapi_env.h"

typedef struct key_data
{
    tapi_cfg_key_manager manager;
    tapi_cfg_key_type type;
    tapi_cfg_key_size size;
    tapi_cfg_key_type other_type;
    tapi_cfg_key_size other_size;
    const char *name;
    unsigned exp_size;
    char *public_key;
    char *private_key_path;
} key_data;

int
main(int argc, char **argv)
{
    key_data keys[] = {
        {.manager = TAPI_CFG_KEY_MANAGER_SSH,
         .type = TAPI_CFG_KEY_TYPE_SSH_RSA,
         .size = TAPI_CFG_KEY_SIZE_MIN,
         .other_type = TAPI_CFG_KEY_TYPE_SSH_RSA,
         .other_size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .name = "rsa-min",
         .exp_size = 1024,
        },
        {.manager = TAPI_CFG_KEY_MANAGER_SSH,
         .type = TAPI_CFG_KEY_TYPE_SSH_RSA,
         .size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .other_type = TAPI_CFG_KEY_TYPE_SSH_DSA,
         .other_size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .name = "rsa-recommended",
         .exp_size = 3072,
        },
        {.manager = TAPI_CFG_KEY_MANAGER_SSH,
         .type = TAPI_CFG_KEY_TYPE_SSH_RSA,
         .size = TAPI_CFG_KEY_SIZE_MAX,
         .other_type = TAPI_CFG_KEY_TYPE_SSH_ECDSA,
         .other_size = TAPI_CFG_KEY_SIZE_MAX,
         .name = "rsa-max",
         .exp_size = 4096,
        },
        {.manager = TAPI_CFG_KEY_MANAGER_SSH,
         .type = TAPI_CFG_KEY_TYPE_SSH_DSA,
         .size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .other_type = TAPI_CFG_KEY_TYPE_SSH_RSA,
         .other_size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .name = "dsa-default",
         .exp_size = 1024,
        },
        {.manager = TAPI_CFG_KEY_MANAGER_SSH,
         .type = TAPI_CFG_KEY_TYPE_SSH_ECDSA,
         .size = TAPI_CFG_KEY_SIZE_MIN,
         .other_type = TAPI_CFG_KEY_TYPE_SSH_RSA,
         .other_size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .name = "ecdsa-min",
         .exp_size = 256,
        },
        {.manager = TAPI_CFG_KEY_MANAGER_SSH,
         .type = TAPI_CFG_KEY_TYPE_SSH_ECDSA,
         .size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .other_type = TAPI_CFG_KEY_TYPE_SSH_ECDSA,
         .other_size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .name = "ecdsa-recommended",
         .exp_size = 384,
        },
        {.manager = TAPI_CFG_KEY_MANAGER_SSH,
         .type = TAPI_CFG_KEY_TYPE_SSH_ECDSA,
         .size = TAPI_CFG_KEY_SIZE_MAX,
         .other_type = TAPI_CFG_KEY_TYPE_SSH_ED25519,
         .other_size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .name = "ecdsa-max",
         .exp_size = 521,
        },
        {.manager = TAPI_CFG_KEY_MANAGER_SSH,
         .type = TAPI_CFG_KEY_TYPE_SSH_ED25519,
         .size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .other_type = TAPI_CFG_KEY_TYPE_SSH_ED25519,
         .other_size = TAPI_CFG_KEY_SIZE_RECOMMENDED,
         .name = "ed25519-default",
         .exp_size = 1,
        },
    };

    rcf_rpc_server *pco_iut = NULL;
    unsigned i;
    unsigned keys_added = 0;

    TEST_START;

    TEST_GET_PCO(pco_iut);

    TEST_STEP("Adding keys");
    for (i = 0; i < TE_ARRAY_LEN(keys); i++)
    {
        CHECK_RC(tapi_cfg_key_add(pco_iut->ta, keys[i].name,
                                  keys[i].manager, keys[i].type, keys[i].size,
                                  TAPI_CFG_KEY_MODE_NEW));
        keys_added++;
    }

    TEST_STEP("Checking bit size");
    for (i = 0; i < keys_added; i++)
    {
        unsigned bitsize;

        bitsize = tapi_cfg_key_get_bitsize(pco_iut->ta, keys[i].name);
        if (bitsize != keys[i].exp_size)
        {
            TEST_VERDICT("Unexpected bit size for key '%s': "
                         "expected %u, got %u", keys[i].name, keys[i].exp_size,
                         bitsize);
        }
    }

    TEST_STEP("Checking key existence");
    for (i = 0; i < keys_added; i++)
    {
        if (!tapi_cfg_key_exists(pco_iut->ta, keys[i].name))
            TEST_VERDICT("Key '%s' reported as non-existing", keys[i].name);
    }

    TEST_STEP("Verify that the key cannot be added again");
    for (i = 0; i < keys_added; i++)
    {
        te_errno rc = tapi_cfg_key_add(pco_iut->ta, keys[i].name,
                                       keys[i].manager, keys[i].type,
                                       keys[i].size,
                                       TAPI_CFG_KEY_MODE_NEW);
        if (TE_RC_GET_ERROR(rc) != TE_EEXIST)
        {
            TEST_VERDICT("Unexpected return status for duplicate key '%s': %r",
                         keys[i].name, rc);
        }
    }

    TEST_STEP("Getting private and public keys");
    for (i = 0; i < keys_added; i++)
    {
        keys[i].private_key_path =
            tapi_cfg_key_get_private_key_path(pco_iut->ta,
                                              keys[i].name);
        if (keys[i].private_key_path == NULL)
            TEST_VERDICT("Cannot get private key path for '%s'", keys[i].name);


        keys[i].public_key = tapi_cfg_key_get_public_key(pco_iut->ta,
                                                         keys[i].name);
        if (keys[i].public_key == NULL)
            TEST_VERDICT("Cannot get public key for '%s'", keys[i].name);

        if (*keys[i].public_key == '\0')
            TEST_VERDICT("Public key of '%s' is empty", keys[i].name);

        if (keys[i].public_key[strlen(keys[i].public_key) - 1] == '\n')
        {
            TEST_VERDICT("Trailing newline in the public key of '%s'",
                         keys[i].name);
        }

        RING("Key '%s': private key path = %s, public key = %s",
             keys[i].name, keys[i].private_key_path, keys[i].public_key);

        if (*keys[i].private_key_path == '\0')
            TEST_VERDICT("Private key path for '%s' is empty", keys[i].name);
    }

    TEST_STEP("Reusing keys");
    for (i = 0; i < keys_added; i++)
    {
        char *result;

        CHECK_RC(tapi_cfg_key_add(pco_iut->ta, keys[i].name,
                                  keys[i].manager, keys[i].type,
                                  keys[i].size, TAPI_CFG_KEY_MODE_REUSE));
        CHECK_NOT_NULL(result =
                       tapi_cfg_key_get_private_key_path(pco_iut->ta,
                                                         keys[i].name));
        if (strcmp(result, keys[i].private_key_path) != 0)
        {
            free(result);
            TEST_VERDICT("Private key path for '%s' has changed", keys[i].name);
        }
        free(result);
        CHECK_NOT_NULL(result = tapi_cfg_key_get_public_key(pco_iut->ta,
                                                            keys[i].name));
        if (strcmp(result, keys[i].public_key) != 0)
        {
            free(result);
            TEST_VERDICT("Public key for '%s' has changed unexpectedly",
                         keys[i].name);
        }
        free(result);
    }

    TEST_STEP("Replacing keys");
    for (i = 0; i < keys_added; i++)
    {
        char *result;

        if (keys[i].other_type != keys[i].type ||
            keys[i].other_size != keys[i].size)
        {
            te_errno rc = tapi_cfg_key_add(pco_iut->ta, keys[i].name,
                                           keys[i].manager, keys[i].other_type,
                                           keys[i].other_size,
                                           TAPI_CFG_KEY_MODE_REUSE);
            if (TE_RC_GET_ERROR(rc) != TE_EBADSLT)
            {
                TEST_VERDICT("Attempted to reuse key '%s', "
                             "unexpected result %r",
                             keys[i].name, rc);
            }
        }

        CHECK_RC(tapi_cfg_key_add(pco_iut->ta, keys[i].name,
                                  keys[i].manager, keys[i].other_type,
                                  keys[i].other_size,
                                  TAPI_CFG_KEY_MODE_REPLACE));

        CHECK_NOT_NULL(result =
                       tapi_cfg_key_get_private_key_path(pco_iut->ta,
                                                         keys[i].name));
        if (strcmp(result, keys[i].private_key_path) != 0)
        {
            free(result);
            TEST_VERDICT("Private key path for '%s' has changed", keys[i].name);
        }
        free(result);
        CHECK_NOT_NULL(result = tapi_cfg_key_get_public_key(pco_iut->ta,
                                                            keys[i].name));
        if (keys[i].other_type != keys[i].type ||
            keys[i].other_size != keys[i].size)
        {
            if (strcmp(result, keys[i].public_key) == 0)
            {
                free(result);
                TEST_VERDICT("Public key for '%s' has NOT changed when it should",
                             keys[i].name);
            }
        }
        else
        {
            if (strcmp(result, keys[i].public_key) != 0)
            {
                free(result);
                TEST_VERDICT("Public key for '%s' has changed unexpectedly",
                             keys[i].name);
            }
        }
        free(result);
    }

    TEST_STEP("Deleting keys");

    while (keys_added > 0)
    {
        keys_added--;
        RING("Deleting key %u", keys_added);
        CHECK_RC(tapi_cfg_key_del(pco_iut->ta, keys[keys_added].name));
    }
    TEST_SUCCESS;

cleanup:

    for (i = 0; i < TE_ARRAY_LEN(keys); i++)
    {
        if (i < keys_added)
            tapi_cfg_key_del(pco_iut->ta, keys[i].name);
        free(keys[i].private_key_path);
        free(keys[i].public_key);
    }

    TEST_END;
}
