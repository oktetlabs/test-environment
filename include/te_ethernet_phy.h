/** @file
 * @brief Test Environment common definitions
 *
 * PHY usefull constants definition (Both: TAPI and configurator)
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifndef __TE_ETHERNET_PHY_H__
#define __TE_ETHERNET_PHY_H__


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

/* PHY duplex states */
#define TE_PHY_DUPLEX_HALF            (0)       /* Half duplex */
#define TE_PHY_DUPLEX_FULL            (1)       /* Full duplex */
#define TE_PHY_DUPLEX_UNKNOWN         (2)       /* Unknown duplex state */

/* PHY speed values */
#define TE_PHY_SPEED_UNKNOWN          (0)       /* Unknown speed value */
#define TE_PHY_SPEED_10               (10)      /* 10 Mb/sec */
#define TE_PHY_SPEED_100              (100)     /* 100 Mb/sec */
#define TE_PHY_SPEED_1000             (1000)    /* 1000 Mb/sec */
#define TE_PHY_SPEED_10000            (10000)   /* 10000 Mb/sec */

/* PHY pause frame use */
#define TE_PHY_PAUSE_NONE                   (0)     /* No pause frame use */
#define TE_PHY_PAUSE_TX_ONLY                (1)     /* Transmit only */
#define TE_PHY_PAUSE_SYMMETRIC              (2)     /* Symmetric */
#define TE_PHY_PAUSE_SYMMETRIC_RX_ONLY      (3)     /* Symmetric or receive only */
#define TE_PHY_PAUSE_UNKNOWN                (4)     /* Unknown pause frame use */

#endif /* __TE_ETHERNET_PHY_H__ */
