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
#include <linux/rtnetlink.h>

#include <linux/if.h>
#include "if_mangle.h"

#include <asm/uaccess.h>

#ifndef NETIF_F_UFO
#define NETIF_F_UFO 0
#endif

#define MANGLE_FEATURE_MASK (NETIF_F_SG | NETIF_F_IP_CSUM | \
                             NETIF_F_NO_CSUM | NETIF_F_HW_CSUM | \
                             NETIF_F_TSO | NETIF_F_UFO | NETIF_F_HIGHDMA | \
                             NETIF_F_FRAGLIST )

typedef struct mangler_t {
    struct net_device_stats stats;
    struct net_device      *slave_dev;
    unsigned                slave_dev_ref_cnt;
    unsigned                drop_rate;
    unsigned                drop_count;
} mangler_t;

static int mangle_open(struct net_device *dev);
static int mangle_close(struct net_device *dev);
static int mangle_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
static int mangle_xmit(struct sk_buff *skb, struct net_device *dev);
static struct net_device_stats *mangle_get_stats(struct net_device *dev);

static char version[] __initdata = 
	"Mangler: Artem V. Andreev <Artem.Andreev@oktetlabs.ru>\n";

static int
mangle_dev_init(struct net_device *dev)
{
    printk(KERN_DEBUG "mangle_dev_init called on %p\n", dev);
    return 0;
}

static void __init
mangle_setup(struct net_device *dev)
{
	mangler_t *mng = netdev_priv(dev);

    printk(KERN_DEBUG "initializing mangle device\n");
    
    SET_MODULE_OWNER(dev);

	dev->open		= mangle_open;
	dev->stop		= mangle_close;
	dev->do_ioctl		= mangle_ioctl;
	dev->hard_start_xmit	= mangle_xmit;
	dev->get_stats		= mangle_get_stats;
    dev->init  = mangle_dev_init;
    dev->destructor = free_netdev;

    ether_setup(dev);
    
	dev->flags    |= IFF_MASTER;
#if 0  
	/*
	 *	Now we undo some of the things that eth_setup does
	 * 	that we don't like 
	 */
	 
	dev->mtu        	= EQL_DEFAULT_MTU;	/* set to 576 in if_eql.h */

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

    printk(KERN_DEBUG "closing mangle device\n");
    if (mng->slave_dev != NULL)
    {
        netdev_set_master(mng->slave_dev, NULL);
        dev_close(mng->slave_dev);
        dev_put(mng->slave_dev);
        mng->slave_dev = NULL;
        mng->slave_dev_ref_cnt = 0;
    }

	return 0;
}

static int mangle_enslave(struct net_device *dev, const char *name);
static int mangle_emancipate(struct net_device *dev, const char *name);
static int mangle_configure(struct net_device *dev, mangle_configure_request __user *req);
static int mangle_update_slave(struct net_device *dev);

static int 
mangle_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{  
    if (!capable(CAP_NET_ADMIN))
        return -EPERM;
    switch (cmd) 
    {
		case MANGLE_ENSLAVE:
			return mangle_enslave(dev, ifr->ifr_slave);
		case MANGLE_EMANCIPATE:
			return mangle_emancipate(dev, ifr->ifr_slave);
        case MANGLE_CONFIGURE:
			return mangle_configure(dev, ifr->ifr_data);
        case MANGLE_UPDATE_SLAVE:
            return mangle_update_slave(dev);
		default:
        {
            printk(KERN_DEBUG "unsupported ioctl %d\n", cmd);
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
        printk(KERN_DEBUG "packet: %d %d %d\n", skb->len, skb->data_len,
               skb_shinfo(skb)->nr_frags);
        if (mng->drop_count == 1)
        {
            mng->drop_count = mng->drop_rate;
            mng->stats.tx_dropped++;
            dev_kfree_skb(skb);
        }
        else
        {
            if (mng->drop_count != 0)
                mng->drop_count--;
            skb->dev = mng->slave_dev;
            skb->priority = 1;
            dev_queue_xmit(skb);
            mng->stats.tx_packets++;
        }
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

    if ((master_dev->flags & IFF_UP) == 0)
    {
        printk(KERN_ERR "mangle0 is not up!!!\n");
        return -EPERM;
    }

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
        else if ((slave_dev->flags & (IFF_MASTER | IFF_SLAVE)) != 0)
        {
            printk(KERN_ERR "interface is already master or slave\n");
            rc = -EBUSY;
            dev_put(slave_dev);
        }
        else
        {
            rc = dev_open(slave_dev);
            if (rc == 0)
            {
                rc = netdev_set_master(slave_dev, master_dev);
                if (rc != 0)
                    dev_close(slave_dev);
                else
                {
                    struct sockaddr addr;

                    memcpy(&addr.sa_data, slave_dev->dev_addr, slave_dev->addr_len);
                    addr.sa_family = slave_dev->type;
                    master_dev->set_mac_address(master_dev, &addr);
                    mng->slave_dev = slave_dev;
                    mng->slave_dev_ref_cnt = 1;
                    rc = mangle_update_slave(master_dev);
                }
            }
            if (rc != 0)
                dev_put(slave_dev);
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
                netdev_set_master(mng->slave_dev, NULL);
                dev_close(mng->slave_dev);
                dev_put(mng->slave_dev);
                mng->slave_dev = NULL;
            }
        }
        dev_put(slave_dev);
	}

	return rc;
}

static int
mangle_configure(struct net_device *master_dev,
                 mangle_configure_request __user *userreq)
{
    mangler_t *mng = netdev_priv(master_dev);
    mangle_configure_request req;
    
    if (copy_from_user(&req, userreq, sizeof(req)))
        return -EFAULT;
    
    switch (req.param)
    {
        case MANGLE_CONFIGURE_DROP_RATE:
            if (req.value < 0)
                return -EINVAL;
            mng->drop_count = mng->drop_rate = req.value;
            return 0;
        default:
            printk(KERN_DEBUG "unsupported configuration param %d", req.param);
            return -EINVAL;
    }
}

static int
mangle_update_slave(struct net_device *master_dev)
{
    mangler_t *mng = netdev_priv(master_dev);

    if (mng->slave_dev == NULL)
        return -ENODEV;

    master_dev->features = mng->slave_dev->features & MANGLE_FEATURE_MASK;
    printk(KERN_DEBUG "computed features are: %8.8x from %8.8x\n", master_dev->features,
           mng->slave_dev->features);
    return 0;
}

static struct net_device *dev_mng;

static int __init 
mangle_init_module(void)
{
	int err;

	printk(version);

    rtnl_lock();
	dev_mng = alloc_netdev(sizeof(mangler_t), "mangle0", mangle_setup);
	if (dev_mng == NULL)
    {
		return -ENOMEM;
    }
    printk(KERN_DEBUG "mangle device created\n");
	err = register_netdevice(dev_mng);
    printk(KERN_DEBUG "mangle device registered (%d)\n", err);
    rtnl_unlock();

	if (err) 
		free_netdev(dev_mng);
	return err;
}

static void __exit 
mangle_cleanup_module(void)
{
    rtnl_lock();
 	unregister_netdevice(dev_mng);
    rtnl_unlock();
}

module_init(mangle_init_module);
module_exit(mangle_cleanup_module);
MODULE_LICENSE("GPL");
