/** @file
 * @brief UPnP Control Point process
 *
 * UPnP Control Point process functions definition.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_UPNP_CP_H__
#define __TE_UPNP_CP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Launch the UPnP Control Point process.
 * This function should be called as: "te_upnp_cp target pathname", where
 * target is a Search Target that shall be one of the following:
 *  - ssdp:all Search for all devices and services.
 *  - upnp:rootdevice Search for root devices only.
 *  - uuid:device-UUID Search for a particular device.
 *  - urn:schemas-upnp-org:device:deviceType:ver Search for any device of
 *    this type where deviceType and ver are defined by the UPnP Forum
 *    working committee.
 *  - urn:schemas-upnp-org:service:serviceType:ver Search for any service of
 *    this type where serviceType and ver are defined by the UPnP Forum
 *    working committee.
 *  - urn:domain-name:device:deviceType:ver Search for any device of this
 *    type. Where domain-name (a Vendor Domain Name), deviceType and ver are
 *    defined by the UPnP vendor.
 *  - urn:domain-name:service:serviceType:ver Search for any service of this
 *    type. Where domain-name (a Vendor Domain Name), serviceType and ver
 *    are defined by the UPnP vendor.
 * pathname is a pathname for a UNIX domain socket.
 * iface is a Network interface which be used by UPnP Control Point to
 *    interact with UPnP devices.
 */
extern int te_upnp_cp(int argc, char *argv[]);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_UPNP_CP_H__ */
