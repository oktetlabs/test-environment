/** @file
 * @brief SUN RPC Library for Windows
 *
 * Adaptation of original module
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2006 Level5 Networks Corp.
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */


/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * xdr_mem.h, XDR implementation using memory buffers.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * If you have some data to be interpreted as external data representation
 * or to be converted to external data representation in a memory buffer,
 * then this is the package for you.
 *
 */

#include "internal.h"

static bool_t xdrmem_getlong (XDR *, long *);
static bool_t xdrmem_putlong (XDR *, const long *);
static bool_t xdrmem_getbytes (XDR *, caddr_t, u_int);
static bool_t xdrmem_putbytes (XDR *, const char *, u_int);
static u_int xdrmem_getpos (const XDR *);
static bool_t xdrmem_setpos (XDR *, u_int);
static int32_t *xdrmem_inline (XDR *, u_int);
static void xdrmem_destroy (XDR *);
static bool_t xdrmem_getint32 (XDR *, int32_t *);
static bool_t xdrmem_putint32 (XDR *, const int32_t *);

static const struct xdr_ops xdrmem_ops =
{
  xdrmem_getlong,
  xdrmem_putlong,
  xdrmem_getbytes,
  xdrmem_putbytes,
  xdrmem_getpos,
  xdrmem_setpos,
  xdrmem_inline,
  xdrmem_destroy,
  xdrmem_getint32,
  xdrmem_putint32
};

/*
 * The procedure xdrmem_create initializes a stream descriptor for a
 * memory buffer.
 */
void
xdrmem_create (XDR *xdrs, const caddr_t addr, u_int size, enum xdr_op op)
{
  xdrs->x_op = op;
  /* We have to add the const since the `struct xdr_ops' in `struct XDR'
     is not `const'.  */
  xdrs->x_ops = (struct xdr_ops *) &xdrmem_ops;
  xdrs->x_private = xdrs->x_base = addr;
  xdrs->x_handy = size;
}
INTDEF(xdrmem_create)

/*
 * Nothing needs to be done for the memory case.  The argument is clearly
 * const.
 */

static void
xdrmem_destroy (XDR *xdrs)
{
}

/*
 * Gets the next word from the memory referenced by xdrs and places it
 * in the long pointed to by lp.  It then increments the private word to
 * point at the next element.  Neither object pointed to is const
 */
static bool_t
xdrmem_getlong (XDR *xdrs, long *lp)
{
  if (xdrs->x_handy < 4)
    return FALSE;
  xdrs->x_handy -= 4;
  *lp = (int32_t) ntohl ((*((int32_t *) (xdrs->x_private))));
  xdrs->x_private += 4;
  return TRUE;
}

/*
 * Puts the long pointed to by lp in the memory referenced by xdrs.  It
 * then increments the private word to point at the next element.  The
 * long pointed at is const
 */
static bool_t
xdrmem_putlong (XDR *xdrs, const long *lp)
{
  if (xdrs->x_handy < 4)
    return FALSE;
  xdrs->x_handy -= 4;
  *(int32_t *) xdrs->x_private = htonl (*lp);
  xdrs->x_private += 4;
  return TRUE;
}

/*
 * Gets an unaligned number of bytes from the xdrs structure and writes them
 * to the address passed in addr.  Be very careful when calling this routine
 * as it could leave the xdrs pointing to an unaligned structure which is not
 * a good idea.  None of the things pointed to are const.
 */
static bool_t
xdrmem_getbytes (XDR *xdrs, caddr_t addr, u_int len)
{
  if (xdrs->x_handy < len)
    return FALSE;
  xdrs->x_handy -= len;
  memcpy (addr, xdrs->x_private, len);
  xdrs->x_private += len;
  return TRUE;
}

/*
 * The complementary function to the above.  The same warnings apply about
 * unaligned data.  The source address is const.
 */
static bool_t
xdrmem_putbytes (XDR *xdrs, const char *addr, u_int len)
{
  if (xdrs->x_handy < len)
    return FALSE;
  xdrs->x_handy -= len;
  memcpy (xdrs->x_private, addr, len);
  xdrs->x_private += len;
  return TRUE;
}

/*
 * Not sure what this one does.  But it clearly doesn't modify the contents
 * of xdrs.  **FIXME** does this not assume u_int == u_long?
 */
static u_int
xdrmem_getpos (const XDR *xdrs)
{
  return (u_long) xdrs->x_private - (u_long) xdrs->x_base;
}

/*
 * xdrs modified
 */
static bool_t
xdrmem_setpos (xdrs, pos)
     XDR *xdrs;
     u_int pos;
{
  caddr_t newaddr = xdrs->x_base + pos;
  caddr_t lastaddr = xdrs->x_private + xdrs->x_handy;

  if ((long) newaddr > (long) lastaddr
      || (UINT_MAX < LONG_MAX
	  && (long) UINT_MAX < (long) lastaddr - (long) newaddr))
    return FALSE;
  xdrs->x_private = newaddr;
  xdrs->x_handy = (long) lastaddr - (long) newaddr;
  return TRUE;
}

/*
 * xdrs modified
 */
static int32_t *
xdrmem_inline (XDR *xdrs, u_int len)
{
  int32_t *buf = 0;

  if (xdrs->x_handy >= len)
    {
      xdrs->x_handy -= len;
      buf = (int32_t *) xdrs->x_private;
      xdrs->x_private += len;
    }
  return buf;
}

/*
 * Gets the next word from the memory referenced by xdrs and places it
 * in the int pointed to by ip.  It then increments the private word to
 * point at the next element.  Neither object pointed to is const
 */
static bool_t
xdrmem_getint32 (XDR *xdrs, int32_t *ip)
{
  if (xdrs->x_handy < 4)
    return FALSE;
  xdrs->x_handy -= 4;
  *ip = ntohl ((*((int32_t *) (xdrs->x_private))));
  xdrs->x_private += 4;
  return TRUE;
}

/*
 * Puts the long pointed to by lp in the memory referenced by xdrs.  It
 * then increments the private word to point at the next element.  The
 * long pointed at is const
 */
static bool_t
xdrmem_putint32 (XDR *xdrs, const int32_t *ip)
{
  if (xdrs->x_handy < 4)
    return FALSE;
  xdrs->x_handy -= 4;
  *(int32_t *) xdrs->x_private = htonl (*ip);
  xdrs->x_private += 4;
  return TRUE;
}
