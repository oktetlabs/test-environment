/** @file
 * @brief Tester Subsystem
 *
 * Types support library declaration.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifndef __TE_TESTER_TYPE_LIB_H__
#define __TE_TESTER_TYPE_LIB_H__

#include "te_errno.h"

#include "tester_conf.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize types support library.
 *
 * @return Status code.
 */
extern te_errno tester_init_types(void);

/**
 * Find type by name in current context.
 *
 * @param session       Test session context
 * @param name          Name of the type to find
 *
 * @return Pointer to found type or NULL.
 */
extern const test_value_type * tester_find_type(const test_session *session,
                                                const char         *name);

/**
 * Register new type in the current context.
 *
 * @param session       Test session context
 * @param type          Type description
 */
extern void tester_add_type(test_session *session, test_value_type *type);


/**
 * Check that plain value belongs to type.
 *
 * @param type          Type description
 * @param plain         Plain value
 *
 * @return Pointer to corresponding value description or NULL.
 */
extern const test_entity_value * tester_type_check_plain_value(
                                     const test_value_type *type,
                                     const char            *plain);

#endif /* !__TE_TESTER_TYPE_LIB_H__ */
