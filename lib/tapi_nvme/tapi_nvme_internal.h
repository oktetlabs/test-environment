/** @file
 * @brief Test API NVMe internal
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TAPI_NVME_INTERNAL_H__
#define __TAPI_NVME_INTERNAL_H__

#include "te_defs.h"
#include "rcf_rpc.h"
#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAPI_NVME_INTERNAL_MODE \
    (RPC_S_IRWXU | RPC_S_IRGRP | RPC_S_IXGRP | RPC_S_IROTH | RPC_S_IXOTH)

#define TAPI_NVME_INTERNAL_DEF_TIMEOUT  (0)

/**
 * Analog of echo 'string' >> path
 *
 * @param rpcs      RPC server
 * @param string    String for append to file
 * @param fmt       Format of path
 * @param ...       Arguments for format path
 *
 * @return Status code
 */
extern te_errno tapi_nvme_internal_file_append(rcf_rpc_server *rpcs,
    unsigned int timeout_sec, const char *string, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

/**
 * Read file in buffer
 *
 * @param rpcs      RPC server
 * @param buffer    Buffer to store the result
 * @param size      Size of buffer
 * @param fmt       Format of path
 * @param ...       Arguments for format path
 *
 * @return Number of read bytes
 * @retval -1       if file cannot opened or read not success
 */
extern int tapi_nvme_internal_file_read(rcf_rpc_server *rpcs,
    char *buffer, size_t size, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

/**
 * Check that directory is exist
 *
 * @param rpcs      RPC server
 * @param path      Path for test
 *
 * @return existence of the directory
 */
extern te_bool tapi_nvme_internal_isdir_exist(rcf_rpc_server *rpcs,
                                              const char *path);

/**
 * Create directory
 *
 * @param rpcs      RPC server
 * @param fmt       Format of path
 * @param ...       Arguments for format path
 *
 * @return Success of creation
 */
extern te_bool tapi_nvme_internal_mkdir(rcf_rpc_server *rpcs,
    const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/**
 * Remove directory
 *
 * @param rpcs      RPC server
 * @param fmt       Format of path
 * @param ...       Arguments for format path
 *
 * @return Success of creation
 */
extern te_bool tapi_nvme_internal_rmdir(rcf_rpc_server *rpcs,
    const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/** Directory info */
typedef struct tapi_nvme_internal_dirinfo {
    char name[NAME_MAX];        /**< Name of directory */
} tapi_nvme_internal_dirinfo;

/**
 * Search all directories of file in @p path begin @p start_from
 *
 * @param rpcs          RPC server
 * @param path          Path for search
 * @param start_from    Pattern for search
 * @param result        Found directories or files
 *                      te_vector have type tapi_nvme_internal_dirinfo
 *
 * @return Status code
 */
extern te_errno tapi_nvme_internal_filterdir(rcf_rpc_server *rpcs,
                                             const char *path,
                                             const char *start_from,
                                             te_vec *result);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_NVME_INTERNAL_H__ */
