/** @file
 * @brief Test Environment
 *
 * A test example
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Ivan Soloducha <Ivan.Soloducha@oktetlabs.ru>
 * 
 */

/** @page loggertest Test for Logger
 *
 *  @objective Check that format specifiers are printed correctly.
 *
 */

#define TE_TEST_NAME  "loggertest"

#include "tapi_test.h"
#include "te_config.h"



int
main(int argc, char *argv[])
{
    char s[] = "String in memory";

    
    TEST_START;

    RING("Integers with standard length modifiers: %hhd %hd %ld %lld", 11, 1111, 11111111, 1111111111111111);
    RING("Integers with '=' length modifiers: %=1d %=2d 0x%=4x 0x%=8x", 11, 1111, 11111111, 1111111111111111);
    RING("Integer with invalid length modifiers: %=3d %llld %hhhd", 77, 88, 99);
    
    RING("Integers with '%%j' and '%%t' modifiers: %jd %td", 111, 222);
    RING("Integers with flags: %+0d, %'0d, %-'d, %#+d", 111, 222, 333, 444);
    RING("Formatted integers: %10.5d, %5.10d", 111111111111, 222222222222);
    
    RING("Message with string parameter: (%s)", "String parameter");
    RING("Message with string parameter containing percent: (%s)", "%tring wi% per%ent%");
    
    RING("Memory dump: location %p, length %d\n%Tm", s, sizeof(s), s, sizeof(s));

    RING("Error message: %r", RPC_EINVAL);

    RING("Invalid specifier starting with 'T': %Te");
    
    RING("Specifier after double percent is not a specifier: %%d");
    
    RING("File dump: existing file %Tf", "test.txt");
    RING("File dump: non-existing file %Tf", "test2.txt");

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
