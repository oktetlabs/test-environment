/** @file
 * @brief Test Environment
 *
 * A test example
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
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
 * @author Ivan Soloducha <Ivan.Soloducha@oktetlabs.ru>
 * 
 * $Id: $
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
