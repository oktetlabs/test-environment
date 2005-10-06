/** @file
 * @brief RCF Common Definitions 
 *
 * Definitions common for all RCF modules.
 * DO NOT include directly from non-RCF sources.
 *
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_RCF_COMMON_H__
#define __TE_RCF_COMMON_H__


/** 
 * Maximum length of the name of the Test Agent, TA type, variable, etc.
 * (including '\0').
 */
#define RCF_MAX_NAME        64

#define RCF_MAX_ID          128 /**< Maximum object or stack 
                                     identifier length */
#define RCF_MAX_VAL         128 /**< Maximum length of variable value
                                     or object instance value */
#define RCF_MAX_PATH        128 /**< Maximum full file name */

#define RCF_MAX_PARAMS      10  /**< Maximum number of routine parameters */


/** Prefix of the fake memory file system */
#define RCF_FILE_MEM_PREFIX     "memory/"
/** Separator used between memory address and length */
#define RCF_FILE_MEM_SEP        ':'

/** Prefix of the fake temporary file system */
#define RCF_FILE_TMP_PREFIX     "te_tmp/"

/** Prefix for files, which should be created on in FTP public directory */
#define RCF_FILE_FTP_PREFIX     "ftp/"

/** Parameter and variable types */
typedef enum {
    RCF_INT8 = 1,   /**< int8_t */
    RCF_UINT8,      /**< uint8_t */
    RCF_INT16,      /**< int16_t */    
    RCF_UINT16,     /**< uint16_t */
    RCF_INT32,      /**< int32_t */
    RCF_UINT32,     /**< uint32_t */
    RCF_INT64,      /**< int64_t */
    RCF_UINT64,     /**< uint64_t */
    RCF_STRING,     /**< char * */
    RCF_TYPE_TOTAL  /**< total number of RCF types */
} rcf_var_type_t;

#endif /* !__TE_RCF_COMMON_H__ */
