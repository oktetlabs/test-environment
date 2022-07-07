/** @file
 * @brief Test allocation of aligned and misaligned memory.
 *
 * Copyright (C) 2022 OKTET Labs Ltd., St. Petersburg, Russia
 *
 * @author Nikolai Kosovskii <nikolai.kosovskii@oktetlabs.ru>
 */

#ifndef __MEMORY_MEMORY_H__
#define __MEMORY_MEMORY_H__

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
#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_rpc_unistd.h"

#endif /* __MEMORY_MEMORY_H__ */
