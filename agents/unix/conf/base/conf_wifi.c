/** @file
 * @brief Unix Test Agent
 *
 * Unix WiFi configuring support
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Conf WiFi"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "unix_internal.h"

#if HAVE_IWLIB_H
#include <iwlib.h>
#else
#error iwlib.h is an obligatory header for WiFi support
#endif

#ifndef KILO
#define KILO    1e3
#endif

/**
 * Returns socket descriptor that should be used in ioctl() calls
 * for configuring wireless interface attributes
 *
 * @return Socket descriptor, or -errno in case of failure.
 */
static int
wifi_get_skfd()
{
    static int skfd = -1;

    if (skfd == -1)
    {
        /* We'll never close this socket until agent shutdown */
        if ((skfd = iw_sockets_open()) < 0)
        {
            ERROR("Cannot open socket for wireless extension");

            return -(TE_OS_RC(TE_TA_UNIX, errno));
        }
    }

    return skfd;
}

/**
 * Check if socket descriptor is valid
 *
 * @param skfd_ Socket descriptor to be checked
 */
#define WIFI_CHECK_SKFD(skfd_) \
    do {                       \
        if ((skfd_) < 0)       \
            return -(skfd_);   \
    } while (0);

/**
 * Update a configuration item in WiFi card.
 *
 * @param ifname  Wireless interface name
 * @param req     Request type
 * @param wrp     request value
 *
 * @return Status of the operation
 */
static int
wifi_set_item(const char *ifname, int req, struct iwreq *wrp)
{
    int rc;
    int retry = 0;
    int skfd = wifi_get_skfd();

#define RETRY_LIMIT 500
    WIFI_CHECK_SKFD(skfd);

    while ((rc = iw_set_ext(skfd, ifname, req, wrp)) != 0)
    {
        if (errno == EBUSY)
        {
            /* Try again */
            if (++retry < RETRY_LIMIT)
            {
                usleep(50);

                continue;
            }
        }

        rc = TE_OS_RC(TE_TA_UNIX, errno);

        break;
    }
#undef RETRY_LIMIT

    if (retry != 0)
        WARN("%s: The number of retries %d", __FUNCTION__, retry);

    return rc;
}

/**
 * Get a configuration item from WiFi card.
 *
 * @param ifname  Wireless interface name
 * @param req     Request type
 * @param wrp     request value (OUT)
 *
 * @return Status of the operation
 */
static int
wifi_get_item(const char *ifname, int req, struct iwreq *wrp)
{
    int rc;
    int retry = 0;
    int skfd = wifi_get_skfd();

#define RETRY_LIMIT 500
    WIFI_CHECK_SKFD(skfd);

    while ((rc = iw_get_ext(skfd, ifname, req, wrp)) != 0)
    {
        if (errno == EBUSY)
        {
            /* Try again */
            if (++retry < RETRY_LIMIT)
            {
                usleep(50);

                continue;
            }
        }

        rc = TE_OS_RC(TE_TA_UNIX, errno);

        break;
    }
#undef RETRY_LIMIT

    if (retry != 0)
        WARN("%s: The number of retries %d", __FUNCTION__, retry);

    return rc;
}

/**
 * Determine if interface supports wireless extension or not.
 *
 * @param gid     group identifier (unused)
 * @param oid     full parent object instance identifier
 * @param sub_id  ID of the object to be listed (unused)
 * @param list    location for the list pointer
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_list(unsigned int gid, const char *oid,
          const char *sub_id, char **list,
          const char *ifname)
{
    struct iwreq     wrq;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if ((rc = wifi_get_item(ifname, SIOCGIWNAME, &wrq)) != 0)
    {
        /* Interface does not support wireless extension */
        VERB("Interface %s does not support WiFi", ifname);

        *list = strdup("");
    }
    else
        *list = strdup("enabled");

    return 0;
}

/**
 * Get channel number used on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - channel number
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_channel_get(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
    int             rc;
    struct iw_range range;
    struct iwreq    wrq;
    double          freq;
    int             channel;
    int             skfd = wifi_get_skfd();

    UNUSED(gid);
    UNUSED(oid);

    WIFI_CHECK_SKFD(skfd);

    memset(&wrq, 0, sizeof(wrq));

    if (iw_get_range_info(skfd, ifname, &range) < 0)
    {
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    if ((rc = wifi_get_item(ifname, SIOCGIWFREQ, &wrq)) == 0)
    {
        freq = iw_freq2float(&(wrq.u.freq));

        channel = iw_freq_to_channel(freq, &range);

        if (freq < KILO)
        {
            WARN("iw_freq2float() function returns channel, not frequency");

            channel = (int)freq;
        }

        if (channel < 0)
        {
            ERROR("Cannot get current channel "
                  "number on %s interface", ifname);

            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }

        sprintf(value, "%d", channel);
    }
    else
    {
        if (errno == EINVAL)
            /* Not supported, return empty value.*/
            sprintf(value, "");
        else
            return rc;
    }

    return 0;
}

/**
 * Set channel number on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - channel number
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_channel_set(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
    struct iwreq    wrq;
    struct iw_range range;
    int             skfd = wifi_get_skfd();
    double          freq;
    int             channel;

    UNUSED(gid);
    UNUSED(oid);

    WIFI_CHECK_SKFD(skfd);

    if (sscanf(value, "%d", &channel) != 1)
    {
        ERROR("Incorrect format of channel value '%s'", value);

        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (iw_get_range_info(skfd, ifname, &range) < 0)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    if (iw_channel_to_freq(channel, &freq, &range) < 0)
    {
        ERROR("Cannot convert %d channel to an appropriate "
              "frequency value", channel);

        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    iw_float2freq(freq, &(wrq.u.freq));

    return wifi_set_item(ifname, SIOCSIWFREQ, &wrq);
}

/**
 * Get the list of supported channels on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - colon separated list of channels
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_channels_get(unsigned int gid, const char *oid, char *value,
                  const char *ifname)
{
    struct iw_range range;
    int             skfd = wifi_get_skfd();
    int             i;
    double          freq;
    int             channel;

    UNUSED(gid);
    UNUSED(oid);

    WIFI_CHECK_SKFD(skfd);

    if (iw_get_range_info(skfd, ifname, &range) < 0)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    *value = '\0';

    for (i = 0; i < range.num_frequency; i++)
    {
        freq = iw_freq2float(&(range.freq[i]));

        channel = iw_freq_to_channel(freq, &range);

        sprintf(value + strlen(value), "%d%c", channel,
                i == (range.num_frequency - 1) ? '\0' : ':');
    }

    return 0;
}

/**
 * Get AP MAC address the STA is associated with.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - AP MAC address,
 *                00:00:00:00:00:00 in case of STA is not associated
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_ap_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    int          rc;
    struct iwreq wrq;
    int          i;

    UNUSED(gid);
    UNUSED(oid);

    memset(&wrq, 0, sizeof(wrq));

    if ((rc = wifi_get_item(ifname, SIOCGIWAP, &wrq)) != 0)
        return rc;

    for (i = 0; i < (ETHER_ADDR_LEN - 1); i++)
    {
        if (wrq.u.ap_addr.sa_data[i] != wrq.u.ap_addr.sa_data[i + 1])
        {
            sprintf(value, "%02x:%02x:%02x:%02x:%02x:%02x",
                    (uint8_t)wrq.u.ap_addr.sa_data[0],
                    (uint8_t)wrq.u.ap_addr.sa_data[1],
                    (uint8_t)wrq.u.ap_addr.sa_data[2],
                    (uint8_t)wrq.u.ap_addr.sa_data[3],
                    (uint8_t)wrq.u.ap_addr.sa_data[4],
                    (uint8_t)wrq.u.ap_addr.sa_data[5]);

            return 0;
        }
    }

    sprintf(value, "00:00:00:00:00:00");

    return 0;
}

/* Unix Test Agent WiFi configuration subtree */

RCF_PCH_CFG_NODE_RW(node_wifi_channel, "channel", NULL, NULL,
                    wifi_channel_get, wifi_channel_set);

RCF_PCH_CFG_NODE_RO(node_wifi_channels, "channels", NULL,
                    &node_wifi_channel, wifi_channels_get);

RCF_PCH_CFG_NODE_RO(node_wifi_ap, "ap", NULL, &node_wifi_channels,
                    wifi_ap_get);

RCF_PCH_CFG_NODE_COLLECTION(node_wifi, "wifi",
                            &node_wifi_ap, NULL,
                            NULL, NULL,
                            wifi_list, NULL);

/**
 * Initializes ta_unix_conf_wifi support.
 *
 * @return Status code (see te_errno.h)
 */
te_errno
ta_unix_conf_wifi_init()
{
    return rcf_pch_add_node("/agent/interface", &node_wifi);
}
