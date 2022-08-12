/** @file
 * @brief Platform definitions
 *
 * Includes file with definitions of all platform-dependent constants.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 */

#ifndef __TE_CONFIG_H__
#define __TE_CONFIG_H__

/*
 * Undefine package parameters to have no warnings, if config.h is
 * included before te_config.h. We lose definitions provided by
 * config.h in any case.
 */
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#define __TE_CONFIG_INTERNAL_H__
#include "te_config_internal.h"
#undef __TE_CONFIG_INTERNAL_H__

/*
 * Undefine package parameters to allow including config.h
 * after te_config.h
 */
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#endif
