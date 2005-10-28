/*	security/chap/md5.h	*/

/*
	Copyright (C) 2001-2003 InterOperability Lab (IOL)
				University of New Hampshier (UNH)
				Durham, NH 03824

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
	USA.

	The name of IOL and/or UNH may not be used to endorse or promote products
	derived from this software without specific prior written permission.
*/

#ifndef MD5_H
#define MD5_H

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

struct MD5_Context {
	unsigned int RBuffer[4];
	unsigned int MBuffer[16];
} ;

void MD5_ProcessMessage(unsigned char *message,
			unsigned int lengthHigh,
			unsigned int lengthLow, unsigned char *digest);

struct MD5_Context *MD5_InitializeContext(void);

void MD5_CalculateDigest(struct MD5_Context * context);

void MD5_PadMessage(struct MD5_Context * context,
		    int length,
		    unsigned char *message,
		    unsigned int lengthHigh, unsigned int lengthLow);

void MD5_FillContext(struct MD5_Context * context, unsigned char *message);

unsigned int MD5_CircularLeftShift(unsigned int number, int bits);

void MD5_HashFunction(unsigned int *a,
		      unsigned int b,
		      unsigned int c,
		      unsigned int d,
		      unsigned int x, unsigned int t, int i, int round);

#endif
