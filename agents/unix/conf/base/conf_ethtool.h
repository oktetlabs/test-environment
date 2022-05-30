/** @file
 * @brief Unix Test Agent
 *
 * Common API for SIOCETHTOOL usage in Unix TA configuration
 *
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_AGENTS_UNIX_CONF_BASE_CONF_ETHTOOL_H_
#define __TE_AGENTS_UNIX_CONF_BASE_CONF_ETHTOOL_H_

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"

#ifdef HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

/** Ethtool command type */
typedef enum ta_ethtool_cmd {
    TA_ETHTOOL_COALESCE,      /**< ETHTOOL_[G|S]COALESCE */
    TA_ETHTOOL_PAUSEPARAM,    /**< ETHTOOL_[G|S]PAUSEPARAM */
    TA_ETHTOOL_LINKSETTINGS,  /**< ETHTOOL_[G|S]LINKSETTINGS if supported,
                                   or ETHTOOL_[G|S]SET otherwise */
} ta_ethtool_cmd;

/**
 * Maximum number of 32bit words every link mode mask can occupy.
 */
#define TA_ETHTOOL_LMODE_MASK_WORDS_MAX 10

/**
 * Number of ethtool link mode bitmasks
 * (supported, advertised and lp_advertised).
 */
#define TA_ETHTOOL_LMODE_MASKS 3

#ifdef ETHTOOL_GLINKSETTINGS
/**
 * Link settings when using ETHTOOL_GLINKSETTINGS and
 * ETHTOOL_SLINKSETTINGS.
 */
typedef struct ta_ethtool_link_settings {
    struct ethtool_link_settings fields; /**< Structure storing link
                                              settings */
    /**
     * Link mode masks which go immediately after the settings structure
     * and whose exact length is learned when calling ioctl()
     * the first time.
     */
    uint32_t link_mode_masks[TA_ETHTOOL_LMODE_MASKS *
                             TA_ETHTOOL_LMODE_MASK_WORDS_MAX];
} ta_ethtool_link_settings;
#endif

/** Link settings data */
typedef union ta_ethtool_lsets_data {
    struct ethtool_cmd xset; /**< Settings obtained/set via
                                  deprecated ETHTOOL_GSET and
                                  ETHTOOL_SSET */
#ifdef ETHTOOL_GLINKSETTINGS
    ta_ethtool_link_settings xlinksettings;  /**< Settings obtained/set via
                                                  ETHTOOL_GLINKSETTINGS and
                                                  ETHTOOL_SLINKSETTINGS */
#endif
} ta_ethtool_lsets_data;

/** Generic structure to store link settings */
typedef struct ta_ethtool_lsets {
    ta_ethtool_lsets_data sets; /**< Link settings */
    te_bool use_xlinksettings;  /**< If @c TRUE, link settings were obtained
                                     via ETHTOOL_GLINKSETTINGS, otherwise
                                     ETHTOOL_GSET was used */
} ta_ethtool_lsets;

/** Link settings field IDs */
typedef enum ta_ethtool_lsets_field {
    TA_ETHTOOL_LSETS_AUTONEG,   /**< Autonegotiation */
    TA_ETHTOOL_LSETS_SPEED,     /**< Speed */
    TA_ETHTOOL_LSETS_DUPLEX,    /**< Duplex */
} ta_ethtool_lsets_field;

/**
 * Get value of link settings field.
 *
 * @param lsets     Pointer to the structure with link settings
 * @param field     Field ID
 * @param value     Where to save requested value
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lsets_field_get(ta_ethtool_lsets *lsets,
                                           ta_ethtool_lsets_field field,
                                           unsigned int *value);

/**
 * Set value of link settings field.
 *
 * @param lsets     Pointer to the structure with link settings
 * @param field     Field ID
 * @param value     Value to set
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lsets_field_set(ta_ethtool_lsets *lsets,
                                           ta_ethtool_lsets_field field,
                                           unsigned int value);

/**
 * Ethtool command independent IDs for all known link modes.
 *
 * @note When adding a new link mode, you also need to update
 *       lists in agent/unix/meson.build, agent/unix/configure.ac
 *       and conf_ethtool.c.
 */
typedef enum ta_ethtool_link_mode {
    TA_ETHTOOL_LINK_MODE_10baseT_Half,
    TA_ETHTOOL_LINK_MODE_10baseT_Full,
    TA_ETHTOOL_LINK_MODE_100baseT_Half,
    TA_ETHTOOL_LINK_MODE_100baseT_Full,
    TA_ETHTOOL_LINK_MODE_1000baseT_Half,
    TA_ETHTOOL_LINK_MODE_1000baseT_Full,
    TA_ETHTOOL_LINK_MODE_Autoneg,
    TA_ETHTOOL_LINK_MODE_TP,
    TA_ETHTOOL_LINK_MODE_AUI,
    TA_ETHTOOL_LINK_MODE_MII,
    TA_ETHTOOL_LINK_MODE_FIBRE,
    TA_ETHTOOL_LINK_MODE_BNC,
    TA_ETHTOOL_LINK_MODE_10000baseT_Full,
    TA_ETHTOOL_LINK_MODE_Pause,
    TA_ETHTOOL_LINK_MODE_Asym_Pause,
    TA_ETHTOOL_LINK_MODE_2500baseX_Full,
    TA_ETHTOOL_LINK_MODE_Backplane,
    TA_ETHTOOL_LINK_MODE_1000baseKX_Full,
    TA_ETHTOOL_LINK_MODE_10000baseKX4_Full,
    TA_ETHTOOL_LINK_MODE_10000baseKR_Full,
    TA_ETHTOOL_LINK_MODE_10000baseR_FEC,
    TA_ETHTOOL_LINK_MODE_20000baseMLD2_Full,
    TA_ETHTOOL_LINK_MODE_20000baseKR2_Full,
    TA_ETHTOOL_LINK_MODE_40000baseKR4_Full,
    TA_ETHTOOL_LINK_MODE_40000baseCR4_Full,
    TA_ETHTOOL_LINK_MODE_40000baseSR4_Full,
    TA_ETHTOOL_LINK_MODE_40000baseLR4_Full,
    TA_ETHTOOL_LINK_MODE_56000baseKR4_Full,
    TA_ETHTOOL_LINK_MODE_56000baseCR4_Full,
    TA_ETHTOOL_LINK_MODE_56000baseSR4_Full,
    TA_ETHTOOL_LINK_MODE_56000baseLR4_Full,
    TA_ETHTOOL_LINK_MODE_25000baseCR_Full,
    TA_ETHTOOL_LINK_MODE_25000baseKR_Full,
    TA_ETHTOOL_LINK_MODE_25000baseSR_Full,
    TA_ETHTOOL_LINK_MODE_50000baseCR2_Full,
    TA_ETHTOOL_LINK_MODE_50000baseKR2_Full,
    TA_ETHTOOL_LINK_MODE_100000baseKR4_Full,
    TA_ETHTOOL_LINK_MODE_100000baseSR4_Full,
    TA_ETHTOOL_LINK_MODE_100000baseCR4_Full,
    TA_ETHTOOL_LINK_MODE_100000baseLR4_ER4_Full,
    TA_ETHTOOL_LINK_MODE_50000baseSR2_Full,
    TA_ETHTOOL_LINK_MODE_1000baseX_Full,
    TA_ETHTOOL_LINK_MODE_10000baseCR_Full,
    TA_ETHTOOL_LINK_MODE_10000baseSR_Full,
    TA_ETHTOOL_LINK_MODE_10000baseLR_Full,
    TA_ETHTOOL_LINK_MODE_10000baseLRM_Full,
    TA_ETHTOOL_LINK_MODE_10000baseER_Full,
    TA_ETHTOOL_LINK_MODE_2500baseT_Full,
    TA_ETHTOOL_LINK_MODE_5000baseT_Full,
    TA_ETHTOOL_LINK_MODE_FEC_NONE,
    TA_ETHTOOL_LINK_MODE_FEC_RS,
    TA_ETHTOOL_LINK_MODE_FEC_BASER,
    TA_ETHTOOL_LINK_MODE_50000baseKR_Full,
    TA_ETHTOOL_LINK_MODE_50000baseSR_Full,
    TA_ETHTOOL_LINK_MODE_50000baseCR_Full,
    TA_ETHTOOL_LINK_MODE_50000baseLR_ER_FR_Full,
    TA_ETHTOOL_LINK_MODE_50000baseDR_Full,
    TA_ETHTOOL_LINK_MODE_100000baseKR2_Full,
    TA_ETHTOOL_LINK_MODE_100000baseSR2_Full,
    TA_ETHTOOL_LINK_MODE_100000baseCR2_Full,
    TA_ETHTOOL_LINK_MODE_100000baseLR2_ER2_FR2_Full,
    TA_ETHTOOL_LINK_MODE_100000baseDR2_Full,
    TA_ETHTOOL_LINK_MODE_200000baseKR4_Full,
    TA_ETHTOOL_LINK_MODE_200000baseSR4_Full,
    TA_ETHTOOL_LINK_MODE_200000baseLR4_ER4_FR4_Full,
    TA_ETHTOOL_LINK_MODE_200000baseDR4_Full,
    TA_ETHTOOL_LINK_MODE_200000baseCR4_Full,
    TA_ETHTOOL_LINK_MODE_100baseT1_Full,
    TA_ETHTOOL_LINK_MODE_1000baseT1_Full,
    TA_ETHTOOL_LINK_MODE_400000baseKR8_Full,
    TA_ETHTOOL_LINK_MODE_400000baseSR8_Full,
    TA_ETHTOOL_LINK_MODE_400000baseLR8_ER8_FR8_Full,
    TA_ETHTOOL_LINK_MODE_400000baseDR8_Full,
    TA_ETHTOOL_LINK_MODE_400000baseCR8_Full,
    TA_ETHTOOL_LINK_MODE_FEC_LLRS,
    TA_ETHTOOL_LINK_MODE_100000baseKR_Full,
    TA_ETHTOOL_LINK_MODE_100000baseSR_Full,
    TA_ETHTOOL_LINK_MODE_100000baseLR_ER_FR_Full,
    TA_ETHTOOL_LINK_MODE_100000baseCR_Full,
    TA_ETHTOOL_LINK_MODE_100000baseDR_Full,
    TA_ETHTOOL_LINK_MODE_200000baseKR2_Full,
    TA_ETHTOOL_LINK_MODE_200000baseSR2_Full,
    TA_ETHTOOL_LINK_MODE_200000baseLR2_ER2_FR2_Full,
    TA_ETHTOOL_LINK_MODE_200000baseDR2_Full,
    TA_ETHTOOL_LINK_MODE_200000baseCR2_Full,
    TA_ETHTOOL_LINK_MODE_400000baseKR4_Full,
    TA_ETHTOOL_LINK_MODE_400000baseSR4_Full,
    TA_ETHTOOL_LINK_MODE_400000baseLR4_ER4_FR4_Full,
    TA_ETHTOOL_LINK_MODE_400000baseDR4_Full,
    TA_ETHTOOL_LINK_MODE_400000baseCR4_Full,
    TA_ETHTOOL_LINK_MODE_100baseFX_Half,
    TA_ETHTOOL_LINK_MODE_100baseFX_Full,
} ta_ethtool_link_mode;

/**
 * Get name of a link mode.
 *
 * @param mode      Link mode ID
 *
 * @return Pointer to string constant.
 */
extern const char *ta_ethtool_lmode_name(ta_ethtool_link_mode mode);

/**
 * Parse string representation of a link mode (i.e. its name).
 *
 * @param name      Link mode name
 * @param mode      Where to save parsed ID
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_parse(const char *name,
                                       ta_ethtool_link_mode *mode);

/**
 * Check whether a given link mode is reported as supported.
 *
 * @param lsets       Pointer to the link settings structure
 * @param mode        Link mode to check
 * @param supported   Will be set to @c TRUE if link mode is supported,
 *                    to @c FALSE otherwise
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_supported(ta_ethtool_lsets *lsets,
                                           ta_ethtool_link_mode mode,
                                           te_bool *supported);

/**
 * Check whether a given link mode is reported as advertised.
 *
 * @param lsets       Pointer to the link settings structure
 * @param mode        Link mode to check
 * @param advertised  Will be set to @c TRUE if link mode is advertised,
 *                    to @c FALSE otherwise
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_advertised(ta_ethtool_lsets *lsets,
                                            ta_ethtool_link_mode mode,
                                            te_bool *advertised);

/**
 * Check whether a given link mode is reported as advertised by link
 * partner.
 *
 * @param lsets           Pointer to the link settings structure
 * @param mode            Link mode to check
 * @param lp_advertised   Will be set to @c TRUE if link mode is advertised
 *                        by link partner, to @c FALSE otherwise
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_lp_advertised(ta_ethtool_lsets *lsets,
                                               ta_ethtool_link_mode mode,
                                               te_bool *lp_advertised);

/**
 * Set a given link mode as advertised or not advertised.
 *
 * @param lsets       Pointer to the link settings structure
 * @param mode        Link mode
 * @param enable      Whether link mode should be advertised or not
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_advertise(ta_ethtool_lsets *lsets,
                                           ta_ethtool_link_mode mode,
                                           te_bool enable);

/**
 * Call @c SIOCETHTOOL ioctl() to get or set some values.
 *
 * @param if_name     Name of the interface
 * @param cmd         Ethtool command number
 * @param value       Pointer to Ethtool command structure
 *
 * @return Status code.
 */
extern te_errno call_ethtool_ioctl(const char *if_name, int cmd,
                                   void *value);

/**
 * Get a pointer to Ethtool command structure to work with.
 * Structure fields are filled with help of related Ethtool get command
 * when it is requested the first time (for a given @p gid).
 *
 * @param if_name         Interface name
 * @param gid             Group ID
 * @param cmd             Ethtool command ID
 * @param ptr_out         Here requested pointer will be saved
 *
 * @return Status code.
 */
extern te_errno get_ethtool_value(const char *if_name, unsigned int gid,
                                  ta_ethtool_cmd cmd, void **ptr_out);

/**
 * Commit configuration changes via @c SIOCETHTOOL.
 * It is assumed that changes made by one or more set operations are
 * saved in an object stored for a given interface which can be
 * retrieved with ta_obj_find().
 *
 * @param if_name         Interface name
 * @param gid             Request group ID
 * @param cmd             Ethtool command ID telling what to commit
 *
 * @return Status code.
 */
extern te_errno commit_ethtool_value(const char *if_name,
                                     unsigned int gid,
                                     ta_ethtool_cmd cmd);

/**
 * Compose list of link mode names which are supported by our interface
 * or advertised by link partner.
 *
 * @param lsets         Structure with link settings
 * @param link_partner  If @c TRUE, get list of link mode names advertised
 *                      by link partner, otherwise - list of link mode
 *                      names supported by the interface
 * @param list_str      Where to append names of link modes
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_list_names(ta_ethtool_lsets *lsets,
                                            te_bool link_partner,
                                            te_string *list_str);

/** Set of strings returned by ETHTOOL_GSTRINGS */
typedef struct ta_ethtool_strings {
    size_t num; /**< Number of strings */
    char strings[][ETH_GSTRING_LEN + 1]; /**< Array of null-terminated
                                              strings */
} ta_ethtool_strings;

/**
 * Get set of strings (like set of RSS hash function names or
 * Ethernet features).
 *
 * @param gid         Request group ID
 * @param if_name     Interface name
 * @param set_id      String set ID
 * @param strs        Where to save pointer to filled ta_ethtool_strings
 *                    (caller should not release it)
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_get_strings(unsigned int gid,
                                       const char *if_name,
                                       unsigned int set_id,
                                       const ta_ethtool_strings **strs);

#ifdef ETHTOOL_GRSSH
/**
 * Get RX flow hash configuration via ETHTOOL_GRSSH.
 * This may return pointer to cached structure if it was already
 * obtained for the current @p gid. Changes made to the obtained
 * structure should be committed via ta_ethtool_commit_rssh().
 *
 * @param gid           Request group ID
 * @param if_name       Interface name
 * @param rss_context   RSS context ID
 * @param rxfh          Where to save pointer to obtained structure
 *                      (caller should not release it)
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_get_rssh(unsigned int gid, const char *if_name,
                                    unsigned int rss_context,
                                    struct ethtool_rxfh **rxfh);

#ifdef ETHTOOL_SRSSH
/**
 * Commit changes to RX flow hash configuration via ETHTOOL_SRSSH.
 *
 * @param gid           Request group ID
 * @param if_name       Interface name
 * @param rss_context   RSS context ID
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_commit_rssh(unsigned int gid,
                                       const char *if_name,
                                       unsigned int rss_context);
#endif

#endif

#endif /* !__TE_AGENTS_UNIX_CONF_BASE_CONF_ETHTOOL_H_ */
