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

#include "sockapi-test.h"
#include "te_config.h"



int
main(int argc, char *argv[])
{
    char s[] = "String in memory";

    
    TEST_START;

    RING("Decimal integers with '=' length modifiers: %=1d %=2d %=4d %=8d", 11, 1111, 11111111, 1111111111111111);
    RING("Decimal integer with invalid '=' length modifier: %=3d", 777);
    
    RING("Message with string parameter: (%s)", "String parameter");
    RING("Message with string parameter containing percent: (%s)", "%tring wi% per%ent%");
    
    RING("Memory dump: location %p, length %d\n%Tm", s, sizeof(s), s, sizeof(s));
    
    RING("File dump: existing file %Tf", "test.txt");
    RING("File dump: non-existing file %Tf", "test2.txt");

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
