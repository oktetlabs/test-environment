/** @file
 * @brief Test Environment
 *
 * Builder C API library
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

#include <stdio.h>
#include <stdlib.h>

#include "te_errno.h"
#include "te_builder_nuts.h"
#include "te_builder_ts.h"

/* Maxmimum length of the shell command */
#define MAX_SH_CMD      2048

static char cmd[MAX_SH_CMD];

/**
 * This function is called by tests to build dynamically NUT bootable
 * image. The image is put  
 * to ${TE_INSTALL_NUT}/bin if it is not empty or
 * to ${TE_INSTALL}/nuts/bin if ${TE_INSTALL_NUT} is empty.
 * This NUT image is then taken by RCF from this well-known place.
 *
 * NUT may be linked with TA libraries located in ${TE_INSTALL}/agents/lib
 * (if ${TE_INSTALL} is not empty).
 *
 * @param name          Name of the NUT image.
 *
 * @param sources       Source location of NUT sources and NUT building
 *                      script te_build_nut. 
 *                      The parameter may be NULL if necessary
 *                      information is provided in the Builder
 *                      configuration file.
 *
 * @param params        Additional parameters to te_build_nut script
 *                      (first and second parameters always are 
 *                      installation prefix and NUT image name).
 *                      Empty string is allowed.
 *                      The parameter may be NULL if necessary
 *                      information is provided in the Builder
 *                      configuration file.
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       bad name is specified
 * @retval ETESHCMD     system call failed
 */
int 
builder_build_nut(char *name, char *sources, char *params)
{
    if (name == NULL)
        return EINVAL;
        
    if (sources == NULL)
        sources = "\"\"";
        
    if (params == NULL)
        params = "";

    snprintf(cmd, sizeof(cmd), 
             "te_build_nut %s  %s \"%s\" >builder.log.%s 2>&1", 
             name, sources, params, name);

    return system(cmd) != 0 ? ETESHCMD : 0;
}

/**
 * This function is called by Tester subsystem to build dynamically
 * a Test Suite. Test Suite is installed to 
 * ${TE_INSTALL_SUITE}/bin/<suite>.
 *
 * Test Suite may be linked with TE libraries. If ${TE_INSTALL} or ${TE_BUILD}
 * are not empty necessary compiler and linker flags are passed to
 * Test Suite configure in variables TE_CFLAGS and TE_LDFLAGS.
 *
 * @param suite         Unique suite name.
 * @param sources       Source location of the Test Suite (Builder will look 
 *                      for configure script in this directory. If the path
 *                      is started from "/" it is considered as absolute.
 *                      Otherwise it is considered as relative from ${TE_BASE}.
 *
 * @return error code
 *
 * @retval 0            success
 * @retval EINVAL       bad source directory or suite name
 * @retval ETESHCMD     system call failed
 */
int 
builder_build_test_suite(char *suite, char *sources)
{
    if (suite == NULL || *suite == 0 || sources == NULL || *sources == 0)
        return EINVAL;
    
    sprintf(cmd, "te_build_suite %s %s >builder.log.%s 2>&1", suite, sources,
            suite);
    return system(cmd) == 0 ? 0 : ETESHCMD;
}

