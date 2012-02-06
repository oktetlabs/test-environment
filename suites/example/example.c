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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 * 
 * $Id$
 */

/** @page example An example test
 *
 *  @objective Example smells example whether it's called example or not.
 *
 */

#include "te_config.h"
#include "tapi_test.h"

#define TE_TEST_NAME "example"

int
main(int argc, char *argv[])
{
    TEST_START;

    RING("All is fine");
    TEST_SUCCESS;

cleanup:


    TEST_END;
}
