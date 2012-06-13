/** @file
 * @brief Demo: Minimal Test Suite
 *
 * Minial test scenario (empty test).
 *
 * Copyright (C) 20012 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME  "sample1"

#include "te_config.h"

#include "logger_api.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    TEST_START;

    RING("Hello World!");

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
