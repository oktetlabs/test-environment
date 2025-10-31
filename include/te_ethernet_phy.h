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

/* Duplex constants */
#define TE_PHY_DUPLEX_STRING_FULL     "full"    /* Full duplex */
#define TE_PHY_DUPLEX_STRING_HALF     "half"    /* Half duplex */
#define TE_PHY_DUPLEX_STRING_UNKNOWN  "unknown" /* Unknown duplex state */


/*
 * Numerical constants
 */


/* PHY link states */
#define TE_PHY_STATE_DOWN             (0)       /* Link down */
#define TE_PHY_STATE_UP               (1)       /* Link up */
#define TE_PHY_STATE_UNKNOWN          (2)       /* Unknown link state */

/* PHY autonegotiation states */
#define TE_PHY_AUTONEG_OFF            (0)       /* Autonegotiation off */
#define TE_PHY_AUTONEG_ON             (1)       /* Autonegotiation on */
#define TE_PHY_AUTONEG_UNKNOWN        (2)       /* Unknown autoneg state */

/* PHY speed values */
#define TE_PHY_SPEED_UNKNOWN          (-1)      /* Unknown speed value */
#define TE_PHY_SPEED_10               (10)      /* 10 Mb/sec */
#define TE_PHY_SPEED_100              (100)     /* 100 Mb/sec */
#define TE_PHY_SPEED_1000             (1000)    /* 1000 Mb/sec */
#define TE_PHY_SPEED_2500             (2500)    /* 2500 Mb/sec */
#define TE_PHY_SPEED_5000             (5000)    /* 5000 Mb/sec */
#define TE_PHY_SPEED_10000            (10000)   /* 10000 Mb/sec */
#define TE_PHY_SPEED_14000            (14000)   /* 14000 Mb/sec */
#define TE_PHY_SPEED_20000            (20000)   /* 20000 Mb/sec */
#define TE_PHY_SPEED_25000            (25000)   /* 25000 Mb/sec */
#define TE_PHY_SPEED_40000            (40000)   /* 40000 Mb/sec */
#define TE_PHY_SPEED_50000            (50000)   /* 50000 Mb/sec */
#define TE_PHY_SPEED_56000            (56000)   /* 56000 Mb/sec */
#define TE_PHY_SPEED_100000           (100000)  /* 100000 Mb/sec */
#define TE_PHY_SPEED_200000           (200000)  /* 200000 Mb/sec */
#define TE_PHY_SPEED_400000           (400000)  /* 400000 Mb/sec */

/* PHY pause frame use */
#define TE_PHY_PAUSE_NONE                   (0)     /* No pause frame use */
#define TE_PHY_PAUSE_TX_ONLY                (1)     /* Transmit only */
#define TE_PHY_PAUSE_SYMMETRIC              (2)     /* Symmetric */
#define TE_PHY_PAUSE_SYMMETRIC_RX_ONLY      (3)     /* Symmetric or receive only */
#define TE_PHY_PAUSE_UNKNOWN                (4)     /* Unknown pause frame use */

#define TE_PHY_DUPLEX(state_) \
    (const te_enum_map){                            \
        .name = TE_PHY_DUPLEX_STRING_ ## state_,    \
        .value = TE_PHY_DUPLEX_ ## state_           \
    }

/** PHY duplex states */
enum te_phy_duplex {
    /** PHY half duplex state */
    TE_PHY_DUPLEX_HALF    = 0,
    /** PHY full duplex state */
    TE_PHY_DUPLEX_FULL    = 1,
    /** PHY unknown duplex state */
    TE_PHY_DUPLEX_UNKNOWN = 0xff,
};

static const te_enum_map te_phy_duplex_map[] = {
    TE_PHY_DUPLEX(FULL),
    TE_PHY_DUPLEX(HALF),
    TE_PHY_DUPLEX(UNKNOWN),
    TE_ENUM_MAP_END
};

#undef TE_PHY_DUPLEX

#endif /* __TE_ETHERNET_PHY_H__ */
