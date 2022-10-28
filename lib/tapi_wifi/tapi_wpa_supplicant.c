/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Control WPA supplicant
 *
 * Test API to control WPA supplicant tool.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "tapi_wpa_supplicant.h"
#include "conf_api.h"
#include "logger_api.h"
#include "te_enum.h"
#include "tapi_mem.h"

/* Path to supplicant instance in the Configurator (format string) */
#define CFG_SUPPLICANT_PATH_FMT "/agent:%s/interface:%s/supplicant:"

/* See description in tapi_wpa_supplicant.h */
void
tapi_wpa_supplicant_security_init_simple(
                    tapi_wifi_policy policy,
                    const char *passphrase,
                    tapi_wpa_supplicant_security *security)
{
    tapi_wifi_key_mgmt key_mgmt = TAPI_WIFI_KEY_MGMT_UNDEF;
    tapi_wifi_cipher pairwise_cipher = TAPI_WIFI_CIPHER_UNDEF;
    tapi_wifi_cipher group_cipher = TAPI_WIFI_CIPHER_UNDEF;
    tapi_wifi_pmf pmf = TAPI_WIFI_PMF_UNDEF;
    tapi_wifi_pmf ieee80211w = TAPI_WIFI_PMF_UNDEF;

    switch (policy)
    {
        case TAPI_WIFI_POLICY_UNDEF:
            /* All settings are default */
            break;

        case TAPI_WIFI_POLICY_NONE:
            key_mgmt = TAPI_WIFI_KEY_MGMT_NONE;
            break;

        case TAPI_WIFI_POLICY_WEP:
            ERROR("%s(): WEP security protocol is not supported", __func__);
            assert(policy != TAPI_WIFI_POLICY_WEP);
            break;

        case TAPI_WIFI_POLICY_WPA:
            key_mgmt = TAPI_WIFI_KEY_MGMT_WPA_PSK;
            pmf = TAPI_WIFI_PMF_DISABLED;
            ieee80211w = TAPI_WIFI_PMF_DISABLED;
            break;

        case TAPI_WIFI_POLICY_WPA2:
            key_mgmt = TAPI_WIFI_KEY_MGMT_WPA_PSK;
            pmf = TAPI_WIFI_PMF_DISABLED;
            ieee80211w = TAPI_WIFI_PMF_DISABLED;
            break;

        case TAPI_WIFI_POLICY_WPA3:
            key_mgmt = TAPI_WIFI_KEY_MGMT_SAE;
            pmf = TAPI_WIFI_PMF_REQUIRED;
            ieee80211w = TAPI_WIFI_PMF_REQUIRED;
            break;

        case TAPI_WIFI_POLICY_WPA_WPA2:
            key_mgmt = TAPI_WIFI_KEY_MGMT_WPA_PSK;
            pmf = TAPI_WIFI_PMF_DISABLED;
            ieee80211w = TAPI_WIFI_PMF_DISABLED;
            break;

        case TAPI_WIFI_POLICY_WPA2_WPA3:
            key_mgmt = TAPI_WIFI_KEY_MGMT_WPA_PSK_SAE;
            pmf = TAPI_WIFI_PMF_ENABLED;
            ieee80211w = TAPI_WIFI_PMF_ENABLED;
            break;
    }

    tapi_wpa_supplicant_security_init(policy, key_mgmt,
                                      pairwise_cipher, group_cipher,
                                      pmf, ieee80211w,
                                      passphrase, security);
}

/* See description in tapi_wpa_supplicant.h */
void
tapi_wpa_supplicant_security_init(
                    tapi_wifi_policy policy,
                    tapi_wifi_key_mgmt key_mgmt,
                    tapi_wifi_cipher pairwise_cipher,
                    tapi_wifi_cipher group_cipher,
                    tapi_wifi_pmf pmf,
                    tapi_wifi_pmf ieee80211w,
                    const char *passphrase,
                    tapi_wpa_supplicant_security *security)
{
    assert(security != NULL);

    security->policy = policy;
    security->key_mgmt = key_mgmt;
    security->pairwise_cipher = pairwise_cipher;
    security->group_cipher = group_cipher;
    security->pmf = pmf;
    security->ieee80211w = ieee80211w;
    if (passphrase == NULL)
        security->passphrase = NULL;
    else
        security->passphrase = tapi_strdup(passphrase);
}

/* See description in tapi_wpa_supplicant.h */
void
tapi_wpa_supplicant_security_clone(
                    tapi_wpa_supplicant_security *to,
                    const tapi_wpa_supplicant_security *from)
{
    assert(from != NULL);
    tapi_wpa_supplicant_security_init(from->policy, from->key_mgmt,
            from->pairwise_cipher, from->group_cipher,
            from->pmf, from->ieee80211w, from->passphrase, to);
}

/* See description in tapi_wpa_supplicant.h */
void
tapi_wpa_supplicant_security_free(
                    tapi_wpa_supplicant_security *security)
{
    if (security == NULL)
        return;

    free(security->passphrase);
}

static const char *
security_policy2val(tapi_wifi_policy policy)
{
    static const te_enum_map mapping[] = {
        {.name = "", .value = TAPI_WIFI_POLICY_UNDEF},
        {.name = "", .value = TAPI_WIFI_POLICY_NONE},
        {.name = "", .value = TAPI_WIFI_POLICY_WEP},
        {.name = "WPA", .value = TAPI_WIFI_POLICY_WPA},
        {.name = "WPA2", .value = TAPI_WIFI_POLICY_WPA2},
        {.name = "RSN", .value = TAPI_WIFI_POLICY_WPA3},
        {.name = "WPA WPA2", .value = TAPI_WIFI_POLICY_WPA_WPA2},
        {.name = "RSN", .value = TAPI_WIFI_POLICY_WPA2_WPA3},
        TE_ENUM_MAP_END
    };

    return te_enum_map_from_value(mapping, policy);
}

static const char *
security_key_mgmt2val(tapi_wifi_key_mgmt key_mgmt)
{
    static const te_enum_map mapping[] = {
        {.name = "", .value = TAPI_WIFI_KEY_MGMT_UNDEF},
        {.name = "NONE", .value = TAPI_WIFI_KEY_MGMT_NONE},
        {.name = "WPA-PSK", .value = TAPI_WIFI_KEY_MGMT_WPA_PSK},
        {.name = "SAE", .value = TAPI_WIFI_KEY_MGMT_SAE},
        {.name = "WPA-PSK SAE", .value = TAPI_WIFI_KEY_MGMT_WPA_PSK_SAE},
        TE_ENUM_MAP_END
    };

    return te_enum_map_from_value(mapping, key_mgmt);
}

static const char *
security_cipher2val(tapi_wifi_cipher cipher)
{
    static const te_enum_map mapping[] = {
        {.name = "", .value = TAPI_WIFI_CIPHER_UNDEF},
        {.name = "CCMP", .value = TAPI_WIFI_CIPHER_CCMP},
        {.name = "TKIP", .value = TAPI_WIFI_CIPHER_TKIP},
        {.name = "CCMP TKIP", .value = TAPI_WIFI_CIPHER_CCMP_TKIP},
        TE_ENUM_MAP_END
    };

    return te_enum_map_from_value(mapping, cipher);
}

static int
security_pmf2val(tapi_wifi_pmf pmf)
{
#define UNKNOWN_PMF (-1)

    static const te_enum_trn translation[] = {
        {.from = TAPI_WIFI_PMF_UNDEF, .to = 0},
        {.from = TAPI_WIFI_PMF_DISABLED, .to = 0},
        {.from = TAPI_WIFI_PMF_ENABLED, .to = 1},
        {.from = TAPI_WIFI_PMF_REQUIRED, .to = 2},
        TE_ENUM_TRN_END
    };
    int mapped = te_enum_translate(translation, pmf, FALSE, UNKNOWN_PMF);

    assert(mapped != UNKNOWN_PMF);
    return mapped;

#undef UNKNOWN_PMF
}

/*
 * Reset WPA supplicant security settings
 *
 * @param ta            Test agent name the supplicant is configured on
 * @param ifname        Wi-Fi network interface the supplicant uses
 *
 * @return Status code
 */
static te_errno
wpa_supplicant_reset_security(const char *ta, const char *ifname)
{
    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/identity:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/cur_method:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/proto:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/key_mgmt:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(INT32, 0),
                            CFG_SUPPLICANT_PATH_FMT "/pmf:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(INT32, 0),
                            CFG_SUPPLICANT_PATH_FMT "/ieee80211w:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/wep_key0:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/wep_key1:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/wep_key2:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/wep_key3:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/wep_tx_keyidx:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/group:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/pairwise:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/psk:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/auth_alg:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/eap-tls:/root_cert:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/eap-tls:/key_passwd:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/eap-tls:/key:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/eap-tls:/cert:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/eap-md5:/username:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/eap-md5:/passwd:",
                            ta, ifname));

    return 0;
}

/*
 * Configure WPA supplicant security settings
 *
 * @param ta            Test agent name the supplicant is configured on
 * @param ifname        Wi-Fi network interface the supplicant uses
 * @param security      Wi-Fi security settings, may be @c NULL to reset them
 *
 * @return Status code
 */
static te_errno
wpa_supplicant_configure_security(
                        const char *ta, const char *ifname,
                        tapi_wpa_supplicant_security *security)
{
    if (security == NULL)
        return wpa_supplicant_reset_security(ta, ifname);

    if (security->policy == TAPI_WIFI_POLICY_WEP)
    {
        ERROR("%s(): WEP security protocol is not supported", __func__);
        return TE_RC(TE_TAPI, TE_EPROTONOSUPPORT);
    }

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CVT_STRING,
                        security_policy2val(security->policy),
                        CFG_SUPPLICANT_PATH_FMT "/proto:", ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CVT_STRING,
                        security_key_mgmt2val(security->key_mgmt),
                        CFG_SUPPLICANT_PATH_FMT "/key_mgmt:", ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CVT_STRING,
                        security_cipher2val(security->group_cipher),
                        CFG_SUPPLICANT_PATH_FMT "/group:", ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CVT_STRING,
                        security_cipher2val(security->pairwise_cipher),
                        CFG_SUPPLICANT_PATH_FMT "/pairwise:", ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CVT_STRING,
                        (security->passphrase != NULL)
                            ? security->passphrase : "",
                        CFG_SUPPLICANT_PATH_FMT "/psk:", ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(INT32,
                        security_pmf2val(security->pmf)),
                        CFG_SUPPLICANT_PATH_FMT "/pmf:", ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(INT32,
                        security_pmf2val(security->ieee80211w)),
                        CFG_SUPPLICANT_PATH_FMT "/ieee80211w:", ta, ifname));

    return 0;
}

/* See description in tapi_wpa_supplicant.h */
te_errno
tapi_wpa_supplicant_configure(
                        const char *ta, const char *ifname,
                        const uint8_t *bssid, const char *ssid,
                        tapi_wpa_supplicant_security *security,
                        te_bool scan_ssid,
                        const char *opts_fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, opts_fmt);
    rc = tapi_wpa_supplicant_configure_va(ta, ifname, bssid, ssid, security,
                                          scan_ssid, opts_fmt, ap);
    va_end(ap);
    return rc;
}

/* See description in tapi_wpa_supplicant.h */
te_errno
tapi_wpa_supplicant_configure_va(
                        const char *ta, const char *ifname,
                        const uint8_t *bssid, const char *ssid,
                        tapi_wpa_supplicant_security *security,
                        te_bool scan_ssid,
                        const char *opts_fmt, va_list ap)
{
    te_string optstr = TE_STRING_INIT;
    te_string bss = TE_STRING_INIT_STATIC(18);
    va_list ap_copy;

    CHECK_NZ_RETURN(cfg_set_instance_fmt(
                        CVT_STRING, (ssid != NULL) ? ssid : "",
                        CFG_SUPPLICANT_PATH_FMT "/network:", ta, ifname));

    if (bssid != NULL)
    {
        CHECK_NZ_RETURN(te_string_append(&bss, TE_PRINTF_MAC_FMT,
                                         TE_PRINTF_MAC_VAL(bssid)));
    }

    CHECK_NZ_RETURN(cfg_set_instance_fmt(
                        CVT_STRING, (bss.ptr != NULL) ? bss.ptr : "",
                        CFG_SUPPLICANT_PATH_FMT "/bssid:", ta, ifname));

    CHECK_NZ_RETURN(wpa_supplicant_configure_security(ta, ifname, security));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(
                        CVT_STRING, (scan_ssid ? "1" : ""),
                        CFG_SUPPLICANT_PATH_FMT "/scan_ssid:", ta, ifname));

    va_copy(ap_copy, ap);
    CHECK_NZ_RETURN(te_string_append_va(&optstr, opts_fmt, ap_copy));
    va_end(ap_copy);

    CHECK_NZ_RETURN(cfg_set_instance_fmt(
                        CFG_VAL(STRING, optstr.ptr),
                        CFG_SUPPLICANT_PATH_FMT "/optstr:", ta, ifname));

    CHECK_NZ_RETURN(cfg_commit_fmt(CFG_SUPPLICANT_PATH_FMT, ta, ifname));

    te_string_free(&optstr);

    return 0;
}

/* See description in tapi_wpa_supplicant.h */
te_errno
tapi_wpa_supplicant_reset(const char *ta, const char *ifname)
{
    CHECK_NZ_RETURN(tapi_wpa_supplicant_stop(ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/scan_ssid:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/optstr:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/network:",
                            ta, ifname));

    CHECK_NZ_RETURN(cfg_set_instance_fmt(CFG_VAL(STRING, ""),
                            CFG_SUPPLICANT_PATH_FMT "/bssid:",
                            ta, ifname));

    CHECK_NZ_RETURN(wpa_supplicant_reset_security(ta, ifname));

    CHECK_NZ_RETURN(cfg_commit_fmt(CFG_SUPPLICANT_PATH_FMT, ta, ifname));

    return 0;
}

/* See description in tapi_wpa_supplicant.h */
te_errno
tapi_wpa_supplicant_start(const char *ta, const char *ifname)
{
    return cfg_set_instance_fmt(CFG_VAL(INT32, 1), CFG_SUPPLICANT_PATH_FMT,
                                ta, ifname);
}

/* See description in tapi_wpa_supplicant.h */
te_errno
tapi_wpa_supplicant_stop(const char *ta, const char *ifname)
{
    return cfg_set_instance_fmt(CFG_VAL(INT32, 0), CFG_SUPPLICANT_PATH_FMT,
                                ta, ifname);
}

/* See description in tapi_wpa_supplicant.h */
te_errno
tapi_wpa_supplicant_get_status(const char *ta, const char *ifname,
                               tapi_wpa_supplicant_status *status)
{
#define UNKNOWN_STATUS (-1)

    static const te_enum_map mapping[] = {
        {.name = "DISCONNECTED", .value = TAPI_WPA_SUPP_DISCONNECTED},
        {.name = "INTERFACE DISABLED", .value = TAPI_WPA_SUPP_INTERFACE_DISABLED},
        {.name = "INACTIVE", .value = TAPI_WPA_SUPP_INACTIVE},
        {.name = "SCANNING", .value = TAPI_WPA_SUPP_SCANNING},
        {.name = "AUTHENTICATING", .value = TAPI_WPA_SUPP_AUTHENTICATING},
        {.name = "ASSOCIATING", .value = TAPI_WPA_SUPP_ASSOCIATING},
        {.name = "ASSOCIATED", .value = TAPI_WPA_SUPP_ASSOCIATED},
        {.name = "4WAY HANDSHAKE", .value = TAPI_WPA_SUPP_4WAY_HANDSHAKE},
        {.name = "GROUP HANDSHAKE", .value = TAPI_WPA_SUPP_GROUP_HANDSHAKE},
        {.name = "COMPLETED", .value = TAPI_WPA_SUPP_COMPLETED},
        TE_ENUM_MAP_END
    };
    char *strval = NULL;
    int mapped;

    CHECK_NZ_RETURN(cfg_get_instance_string_fmt(&strval,
                            CFG_SUPPLICANT_PATH_FMT "/status:",
                            ta, ifname));

    mapped = te_enum_map_from_str(mapping, strval, UNKNOWN_STATUS);
    if (mapped == UNKNOWN_STATUS)
    {
        ERROR("%s(): unknown status '%s', this function requires an update",
              __func__, strval);
        free(strval);
        return TE_RC(TE_TAPI, TE_EUNKNOWN);
    }

    *status = (tapi_wpa_supplicant_status)mapped;

    free(strval);

    return 0;

#undef UNKNOWN_STATUS
}
