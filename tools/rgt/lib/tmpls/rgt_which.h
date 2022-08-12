/** @file
 * @brief API to locate the first entry of command name in PATH variable
 *
 *
 * Copyright (C) 2018-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_RGT_WHICH_H__
#define __TE_RGT_WHICH_H__

#include "te_errno.h"

/**
 * Traverse PATH variable and locate first entry that is the same as
 * command and is an executable
 *
 * @warning        If command contains path separator (e.g. '/')
 *                 rgt_which will fail
 * @param[in]   command  Command to find
 * @param[in]   size     Size of the buffer pointed to by location
 * @param[out]  location Pointer to buffer to write result to
 *
 * @return Status code
 */
extern te_errno rgt_which(const char *command, size_t size, char *location);

#endif /* __TE_RGT_WHICH_H__ */
