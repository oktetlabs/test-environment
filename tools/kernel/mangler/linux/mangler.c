/** @file
 * @brief Socket buffer mangler
 *
 * Linux module for socket buffer mangling
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author: Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id: tapi_iscsi.c 30203 2006-07-14 16:32:31Z artem $
 */


/*
 * This file is based on eql.c code:
 *
 * (c) Copyright 1995 Simon "Guru Aleph-Null" Janes
 * NCM: Network and Communications Management, Inc.
 *
 * (c) Copyright 2002 David S. Miller (davem@redhat.com)
 *
 *	This software may be used and distributed according to the terms
 *	of the GNU General Public License, incorporated herein by reference.
 * 
 * The author may be reached as simon@ncm.com, or C/O
 *    NCM
 *    Attn: Simon Janes
 *    6803 Whittier Ave
 *    McLean VA 22101
 *    Phone: 1-703-847-0040 ext 103
 */

/*
 * Sources:
 *   skeleton.c by Donald Becker.
 * Inspirations:
 *   The Harried and Overworked Alan Cox
 * Conspiracies:
 *   The Alan Cox and Mike McLagan plot to get someone else to do the code, 
 *   which turned out to be me.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/netdevice.h>

#include <linux/if.h>
#include "if_mangle.h"

#include <asm/uaccess.h>

typedef struct mangler_t {
    struct net_device_stats stats;
    struct net_device      *slave_dev;
    unsigned                slave_dev_ref_cnt;
} mangler_t;

static int mangle_open(struct net_device *dev);
static int mangle_close(struct net_device *dev);
static int mangle_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
static int mangle_xmit(struct sk_buff *skb, struct net_device *dev);
static struct net_device_stats *mangle_get_stats(struct net_device *dev);

static char version[] __initdata = 
	"Mangler: Artem V. Andreev <Artem.Andreev@oktetlabs.ru>\n";

static void __init
mangle_setup(struct net_device *dev)
{
	mangler_t *mng = netdev_priv(dev);

	SET_MODULE_OWNER(dev);

	dev->open		= mangle_open;
	dev->stop		= mangle_close;
	dev->do_ioctl		= mangle_ioctl;
	dev->hard_start_xmit	= mangle_xmit;
	dev->get_stats		= mangle_get_stats;

    ether_setup(dev);
    
    dev->features |= NETIF_F_SG;
#if 0  
	/*
	 *	Now we undo some of the things that eth_setup does
	 * 	that we don't like 
	 */
	 
	dev->mtu        	= EQL_DEFAULT_MTU;	/* set to 576 in if_eql.h */
	dev->flags      	= IFF_MASTER;

	dev->type       	= ARPHRD_SLIP;
	dev->tx_queue_len 	= 5;		/* Hands them off fast */
#endif
}

static int 
mangle_open(struct net_device *dev)
{
	mangler_t *mng = netdev_priv(dev);

	return 0;
}


static int 
mangle_close(struct net_device *dev)
{
	mangler_t *mng = netdev_priv(dev);

    if (mng->slave_dev != NULL)
    {
        dev_put(mng->slave_dev);
        mng->slave_dev = NULL;
        mng->slave_dev_ref_cnt = 0;
    }

	return 0;
}

static int mangle_enslave(struct net_device *dev, const char *name);
static int mangle_emancipate(struct net_device *dev, const char *name);

static int 
mangle_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{  
    switch (cmd) 
    {
		case MANGLE_ENSLAVE:
			return capable(CAP_NET_ADMIN) ? 
                mangle_enslave(dev, ifr->ifr_slave) :
                -EPERM;
		case MANGLE_EMANCIPATE:
			return capable(CAP_NET_ADMIN) ? 
                mangle_emancipate(dev, ifr->ifr_slave) :
                -EPERM;
		default:
        {
			return -EOPNOTSUPP;   
        }
	}
}


static int 
mangle_xmit(struct sk_buff *skb, struct net_device *dev)
{
	mangler_t *mng = netdev_priv(dev);

	if (mng->slave_dev != NULL)
    {
		skb->dev = mng->slave_dev;
		skb->priority = 1;
		dev_queue_xmit(skb);
		mng->stats.tx_packets++;
	}
    else
    {
		mng->stats.tx_dropped++;
		dev_kfree_skb(skb);
	}	  
	return 0;
}

static struct net_device_stats * 
mangle_get_stats(struct net_device *dev)
{
	mangler_t *mng = netdev_priv(dev);
	return &mng->stats;
}


static int 
mangle_enslave(struct net_device *master_dev, 
               const char *slave_name)
{
    mangler_t *mng = netdev_priv(master_dev);
	struct net_device *slave_dev;
    int rc = -EINVAL;

    printk(KERN_INFO "attaching interface %s\n", slave_name);

	slave_dev  = dev_get_by_name(slave_name);
	if (slave_dev != NULL) 
    {
        if (mng->slave_dev != NULL)
        {
            if (mng->slave_dev != slave_dev)
                rc = -EEXIST;
            else
            {
                mng->slave_dev_ref_cnt++;
                rc = 0;
            }
            dev_put(slave_dev);
        }
        else
        {
            mng->slave_dev = slave_dev;
            mng->slave_dev_ref_cnt = 1;
            rc = 0;
		}
	}

	return rc;
}

static int 
mangle_emancipate(struct net_device *master_dev, 
                  const char *slave_name)
{
	mangler_t *mng = netdev_priv(master_dev);
	struct net_device *slave_dev;
	int rc;

	slave_dev = dev_get_by_name(slave_name);
	rc = -EINVAL;
	if (slave_dev != NULL) 
    {
        if (slave_dev == mng->slave_dev)
        {
            rc = 0;
            if (--mng->slave_dev_ref_cnt == 0)
            {
                dev_put(mng->slave_dev);
                mng->slave_dev = NULL;
            }
        }
        dev_put(slave_dev);
	}

	return rc;
}

static struct net_device *dev_mng;

static int __init 
mangle_init_module(void)
{
	int err;

	printk(version);

	dev_mng = alloc_netdev(sizeof(mangler_t), "mangle0", mangle_setup);
	if (!dev_mng)
		return -ENOMEM;

	err = register_netdev(dev_mng);
	if (err) 
		free_netdev(dev_mng);
	return err;
}

static void __exit 
mangle_cleanup_module(void)
{
	unregister_netdev(dev_mng);
	free_netdev(dev_mng);
}

module_init(mangle_init_module);
module_exit(mangle_cleanup_module);
MODULE_LICENSE("GPL");
