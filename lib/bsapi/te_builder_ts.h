/** @file
 * @brief Test Environment
 *
 * Definition of the C API provided by Builder to Tester (Test Suite
 * building)
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_BUILDER_TS_H__
#define __TE_BUILDER_TS_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function is called by Tester subsystem to build dynamically
 * a Test Suite. Test Suite is installed to 
 * ${TE_INSTALL_SUITE}/bin/<suite>.
 *
 * Test Suite may be linked with TE libraries. If ${TE_INSTALL} or
 * ${TE_BUILD} are not empty necessary compiler and linker flags are passed
 * to Test Suite configure in variables TE_CFLAGS and TE_LDFLAGS.
 *
 * @param suite         Unique suite name.
 * @param sources       Source location of the Test Suite (Builder will
 *                      look for configure script in this directory.
 *                      If the path is started from "/" it is considered as
 *                      absolute.  Otherwise it is considered as relative
 *                      from ${TE_BASE}.
 *
 * @return error code
 */
extern int builder_build_test_suite(const char *suite, const char *sources);


#ifdef __cplusplus
}
#endif
#endif /* __TE_BUILDER_TS_H__ */
