/** @file
 * @brief API to operate the time
 *
 * @defgroup te_tools_te_time Date, time
 * @ingroup te_tools
 * @{
 *
 * Functions to operate the date and time.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#ifndef __TE_TIME_H__
#define __TE_TIME_H__


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get string representation of the current date.
 *
 * @note Return value should be freed with free(3) when it is no longer needed.
 *
 * @return Current date, or @c NULL in case of failure.
 */
extern char *te_time_current_date2str(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TIME_H__ */
/**@} <!-- END te_tools_te_time --> */
