/** @file
 * @brief Logging Subsystem Internal Definitions
 *
 * Definitions common for parts of logging subsystem but not required
 * for other TE entities.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_LOGGER_INT_H__
#define __TE_LOGGER_INT_H__

#ifdef __cplusplus
extern "C" {
#endif


/** Convert value to the network order */
#define LGR_16_TO_NET(_val, _res) \
    (((int8_t *)(_res))[0] = (int8_t)(((_val)>>8) & 0xff), \
     ((int8_t *)(_res))[1] = (int8_t)((_val) & 0xff))

#define LGR_32_TO_NET(_val, _res) \
    (((int8_t *)(_res))[0] = (int8_t)(((_val)>>24) & 0xff), \
     ((int8_t *)(_res))[1] = (int8_t)(((_val)>>16) & 0xff), \
     ((int8_t *)(_res))[2] = (int8_t)(((_val)>>8) & 0xff),  \
     ((int8_t *)(_res))[3] = (int8_t)((_val) & 0xff))


#define LGR_16_PUT(val_, buf_) \
    do {                            \
        uint16_t    nfl_ = (val_);  \
                                    \
        LGR_16_TO_NET(nfl_, buf_);  \
        (buf_) += 2;                \
    } while (0)

#if (SIZEOF_TE_LOG_NFL == 2)
#define LGR_NFL_PUT(val_, buf_)     LGR_16_PUT(val_, buf_)
#else
#error LGR_NFL_PUT is not defined
#endif

#define LGR_SRV_SNIFFER_MARK "LGR-SNIFFER_MARK"
#define SNIFFER_MIN_MARK_SIZE 512

/* ==== Test Agent Logger lib definitions */

/*
 * If it's needed override default value to release oldest message
 * and register new one when unused ring buffer resources is absent
 * as following:
 *  #undef LGR_RB_FORCE
 *  #define LGR_RB_FORCE 1
 */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOGGER_INT_H__ */
