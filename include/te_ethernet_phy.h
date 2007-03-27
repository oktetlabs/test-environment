/** @file
 * @brief Test Environment common definitions
 *
 * PHY usefull constants definition (Both: TAPI and configurator)
 *
 *
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Vadim V. Galitsyn <Vadim.Galitsyn@oktetlabs.ru>
 *
 * $Id: te_ethernet_phy.h 33874 2006-11-28 17:08:31Z edward $
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
#define TE_PHY_DUPLEX_STRING_UNKNOWN  "unknown" /* Unknown duples state */


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
#define TE_PHY_DUPLEX_UNKNOWN         (2)      /* Unknown duplex state */

/* PHY speed values */
#define TE_PHY_SPEED_UNKNOWN          (0)       /* Unknown speed value */
#define TE_PHY_SPEED_10               (10)      /* 10 Mb/sec */
#define TE_PHY_SPEED_100              (100)     /* 100 Mb/sec */
#define TE_PHY_SPEED_1000             (1000)    /* 1000 Mb/sec */
#define TE_PHY_SPEED_10000            (10000)   /* 10000 Mb/sec */

#endif /* __TE_ETHERNET_PHY_H__ */
