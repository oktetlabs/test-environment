/** @file
 * @brief TAD CSAP IDs
 *
 * Traffic Application Domain Command Handler.
 * Definition of CSAP IDs database functions.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 */

#ifndef __TE_TAD_CSAP_ID_H__
#define __TE_TAD_CSAP_ID_H__

#include "tad_common.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize CSAP IDs database.
 */
extern void csap_id_init(void);

/**
 * Destroy CSAP IDs database.
 */
extern void csap_id_destroy(void);


/**
 * Allocate a new CSAP ID and associate it with specified pointer.
 *
 * @param ptr       Associated pointer (must be not equal to NULL)
 *
 * @return A new CSAP ID or CSAP_INVALID_HANDLE
 */
extern csap_handle_t csap_id_new(void *ptr);

/**
 * Get pointer associated with CSAP ID.
 *
 * @param csap_id   CSAP ID
 *
 * @return Associated pointer or NULL
 */
extern void *csap_id_get(csap_handle_t csap_id);

/**
 * Forget CSAP ID.
 *
 * @param csap_id   CSAP ID
 *
 * @return Associated pointer or NULL
 */
extern void *csap_id_delete(csap_handle_t csap_id);


/**
 * Prototype of callback function for CSAP IDs enumeration.
 *
 * @param csap_id   CSAP ID
 * @param ptr       Associated pointer
 * @param opaque    User opaque data
 *
 * @retval TRUE     Continue
 * @retval FALSE    Break
 */
typedef void (*csap_id_enum_cb)(csap_handle_t csap_id, void *ptr,
                                void *opaque);

/**
 * Enumerate all known CSAP IDs.
 *
 * @param cb        Function to be called for each CSAP ID
 * @param opaque    User opaque data
 *
 * It is allowed to delete enumerated item from callback.
 */
extern void csap_id_enum(csap_id_enum_cb cb, void *opaque);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAD_CSAP_ID_H__ */
