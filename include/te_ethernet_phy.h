/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment common definitions
 *
 * PHY useful constants definition (Both: TAPI and configurator)
 *
 *
 * Copyright (C) 2004-2025 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_ETHERNET_PHY_H__
#define __TE_ETHERNET_PHY_H__

#include "te_enum.h"

/*
 * String constants
 */


/* Speed constants */
#define TE_PHY_SPEED_STRING_10        "10"      /* 10 Mbit/sec */
#define TE_PHY_SPEED_STRING_100       "100"     /* 100 Mbit/sec */
#define TE_PHY_SPEED_STRING_1000      "1000"    /* 1000 Mbit/sec */
#define TE_PHY_SPEED_STRING_10000     "10000"   /* 10000 Mbit/sec */

/* Autonegotiation constants */
#define TE_PHY_AUTONEG_STRING_ON      "on"      /* Autonegotiation on */
#define TE_PHY_AUTONEG_STRING_OFF     "off"     /* Autonegatiation off */
#define TE_PHY_AUTONEG_STRING_UNKNOWN "unknown" /* Unknown autoneg state */

/* Duplex constants */
#define TE_PHY_DUPLEX_STRING_FULL     "full"    /* Full duplex */
#define TE_PHY_DUPLEX_STRING_HALF     "half"    /* Half duplex */
#define TE_PHY_DUPLEX_STRING_UNKNOWN  "unknown" /* Unknown duplex state */

#define TE_PHY_AUTONEG(state_) \
    (const te_enum_map){                            \
        .name = TE_PHY_AUTONEG_STRING_ ## state_,   \
        .value = TE_PHY_AUTONEG_ ## state_          \
    }

#define TE_PHY_DUPLEX(state_) \
    (const te_enum_map){                            \
        .name = TE_PHY_DUPLEX_STRING_ ## state_,    \
        .value = TE_PHY_DUPLEX_ ## state_           \
    }

#define TE_PHY_SPEED(speed_) \
    (const te_enum_map){ .name = #speed_, .value = TE_PHY_SPEED_ ## speed_ }

/** PHY link states */
enum te_phy_link_state {
    /** PHY link state DOWN */
    TE_PHY_STATE_DOWN    = 0,
    /** PHY link state UP */
    TE_PHY_STATE_UP      = 1,
    /** PHY link state UNKNOWN */
    TE_PHY_STATE_UNKNOWN = 2,
};

/** PHY autonegotiation states */
enum te_phy_autoneg {
    /** PHY autonegatiation state OFF */
    TE_PHY_AUTONEG_OFF     = 0,
    /** PHY autonegatiation state ON */
    TE_PHY_AUTONEG_ON      = 1,
    /** PHY autonegatiation state UNKNOWN */
    TE_PHY_AUTONEG_UNKNOWN = 2,
};

/** PHY duplex states */
enum te_phy_duplex {
    /** PHY half duplex state */
    TE_PHY_DUPLEX_HALF    = 0,
    /** PHY full duplex state */
    TE_PHY_DUPLEX_FULL    = 1,
    /** PHY unknown duplex state */
    TE_PHY_DUPLEX_UNKNOWN = 0xff,
};

/** PHY speed values (in Mbps) */
enum te_phy_speed {
    /** PHY link speed unknwon value */
    TE_PHY_SPEED_UNKNOWN = -1,
    /** PHY link speed 10 Mb/sec */
    TE_PHY_SPEED_10      = 10,
    /** PHY link speed 100 Mb/sec */
    TE_PHY_SPEED_100     = 100,
    /** PHY link speed 1000 Mb/sec */
    TE_PHY_SPEED_1000    = 1000,
    /** PHY link speed 2500 Mb/sec */
    TE_PHY_SPEED_2500    = 2500,
    /** PHY link speed 5000 Mb/sec */
    TE_PHY_SPEED_5000    = 5000,
    /** PHY link speed 10000 Mb/sec */
    TE_PHY_SPEED_10000   = 10000,
    /** PHY link speed 14000 Mb/sec */
    TE_PHY_SPEED_14000   = 14000,
    /** PHY link speed 20000 Mb/sec */
    TE_PHY_SPEED_20000   = 20000,
    /** PHY link speed 25000 Mb/sec */
    TE_PHY_SPEED_25000   = 25000,
    /** PHY link speed 40000 Mb/sec */
    TE_PHY_SPEED_40000   = 40000,
    /** PHY link speed 50000 Mb/sec */
    TE_PHY_SPEED_50000   = 50000,
    /** PHY link speed 56000 Mb/sec */
    TE_PHY_SPEED_56000   = 56000,
    /** PHY link speed 100000 Mb/sec */
    TE_PHY_SPEED_100000  = 100000,
    /** PHY link speed 200000 Mb/sec */
    TE_PHY_SPEED_200000  = 200000,
    /** PHY link speed 400000 Mb/sec */
    TE_PHY_SPEED_400000  = 400000,
};

/** PHY pause frame use */
enum te_phy_pause {
    /** PHY no pause frame to use */
    TE_PHY_PAUSE_NONE              = 0,
    /** PHY transmit only pause frame */
    TE_PHY_PAUSE_TX_ONLY           = 1,
    /** PHY symmetric pause frame */
    TE_PHY_PAUSE_SYMMETRIC         = 2,
    /** PHY symmetric or receive only pause frame */
    TE_PHY_PAUSE_SYMMETRIC_RX_ONLY = 3,
    /** PHY unknown pause frame */
    TE_PHY_PAUSE_UNKNOWN           = 4,
};

static const te_enum_map te_phy_autoneg_map[] = {
    TE_PHY_AUTONEG(OFF),
    TE_PHY_AUTONEG(ON),
    TE_PHY_AUTONEG(UNKNOWN),
    TE_ENUM_MAP_END
};

static const te_enum_map te_phy_duplex_map[] = {
    TE_PHY_DUPLEX(FULL),
    TE_PHY_DUPLEX(HALF),
    TE_PHY_DUPLEX(UNKNOWN),
    TE_ENUM_MAP_END
};

static const te_enum_map te_phy_speed_map[] = {
    { .name = "unknown", .value = TE_PHY_SPEED_UNKNOWN },
    TE_PHY_SPEED(10),
    TE_PHY_SPEED(100),
    TE_PHY_SPEED(100),
    TE_PHY_SPEED(1000),
    TE_PHY_SPEED(2500),
    TE_PHY_SPEED(5000),
    TE_PHY_SPEED(10000),
    TE_PHY_SPEED(14000),
    TE_PHY_SPEED(20000),
    TE_PHY_SPEED(25000),
    TE_PHY_SPEED(40000),
    TE_PHY_SPEED(50000),
    TE_PHY_SPEED(56000),
    TE_PHY_SPEED(100000),
    TE_PHY_SPEED(200000),
    TE_PHY_SPEED(400000),
    TE_ENUM_MAP_END
};

#undef TE_PHY_SPEED
#undef TE_PHY_DUPLEX
#undef TE_PHY_AUTONEG

#endif /* __TE_ETHERNET_PHY_H__ */
