/** @file
 * @brief Configurator Tester
 *
 * Test Example
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
 *
 * @author  Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 */

#define LOG_LEVEL 0xff
#define TE_LOG_LEVEL 0xff

#include "test.h"
#include "../paths.c"

DEFINE_LGR_ENTITY("test");

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

