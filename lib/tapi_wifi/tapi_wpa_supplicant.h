/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Control WPA supplicant
 *
 * @defgroup tapi_wifi_wpa_supplicant Control WPA supplicant
 * @ingroup tapi_wifi
 * @{
 *
 * Test API to control WPA supplicant tool
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TAPI_WPA_SUPPLICANT_H__
#define __TAPI_WPA_SUPPLICANT_H__

#include "te_defs.h"
#include "te_errno.h"
#include "tapi_wifi_security.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * WPA supplicant security settings.
 */
typedef struct tapi_wpa_supplicant_security {
    /** Wi-Fi security protocol. */
    tapi_wifi_policy policy;
    /** Wi-Fi authenticated key management protocol. */
    tapi_wifi_key_mgmt key_mgmt;
    /** Wi-Fi pairwise (unicast) encryption protocol. */
    tapi_wifi_cipher pairwise_cipher;
    /** Wi-Fi group (broadcast/multicast) encryption protocol. */
    tapi_wifi_cipher group_cipher;
    /**
     * Wi-Fi Protected Management Frame.
     * Global for all networks, can be overridden by @p ieee80211w value.
     */
    tapi_wifi_pmf pmf;
    /**
     * Wi-Fi Protected Management Frame.
     * Per-network setting, overrides global @p pmf value.
     */
    tapi_wifi_pmf ieee80211w;
    /**
     * WPA preshared key passphrase.
     * The real PSK will be generated using this passphrase and SSID.
     * ASCII passphrase must be between 8 and 63 characters (inclusive).
     */
    char *passphrase;
} tapi_wpa_supplicant_security;

/**
 * Configure WPA supplicant settings. It does not care if the supplicant is
 * already running or not, a user should ensure themselves that the supplicant
 * is inactive before reconfiguring.
 *
 * @param ta            Test agent name the supplicant is configured on.
 * @param ifname        Wi-Fi network interface the supplicant uses.
 * @param bssid         Wi-Fi Access Point's BSSID, may be @c NULL.
 * @param ssid          Wi-Fi Access Point's SSID, may be @c NULL.
 * @param security      Wi-Fi security settings, may be @c NULL to reset them.
 * @param scan_ssid     Enable/disable scanning SSID with specific Probe
 *                      Request frames. This can be enabled to find APs that
 *                      do not accept broadcast SSID or use multiple SSIDs;
 *                      this will add latency to scanning, so enable this
 *                      only when needed.
 * @param opts_fmt      Format string with extra wpa_supplicant options,
 *                      such as "-dd", "-D", etc. Useful to set options
 *                      which cannot be applied with a normal way
 * @param ...           Format string arguments.
 *
 * @return Status code.
 *
 * @sa tapi_wpa_supplicant_configure_va
 */
extern te_errno tapi_wpa_supplicant_configure(
                        const char *ta, const char *ifname,
                        const uint8_t *bssid, const char *ssid,
                        tapi_wpa_supplicant_security *security,
                        te_bool scan_ssid,
                        const char *opts_fmt, ...)
                        __attribute__((format(printf, 7, 8)));

/**
 * Same as tapi_wpa_supplicant_configure() but operates with va_list
 * instead of format string.
 *
 * @sa tapi_wpa_supplicant_configure
 */
extern te_errno tapi_wpa_supplicant_configure_va(
                        const char *ta, const char *ifname,
                        const uint8_t *bssid, const char *ssid,
                        tapi_wpa_supplicant_security *security,
                        te_bool scan_ssid,
                        const char *opts_fmt, va_list ap);

/**
 * Stop WPA supplicant and reset its settings.
 *
 * @param ta            Test agent name the supplicant is configured on.
 * @param ifname        Wi-Fi network interface the supplicant uses.
 *
 * @return Status code.
 */
extern te_errno tapi_wpa_supplicant_reset(
                        const char *ta, const char *ifname);

/**
 * Start WPA supplicant.
 *
 * @param ta            Test agent name the supplicant is started on.
 * @param ifname        Wi-Fi network interface the supplicant uses.
 *
 * @return Status code.
 */
extern te_errno tapi_wpa_supplicant_start(
                        const char *ta, const char *ifname);

/**
 * Stop WPA supplicant.
 *
 * @param ta            Test agent name the supplicant is stopped on.
 * @param ifname        Wi-Fi network interface the supplicant uses.
 *
 * @return Status code.
 */
extern te_errno tapi_wpa_supplicant_stop(
                        const char *ta, const char *ifname);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_WPA_SUPPLICANT_H__ */

/**@} <!-- END tapi_wifi_wpa_supplicant --> */
