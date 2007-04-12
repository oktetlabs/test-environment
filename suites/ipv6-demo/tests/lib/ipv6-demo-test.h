/** @file
 * @brief IPv6 Demo Test Suite
 *
 * A set of macros intended to be used from tests.
 *
 * Copyright (C) 2007 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __IPV6_DEMO_TEST_H__
#define __IPV6_DEMO_TEST_H__

#include "te_config.h"

#include "ipv6-demo-ts.h"
#include "ipv6-demo-params.h"

#ifndef TEST_START_VARS
/**
 * Test suite specific variables of the test @b main() function.
 */
#define TEST_START_VARS TEST_START_ENV_VARS
#endif

#ifndef TEST_START_SPECIFIC
/**
 * Test suite specific the first actions of the test.
 */
#define TEST_START_SPECIFIC TEST_START_ENV
#endif

#ifndef TEST_END_SPECIFIC
/**
 * Test suite specific part of the last action of the test @b main()
 * function.
 */
#define TEST_END_SPECIFIC TEST_END_ENV
#endif

#include "tapi_test.h"

#endif /* !__IPV6_DEMO_TEST_H__ */
