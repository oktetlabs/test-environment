/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Common API for SIOCETHTOOL usage in Unix TA configuration
 *
 *
 * Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_AGENTS_UNIX_CONF_BASE_CONF_ETHTOOL_H_
#define __TE_AGENTS_UNIX_CONF_BASE_CONF_ETHTOOL_H_

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
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
    TA_ETHTOOL_PFLAGS,        /**< ETHTOOL_[G|S]PFLAGS */
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
    bool use_xlinksettings;  /**< If @c true, link settings were obtained
                                     via ETHTOOL_GLINKSETTINGS, otherwise
                                     ETHTOOL_GSET was used */
    bool set_supported;      /**< If @c true, changing link settings is
                                     supported */
} ta_ethtool_lsets;

/** Link settings field IDs */
typedef enum ta_ethtool_lsets_field {
    TA_ETHTOOL_LSETS_AUTONEG,   /**< Autonegotiation */
    TA_ETHTOOL_LSETS_SPEED,     /**< Speed */
    TA_ETHTOOL_LSETS_DUPLEX,    /**< Duplex */
    TA_ETHTOOL_LSETS_PORT,      /**< Physical connector type */
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
 * @param supported   Will be set to @c true if link mode is supported,
 *                    to @c false otherwise
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_supported(ta_ethtool_lsets *lsets,
                                           ta_ethtool_link_mode mode,
                                           bool *supported);

/**
 * Check whether a given link mode is reported as advertised.
 *
 * @param lsets       Pointer to the link settings structure
 * @param mode        Link mode to check
 * @param advertised  Will be set to @c true if link mode is advertised,
 *                    to @c false otherwise
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_advertised(ta_ethtool_lsets *lsets,
                                            ta_ethtool_link_mode mode,
                                            bool *advertised);

/**
 * Check whether a given link mode is reported as advertised by link
 * partner.
 *
 * @param lsets           Pointer to the link settings structure
 * @param mode            Link mode to check
 * @param lp_advertised   Will be set to @c true if link mode is advertised
 *                        by link partner, to @c false otherwise
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_lp_advertised(ta_ethtool_lsets *lsets,
                                               ta_ethtool_link_mode mode,
                                               bool *lp_advertised);

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
                                           bool enable);

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
 * @param link_partner  If @c true, get list of link mode names advertised
 *                      by link partner, otherwise - list of link mode
 *                      names supported by the interface
 * @param list_str      Where to append names of link modes
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_lmode_list_names(ta_ethtool_lsets *lsets,
                                            bool link_partner,
                                            te_string *list_str);

/**
 * Determine best speed/duplex supported by network interface.
 *
 * @param lsets         Structure with link settings
 * @param speed         Where to save best speed
 * @param duplex        Where to save corresponding duplex value
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_get_max_speed(ta_ethtool_lsets *lsets,
                                         unsigned int *speed,
                                         unsigned int *duplex);

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

/*
 * Same as ta_ethtool_get_strings(), but obtained set of strings is
 * returned as a space-separated list suitable for list() method of
 * configuration object.
 *
 * @param gid         Request group ID
 * @param if_name     Interface name
 * @param set_id      String set ID
 * @param list_out    Where to save pointer to the list
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_get_strings_list(unsigned int gid,
                                            const char *if_name,
                                            unsigned int set_id,
                                            char **list_out);

/**
 * Get index of a given string in a set of strings.
 *
 * @param gid         Request group ID
 * @param if_name     Interface name
 * @param set_id      String set ID
 * @param target      Target string
 * @param idx         Where to save index of the target string
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_get_string_idx(unsigned int gid,
                                          const char *if_name,
                                          unsigned int set_id,
                                          const char *target,
                                          unsigned int *idx);

#ifdef ETHTOOL_GRSSH

/** Data associated with ETHTOOL_GRSSH/ETHTOOL_SRSSH */
typedef struct ta_ethtool_rxfh {
    /** Pointer to ethtool structure passed to ioctl(). */
    struct ethtool_rxfh *rxfh;
    /**
     * Should be set to @c true if change of RSS indirection table
     * is required.
     */
    bool indir_change;
    /** If @c true, reset indirection table to default values. */
    bool indir_reset;
    /** Should be set to @c true if change of RSS hash key is required. */
    bool hash_key_change;
} ta_ethtool_rxfh;

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
                                    ta_ethtool_rxfh **rxfh);

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

#ifdef ETHTOOL_GRXCLSRLALL

/** Information about Rx rules */
typedef struct ta_ethtool_rx_cls_rules {
    unsigned int table_size;  /**< Size of rules table */
    unsigned int rule_cnt;    /**< Current number of rules */
    bool spec_loc_flag;    /**< If @c true, special insert locations
                                   for rules are supported */
    unsigned int *locs;       /**< Locations of existing rules in rules
                                   table */
} ta_ethtool_rx_cls_rules;

/* Maximum size of level 3 (IP) address */
#define TA_MAX_L3_ADDR sizeof(struct in6_addr)

/* Fields defining network flow for Rx classification rule */
typedef struct ta_ethtool_rx_cls_rule_fields {
    /** Source MAC address */
    uint8_t src_mac[ETH_ALEN];
    /** Destination MAC address */
    uint8_t dst_mac[ETH_ALEN];
    /** EtherType */
    uint16_t ether_type;
    /** VLAN tag protocol identifier */
    uint16_t vlan_tpid;
    /** VLAN tag control information */
    uint16_t vlan_tci;
    /** First number of user-defined data */
    uint32_t data0;
    /** Second number of user-defined data */
    uint32_t data1;

    /** Level 3 (IP) source address */
    uint8_t src_l3_addr[TA_MAX_L3_ADDR];
    /** Level 3 (IP) destination address */
    uint8_t dst_l3_addr[TA_MAX_L3_ADDR];
    /** Source port (TCP, UDP) */
    uint16_t src_port;
    /** Destination port (TCP, UDP) */
    uint16_t dst_port;
    /** IPv4 TOS or IPv6 traffic class */
    uint8_t tos_or_tclass;
    /** Security Parameters Index */
    uint32_t spi;
    /** First four bytes of L4 (transport) header */
    uint32_t l4_4_bytes;
    /** Transport protocol number */
    uint8_t l4_proto;
} ta_ethtool_rx_cls_rule_fields;

/** Description of Rx classification rule */
typedef struct ta_ethtool_rx_cls_rule {
    /** Location in rules table */
    uint32_t location;

    /** Network flow type (TCP_V4_FLOW, ETHER_FLOW, etc) */
    uint32_t flow_type;
    /** RSS context (negative means not set) */
    intmax_t rss_context;
    /** Rx queue */
    uintmax_t rx_queue;

    /** Values of network flow fields */
    ta_ethtool_rx_cls_rule_fields field_values;
    /** Masks of network flow fields */
    ta_ethtool_rx_cls_rule_fields field_masks;
} ta_ethtool_rx_cls_rule;

/**
 * Get information about Rx rules.
 *
 * @param gid         Request group ID
 * @param if_name     Interface name
 * @param rules_data  Where to save pointer to ta_ethtool_rx_cls_rules
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_get_rx_cls_rules(
                                      unsigned int gid, const char *if_name,
                                      ta_ethtool_rx_cls_rules **rules_data);

/**
 * Get information about specific Rx classification rule.
 *
 * @param gid             Request group ID
 * @param if_name         Interface name
 * @param location        Rule location
 * @param rule_out        Where to save pointer to rule data
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_get_rx_cls_rule(
                                      unsigned int gid, const char *if_name,
                                      unsigned int location,
                                      ta_ethtool_rx_cls_rule **rule_out);

/**
 * Add a new Rx classification rule (should be committed later).
 *
 * @param gid             Request group ID
 * @param if_name         Interface name
 * @param location        Rule location
 * @param rule_out        Where to save pointer to rule data
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_add_rx_cls_rule(
                                      unsigned int gid, const char *if_name,
                                      unsigned int location,
                                      ta_ethtool_rx_cls_rule **rule_out);

/**
 * Commit changes made to existing Rx classification rule or insert a new
 * rule.
 *
 * @param gid             Request group ID
 * @param if_name         Interface name
 * @param location        Rule location (may be one of special rule
 *                        insert location values like @c RX_CLS_LOC_ANY)
 * @param ret_location    Resulting rule location (may be different from
 *                        requested @p location if special value was
 *                        passed)
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_commit_rx_cls_rule(
                                            unsigned int gid,
                                            const char *if_name,
                                            unsigned int location,
                                            unsigned int *ret_location);

/**
 * Remove existing Rx classification rule.
 *
 * @param if_name         Interface name
 * @param location        Rule location
 *
 * @return Status code.
 */
extern te_errno ta_ethtool_del_rx_cls_rule(const char *if_name,
                                           unsigned int location);

#endif /* ifdef ETHTOOL_GRXCLSRLALL */

/**
 * Get the failed Ethtool command.
 *
 * @return The last failed command number.
 */
extern int ta_ethtool_failed_cmd(void);

/**
 * Reset the failed Ethtool command number. Call this right
 * before calling a function from this API which may fail due to
 * ioctl(SIOCETHTOOL), if you are going to check
 * ta_ethtool_failed_cmd() after that.
 */
extern void ta_ethtool_reset_failed_cmd(void);

/**
 * Get string representation of native ethtool command.
 *
 * @param cmd     Ethtool command number.
 *
 * @return String representation.
 */
extern const char *ta_ethtool_cmd2str(int cmd);

#endif /* !__TE_AGENTS_UNIX_CONF_BASE_CONF_ETHTOOL_H_ */
