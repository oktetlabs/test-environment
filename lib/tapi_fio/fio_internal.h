/** @file
 * @brief Auxiliary functions to fio TAPI
 *
 * @defgroup tapi_fio_internal FIO tool internals
 * @ingroup tapi_fio
 * @{
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __FIO_INTERNAL_H__
#define __FIO_INTERNAL_H__

#include "tapi_fio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Starting fio tool.
 *
 * @param app           App context.
 *
 * @return Status code
 */
extern te_errno fio_app_start(tapi_fio_app *app);

/**
 * Stopping fio tool.
 *
 * @param app           App context.
 *
 * @return Status code
 */
extern te_errno fio_app_stop(tapi_fio_app *app);

/**
 * Waiting fio tool.
 *
 * @param app           App context.
 * @param timeout_sec   Timeout in seconds.
 *
 * @return Status code
 */
extern te_errno fio_app_wait(tapi_fio_app *app, int16_t timeout_sec);

/**
 * Receive fio report in JSON format.
 *
 * @param      app          App context.
 * @param[out] reoirt       Fio report
 *
 * @return Status code
 */
extern te_errno fio_app_receive_report(tapi_fio_app *app, char **report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __FIO_INTERNAL_H__ */

/**@} <!-- END tapi_fio_internal --> */
