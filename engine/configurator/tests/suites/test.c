/** @file
 * @brief Configurator Tester
 *
 * Test Example
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define LOG_LEVEL 0xff
#define TE_LOG_LEVEL 0xff

#include "test.h"
#include "../paths.c"

#define RC(expr_) \
    do {                                                \
        int rc_ = 0;                                    \
                                                        \
        rc_ = (expr_);                                  \
        if (rc_ < 0)                                    \
        {                                               \
            printf("%s returned %d\n", # expr_, rc_);   \
            goto cleanup;                               \
        }                                               \
    } while (0)

int
main(void)
{
    COMMON_TEST_PARAMS;
    int                     conf;
    cfg_obj_descr           descr = {CVT_STRING, CFG_READ_WRITE};

    te_log_init("test", te_log_message_file);

    /*
     * Exporting environment variables which are necessary
     * for correct work of the testing network.
     */
    EXPORT_ENV;

    START_LOGGER("logger.conf");
    START_RCF_EMULATOR("config.db");
    RCFRH_CONFIGURATION_CREATE(conf);
    RCFRH_SET_DEFAULT_HANDLERS(conf);
    RCFRH_CONFIGURATION_SET_CURRENT(conf);

    START_CONFIGURATOR("test.conf");
    RC(cfg_commit("/agent:Agt_T"));

    CONFIGURATOR_TEST_SUCCESS;
cleanup:
    STOP_CONFIGURATOR;
    STOP_RCF_EMULATOR;
    STOP_LOGGER;

    CONFIGURATOR_TEST_END;
}

