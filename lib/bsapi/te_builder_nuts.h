/** @file
 * @brief Test Environment
 *
 * Definition of the C API provided by Builder to tests (NUT images building)
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

#ifndef __TE_BUILDER_NUTS_H__
#define __TE_BUILDER_NUTS_H__
#ifdef __cplusplus
extern "C" {
#endif

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
extern int builder_build_nut(const char *name, const char *sources,
                             const char *params);

#ifdef __cplusplus
}
#endif
#endif /* __TE_BUILDER_NUTS_H__ */
