/** @file
 * @brief FIO Test API for fio tool routine
 *
 * @defgroup tapi_fio_fio Control a FIO tool
 * @ingroup tapi_fio
 * @{
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __FIO_H__
#define __FIO_H__

#include "tapi_fio.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Default fio job name */
#define FIO_DEFAULT_NAME    "default.fio"

/** Initialized methods for fio work */
extern tapi_fio_methods methods;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __FIO_H__ */

/**@} <!-- END tapi_fio_fio --> */
