/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Wi-Fi security
 *
 * @defgroup tapi_wifi_security Wi-Fi security
 * @ingroup tapi_wifi
 * @{
 *
 * Common Wi-Fi security definitions
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TAPI_WIFI_SECURITY_H__
#define __TAPI_WIFI_SECURITY_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Supported Wi-Fi security protocols.
 */
typedef enum tapi_wifi_policy {
    /** Default value for particular tool (as if not set). */
    TAPI_WIFI_POLICY_UNDEF,
    /** No security, open Wi-Fi network. */
    TAPI_WIFI_POLICY_NONE,
    /** Wired Equivalent Privacy. */
    TAPI_WIFI_POLICY_WEP,
    /** Wi-Fi Protected Access. */
    TAPI_WIFI_POLICY_WPA,
    /** Wi-Fi Protected Access 2. */
    TAPI_WIFI_POLICY_WPA2,
    /** Wi-Fi Protected Access 3. */
    TAPI_WIFI_POLICY_WPA3,

    /* Combinations */
    /** Allow both WPA and WPA2. */
    TAPI_WIFI_POLICY_WPA_WPA2,
    /** Allow both WPA2 and WPA3. */
    TAPI_WIFI_POLICY_WPA2_WPA3,
} tapi_wifi_policy;

/**
 * The list of values allowed for parameter of type @ref tapi_wifi_policy.
 */
#define TAPI_WIFI_POLICY_MAPPING_LIST \
    { "NONE",       TAPI_WIFI_POLICY_NONE },        \
    { "WEP",        TAPI_WIFI_POLICY_WEP },         \
    { "WPA",        TAPI_WIFI_POLICY_WPA },         \
    { "WPA2",       TAPI_WIFI_POLICY_WPA2 },        \
    { "WPA3",       TAPI_WIFI_POLICY_WPA3 },        \
    { "WPA_WPA2",   TAPI_WIFI_POLICY_WPA_WPA2 },    \
    { "WPA2_WPA3",  TAPI_WIFI_POLICY_WPA2_WPA3 }

/**
 * Return the value of parameter of type @ref tapi_wifi_policy.
 *
 * @param[in] var_name_     Name of the variable is used to get the value of
 *                          "var_name_" parameter.
 */
#define TEST_WIFI_POLICY_PARAM(var_name_) \
    TEST_ENUM_PARAM(var_name_, TAPI_WIFI_POLICY_MAPPING_LIST)

/**
 * Get the value of parameter of type @ref tapi_wifi_policy.
 *
 * @param[in,out] var_name_     Name of the variable is used to get
 *                              the value of "var_name_" parameter.
 */
#define TEST_GET_WIFI_POLICY_PARAM(var_name_) \
    (var_name_) = TEST_WIFI_POLICY_PARAM(var_name_)

/**
 * Supported Wi-Fi encryption protocols.
 */
typedef enum tapi_wifi_cipher {
    /** Default value for particular tool (as if not set). */
    TAPI_WIFI_CIPHER_UNDEF,
    /** AES in Counter mode with CBC-MAC. */
    TAPI_WIFI_CIPHER_CCMP,
    /** Temporal Key Integrity Protocol. */
    TAPI_WIFI_CIPHER_TKIP,

    /* Combinations */
    /** Allow both TKIP and CCMP. */
    TAPI_WIFI_CIPHER_CCMP_TKIP,
} tapi_wifi_cipher;

/**
 * The list of values allowed for parameter of type @ref tapi_wifi_cipher.
 */
#define TAPI_WIFI_CIPHER_MAPPING_LIST \
    { "CCMP",       TAPI_WIFI_CIPHER_CCMP },        \
    { "TKIP",       TAPI_WIFI_CIPHER_TKIP },        \
    { "CCMP_TKIP",  TAPI_WIFI_CIPHER_CCMP_TKIP }

/**
 * Return the value of parameter of type @ref tapi_wifi_cipher.
 *
 * @param[in] var_name_     Name of the variable is used to get the value of
 *                          "var_name_" parameter (OUT).
 */
#define TEST_WIFI_CIPHER_PARAM(var_name_) \
    TEST_ENUM_PARAM(var_name_, TAPI_WIFI_CIPHER_MAPPING_LIST)

/**
 * Get the value of parameter of type @ref tapi_wifi_cipher.
 *
 * @param[in,out] var_name_     Name of the variable is used to get
 *                              the value of "var_name_" parameter.
 */
#define TEST_GET_WIFI_CIPHER_PARAM(var_name_) \
    (var_name_) = TEST_WIFI_CIPHER_PARAM(var_name_)

/**
 * Supported Wi-Fi authenticated key management protocols.
 */
typedef enum tapi_wifi_key_mgmt {
    /** Default value for particular tool (as if not set). */
    TAPI_WIFI_KEY_MGMT_UNDEF,
    /** WPA is not used; plaintext or static WEP could be used. */
    TAPI_WIFI_KEY_MGMT_NONE,
    /** WPA pre-shared key. */
    TAPI_WIFI_KEY_MGMT_WPA_PSK,
    /**
     * Simultaneous authentication of equals; pre-shared key/password -based
     * authentication with stronger security than WPA-PSK especially when using
     * not that strong password; a.k.a. WPA3-Personal.
     */
    TAPI_WIFI_KEY_MGMT_SAE,

    /* Combinations */
    /** Allow both WPA_PSK and SAE. */
    TAPI_WIFI_KEY_MGMT_WPA_PSK_SAE,
} tapi_wifi_key_mgmt;

/**
 * Supported Wi-Fi Protected Management Frames (PMF).
 */
typedef enum tapi_wifi_pmf {
    /** Default value for particular tool (as if not set). */
    TAPI_WIFI_PMF_UNDEF,
    /** Disabled PMF. */
    TAPI_WIFI_PMF_DISABLED,
    /** Optional PMF, may be used with any value of @ref tapi_wifi_policy. */
    TAPI_WIFI_PMF_ENABLED,
    /** Required PMF (mandatory). */
    TAPI_WIFI_PMF_REQUIRED,
} tapi_wifi_pmf;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_WIFI_SECURITY_H__ */

/**@} <!-- END tapi_wifi_security --> */
