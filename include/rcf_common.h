/** @file
 * @brief RCF Common Definitions
 *
 * Definitions common for all RCF modules.
 * DO NOT include directly from non-RCF sources.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
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

#define RCF_MAX_ID          256 /**< Maximum object or stack
                                     identifier length */
#define RCF_MAX_VAL         (128  * 8) /**< Maximum length of variable value
                                     or object instance value */
#define RCF_MAX_PATH        256 /**< Maximum full file name */

#define RCF_MAX_PARAMS      10  /**< Maximum number of routine parameters */


/** Prefix of the fake memory file system */
#define RCF_FILE_MEM_PREFIX     "memory/"
/** Separator used between memory address and length */
#define RCF_FILE_MEM_SEP        ':'

/** Prefix of the fake temporary file system */
#define RCF_FILE_TMP_PREFIX     "te_tmp/"

/** Prefix for files, which should be created on in FTP public directory */
#define RCF_FILE_FTP_PREFIX     "ftp/"


/**
 * Parameter and variable types.
 *
 * Corresponding integer values must start from 0 and be continuos,
 * since they are used as indexes for rcf_types[] and rcf_type_len[]
 * arrays from te_proto.h.
 */
typedef enum {
    RCF_INT8 = 0,   /**< int8_t */
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

/**
 * The ways a function may be called on TA
 */
typedef enum rcf_execute_mode {
    RCF_FUNC = 1,  /**< Execute a function in the same context */
    RCF_THREAD,    /**< Execute a function in the TA thread */
    RCF_PROCESS    /**< Execute a function in the separate process */
} rcf_execute_mode;

#endif /* !__TE_RCF_COMMON_H__ */
