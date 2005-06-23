/** @file
 * @brief Configurator Tester
 *
 * Test Interface
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
#include "te_config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif


#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_defs.h"
#include "logger_api.h"
#include "logger_ten.h"

#include "../rcf_emul/rcf_emul.h"
#include "conf_api.h"

/** Maximum length of the path in linux */
#define PATH_MAX 1024

/**
 * Define common test params. They are internal so
 * implementer of the test should not know about them
 */
#define COMMON_TEST_PARAMS \
    pthread_t               rcf_emul_thread;                \
    int                     rc = -1;                        \
    int                     result = -1;                    \
    
/**
 * Starting RCF emulator thread. No configurations are created.
 *
 * @param config_file_name_  Name of the database configuration file.
 */
#define START_RCF_EMULATOR(config_file_name_) \
    do {                                                          \
        char config_file[PATH_MAX];                               \
                                                                  \
        memset(config_file, 0, sizeof(config_file));              \
        strcpy(config_file, srcdir);                              \
        strcat(config_file, "/suites/");                          \
        strcat(config_file, config_file_name_);                   \
        pthread_create(&rcf_emul_thread, NULL,                    \
                       rcf_emulate, config_file);                 \
    } while (0)

/**
 * Starts configurator with the given configuration file.
 * For normal work of the configurator before calling this
 * macro the current request handling configuration should
 * be registered.
 *
 * @param conf_file_      Configuration file of the configurator.
 */
#define START_CONFIGURATOR(conf_file_) \
    do {                                                                      \
        int pid;                                                              \
        int rc;                                                               \
        char *te_call = calloc(PATH_MAX, 1);                                  \
        char *file_name = calloc(PATH_MAX, 1);                                \
                                                                              \
        sleep(3);                                                             \
        if (te_call == NULL || file_name == NULL)                             \
        {                                                                     \
            printf("Failed to allocate buffer\n");                            \
            goto cleanup;                                                     \
        }                                                                     \
        strcpy(file_name, srcdir);                                            \
        strcat(file_name, "/suites/");                                        \
        strcat(file_name, conf_file_);                                        \
        te_call = strcat(te_call, "../../te_cs");                             \
        pid = fork();                                                         \
        if (pid == 0)                                                         \
        {                                                                     \
            rc = execl(te_call, te_call,                                      \
                       file_name,                                             \
                       NULL);                                                 \
            if (rc < 0)                                                       \
            {                                                                 \
                printf("Failed to start configurator, error = %d\n", errno);  \
                goto cleanup;                                                 \
            }                                                                 \
            return 0;                                                         \
        }                                                                     \
        sleep(1);                                                             \
    } while (0)

/**
 * Starts logger with the given configuration file.
 */
#define START_LOGGER(log_file_) \
    do {                                                                     \
        int pid;                                                             \
        int rc;                                                              \
        char *te_call = calloc(PATH_MAX, 1);                                 \
        char *file_name = calloc(PATH_MAX, 1);                               \
                                                                             \
        if (te_call == NULL || file_name == NULL)                            \
        {                                                                    \
            printf("Failed to allocate buffer\n");                           \
            goto cleanup;                                                    \
        }                                                                    \
        strcpy(file_name, srcdir);                                           \
        file_name = strcat(file_name, "/suites/");                           \
        file_name = strcat(file_name, log_file_);                            \
        te_call = strcat(te_call,                                            \
                         "../../../logger/te_logger");                       \
        pid = fork();                                                        \
        if (pid == 0)                                                        \
        {                                                                    \
            rc = execl(te_call, te_call,                                     \
                       file_name,                                            \
                       NULL);                                                \
            if (rc < 0)                                                      \
            {                                                                \
                printf("Failed to start logger %d\n", errno);                \
                goto cleanup;                                                \
            }                                                                \
            return 0;                                                        \
        }                                                                    \
    } while (0)

/* 
 * Exports environment variables which are necessary
 * for correct work of the testing network.
 *
 * TE_TMP
 * TE_TMP
 * TE_LOG_DIR
 * TE_LOG_RAW
 * LD_LIBRARY_PATH
 */
#define EXPORT_ENV \
    do {                                                                    \
        char *env = calloc(PATH_MAX, 1);                                    \
                                                                            \
        memset(env, 0, sizeof(env));                                        \
                                                                            \
        strcat(env, te_install);                                            \
        strcat(env, "/lib/");                                               \
        setenv("LD_LIBRARY_PATH", env, TRUE);                               \
        setenv("TE_TMP", "/tmp/", TRUE);                                    \
                                                                            \
        setenv("TE_LOG_DIR", "/tmp", TRUE);                                 \
        setenv("TE_LOG_RAW", "/tmp/conf_tester_tmp_raw_log", TRUE);         \
        printf("==================\n");                                     \
    } while (0)

/**
 * Stops the RCF Emulator
 */
#define STOP_RCF_EMULATOR \
    do {                                                      \
        VERB("Shutting down the RCF Emulator");               \
        if (rcf_shutdown_call() != 0)                         \
        {                                                     \
            printf("Failed to shut down the RCF Emulator\n"); \
            result = -1;                                      \
        }                                                     \
    } while (0)

/**
 * Stops the TE Logger.
 */
#define STOP_LOGGER \
    do {                                                                     \
        char *te_call = calloc(PATH_MAX, 1);                                 \
                                                                             \
        if (te_call == NULL)                                                 \
        {                                                                    \
            printf("Failed to shut down the logger\n");                      \
        }                                                                    \
        memset(te_call, 0, PATH_MAX);                                        \
        strcat(te_call, "../../../logger/te_log_shutdown");                  \
        VERB("Shutting down the Logger");                                    \
        system(te_call); \
    } while (0)

/**
 * Stops the configurator.
 */
#define STOP_CONFIGURATOR \
    do {                                                                     \
        char *te_call = calloc(PATH_MAX, 1);                                 \
                                                                             \
        if (te_call == NULL)                                                 \
        {                                                                    \
            printf("Failed to shut down the configurator\n");                \
        }                                                                    \
        memset(te_call, 0, PATH_MAX);                                        \
        strcat(te_call, "../../te_cs_shutdown");                              \
                                                                             \
        VERB("Shutting down the Configurator");                              \
        system(te_call); \
    } while (0)

/**
 * Returns the test result.
 */
#define CONFIGURATOR_TEST_END \
    return result

/**
 * This macro should be called just before cleanup mark.
 */
#define CONFIGURATOR_TEST_SUCCESS \
    result = 0
