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

/* @(#)xdr_array.c	2.1 88/07/29 4.0 RPCSRC */
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
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)xdr_array.c 1.10 87/08/11 Copyr 1984 Sun Micro";
#endif

/*
 * xdr_array.c, Generic XDR routines implementation.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * arrays.  See xdr.h for more info on the interface to xdr.
 */

#include "internal.h"

#define LASTUNSIGNED	((u_int)0-1)


/*
 * XDR an array of arbitrary elements
 * *addrp is a pointer to the array, *sizep is the number of elements.
 * If addrp is NULL (*sizep * elsize) bytes are allocated.
 * elsize is the size (in bytes) of each element, and elproc is the
 * xdr procedure to call to handle each element of the array.
 */
bool_t
xdr_array (xdrs, addrp, sizep, maxsize, elsize, elproc)
     XDR *xdrs;
     caddr_t *addrp;		/* array pointer */
     u_int *sizep;		/* number of elements */
     u_int maxsize;		/* max numberof elements */
     u_int elsize;		/* size in bytes of each element */
     xdrproc_t elproc;		/* xdr routine to handle each element */
{
  u_int i;
  caddr_t target = *addrp;
  u_int c;		/* the actual element count */
  bool_t stat = TRUE;
  u_int nodesize;

  /* like strings, arrays are really counted arrays */
  if (!INTUSE(xdr_u_int) (xdrs, sizep))
    {
      return FALSE;
    }
  c = *sizep;
  /*
   * XXX: Let the overflow possibly happen with XDR_FREE because mem_free()
   * doesn't actually use its second argument anyway.
   */
  if ((c > maxsize || c > UINT_MAX / elsize) && (xdrs->x_op != XDR_FREE))
    {
      return FALSE;
    }
  nodesize = c * elsize;

  /*
   * if we are deserializing, we may need to allocate an array.
   * We also save time by checking for a null array if we are freeing.
   */
  if (target == NULL)
    switch (xdrs->x_op)
      {
      case XDR_DECODE:
	if (c == 0)
	  return TRUE;
	*addrp = target = mem_alloc (nodesize);
	if (target == NULL)
	  {
#ifdef USE_IN_LIBIO
	    if (_IO_fwide (stderr, 0) > 0)
	      (void) __fwprintf (stderr, L"%s",
				 _("xdr_array: out of memory\n"));
	    else
#endif
	      (void) fputs (_("xdr_array: out of memory\n"), stderr);
	    return FALSE;
	  }
	__bzero (target, nodesize);
	break;

      case XDR_FREE:
	return TRUE;
      default:
	break;
      }

  /*
   * now we xdr each element of array
   */
  for (i = 0; (i < c) && stat; i++)
    {
      stat = (*elproc) (xdrs, target, LASTUNSIGNED);
      target += elsize;
    }

  /*
   * the array may need freeing
   */
  if (xdrs->x_op == XDR_FREE)
    {
      mem_free (*addrp, nodesize);
      *addrp = NULL;
    }
  return stat;
}
INTDEF(xdr_array)

/*
 * xdr_vector():
 *
 * XDR a fixed length array. Unlike variable-length arrays,
 * the storage of fixed length arrays is static and unfreeable.
 * > basep: base of the array
 * > size: size of the array
 * > elemsize: size of each element
 * > xdr_elem: routine to XDR each element
 */
bool_t
xdr_vector (xdrs, basep, nelem, elemsize, xdr_elem)
     XDR *xdrs;
     char *basep;
     u_int nelem;
     u_int elemsize;
     xdrproc_t xdr_elem;
{
  u_int i;
  char *elptr;

  elptr = basep;
  for (i = 0; i < nelem; i++)
    {
      if (!(*xdr_elem) (xdrs, elptr, LASTUNSIGNED))
	{
	  return FALSE;
	}
      elptr += elemsize;
    }
  return TRUE;
}
