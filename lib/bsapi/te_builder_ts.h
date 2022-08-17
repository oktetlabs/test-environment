/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Definition of the C API provided by Builder to Tester (Test Suite
 * building)
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_BUILDER_TS_H__
#define __TE_BUILDER_TS_H__

#include "te_errno.h"

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
 * @return Status code.
 */
extern te_errno builder_build_test_suite(const char *suite,
                                         const char *sources);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_BUILDER_TS_H__ */
