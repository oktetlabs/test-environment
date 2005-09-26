/*	security/chap/md5.c	*/

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

#include <stdlib.h>
#include <string.h>

#include <my_memory.h>
#include <stdlib.h>
#include <stddef.h>

#include "md5.h"

static unsigned int T[] = { 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x2441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x4881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

/*	computes the MD5 hash of the input "message" and stores the 16-byte result
	in "digest".
	On input, the length of the input "message" is in length.
*/
void
MD5_ProcessMessage(unsigned char *message,
		   unsigned int lengthHigh,
		   unsigned int lengthLow, unsigned char *digest)
{
	int i;
	unsigned char messageBlock[64];
	struct MD5_Context *context;
	long long length = ((((long long) lengthHigh) << 32) + lengthLow) / 8;

	if ((message == NULL) || (digest == NULL))
		return;

	/* RDR start */
	memset(digest, 0, 16);

	context = MD5_InitializeContext();
	if (context == NULL)
		return;
	/* RDR end */

	while (length >= 64) {
		memcpy(messageBlock, message, 64);
		MD5_FillContext(context, messageBlock);
		MD5_CalculateDigest(context);
		message += 64;
		length -= 64;
	}
	memset(messageBlock, 0, 64);
	if (length != 0)
		memcpy(messageBlock, message, length);
	MD5_PadMessage(context, length, messageBlock, lengthHigh, lengthLow);
	MD5_CalculateDigest(context);
	for (i = 0; i < 4; i++) {
		digest[4 * i] = context->RBuffer[i];
		digest[4 * i + 1] = context->RBuffer[i] >> 8;
		digest[4 * i + 2] = context->RBuffer[i] >> 16;
		digest[4 * i + 3] = context->RBuffer[i] >> 24;
	}
	free(context);
}

struct MD5_Context *
MD5_InitializeContext(void)
{
	struct MD5_Context *context;
	context = (struct MD5_Context *) 
			malloc(sizeof (struct MD5_Context));
	if (context == NULL)
		return NULL;
	memset(context, 0, sizeof (struct MD5_Context));
	context->RBuffer[0] = 0x67452301;
	context->RBuffer[1] = 0xefcdab89;
	context->RBuffer[2] = 0x98badcfe;
	context->RBuffer[3] = 0x10325476;
	return context;
}

void
MD5_CalculateDigest(struct MD5_Context * context)
{
	unsigned int a;
	unsigned int b;
	unsigned int c;
	unsigned int d;

	a = context->RBuffer[0];
	b = context->RBuffer[1];
	c = context->RBuffer[2];
	d = context->RBuffer[3];
	//round 0
	MD5_HashFunction(&a, b, c, d, context->MBuffer[0], T[0], 7, 0);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[1], T[1], 12, 0);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[2], T[2], 17, 0);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[3], T[3], 22, 0);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[4], T[4], 7, 0);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[5], T[5], 12, 0);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[6], T[6], 17, 0);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[7], T[7], 22, 0);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[8], T[8], 7, 0);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[9], T[9], 12, 0);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[10], T[10], 17, 0);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[11], T[11], 22, 0);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[12], T[12], 7, 0);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[13], T[13], 12, 0);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[14], T[14], 17, 0);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[15], T[15], 22, 0);
	//round 1
	MD5_HashFunction(&a, b, c, d, context->MBuffer[1], T[16], 5, 1);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[6], T[17], 9, 1);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[11], T[18], 14, 1);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[0], T[19], 20, 1);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[5], T[20], 5, 1);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[10], T[21], 9, 1);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[15], T[22], 14, 1);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[4], T[23], 20, 1);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[9], T[24], 5, 1);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[14], T[25], 9, 1);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[3], T[26], 14, 1);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[8], T[27], 20, 1);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[13], T[28], 5, 1);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[2], T[29], 9, 1);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[7], T[30], 14, 1);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[12], T[31], 20, 1);
	//round 2
	MD5_HashFunction(&a, b, c, d, context->MBuffer[5], T[32], 4, 2);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[8], T[33], 11, 2);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[11], T[34], 16, 2);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[14], T[35], 23, 2);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[1], T[36], 4, 2);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[4], T[37], 11, 2);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[7], T[38], 16, 2);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[10], T[39], 23, 2);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[13], T[40], 4, 2);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[0], T[41], 11, 2);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[3], T[42], 16, 2);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[6], T[43], 23, 2);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[9], T[44], 4, 2);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[12], T[45], 11, 2);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[15], T[46], 16, 2);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[2], T[47], 23, 2);
	//round 3
	MD5_HashFunction(&a, b, c, d, context->MBuffer[0], T[48], 6, 3);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[7], T[49], 10, 3);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[14], T[50], 15, 3);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[5], T[51], 21, 3);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[12], T[52], 6, 3);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[3], T[53], 10, 3);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[10], T[54], 15, 3);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[1], T[55], 21, 3);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[8], T[56], 6, 3);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[15], T[57], 10, 3);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[6], T[58], 15, 3);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[13], T[59], 21, 3);

	MD5_HashFunction(&a, b, c, d, context->MBuffer[4], T[60], 6, 3);
	MD5_HashFunction(&d, a, b, c, context->MBuffer[11], T[61], 10, 3);
	MD5_HashFunction(&c, d, a, b, context->MBuffer[2], T[62], 15, 3);
	MD5_HashFunction(&b, c, d, a, context->MBuffer[9], T[63], 21, 3);
	context->RBuffer[0] += a;
	context->RBuffer[1] += b;
	context->RBuffer[2] += c;
	context->RBuffer[3] += d;
}

void
MD5_PadMessage(struct MD5_Context * context,
	       int length,
	       unsigned char *message,
	       unsigned int lengthHigh, unsigned int lengthLow)
{
	if (length == 64)
		return;
	message[length] = 0x80;
	length++;
	if (length <= 56) {
		while (length < 56) {
			message[length] = 0x00;
			length++;
		}
		message[56] = lengthLow;
		message[57] = lengthLow >> 8;
		message[58] = lengthLow >> 16;
		message[59] = lengthLow >> 24;
		message[60] = lengthHigh;
		message[61] = lengthHigh >> 8;
		message[62] = lengthHigh >> 16;
		message[63] = lengthHigh >> 24;
		MD5_FillContext(context, message);
	} else {
		while (length < 64) {
			message[length] = 0x00;
			length++;
		}
		MD5_FillContext(context, message);
		MD5_CalculateDigest(context);
		length = 0;
		while (length < 56) {
			message[length] = 0x00;
			length++;
		}
		message[56] = lengthLow;
		message[57] = lengthLow >> 8;
		message[58] = lengthLow >> 16;
		message[59] = lengthLow >> 24;
		message[60] = lengthHigh;
		message[61] = lengthHigh >> 8;
		message[62] = lengthHigh >> 16;
		message[63] = lengthHigh >> 24;
		MD5_FillContext(context, message);
	}
}

void
MD5_FillContext(struct MD5_Context * context, unsigned char *message)
{
	int i;

	for (i = 0; i < 16; i++) {
		context->MBuffer[i] = message[i * 4];
		context->MBuffer[i] |= message[i * 4 + 1] << 8;
		context->MBuffer[i] |= message[i * 4 + 2] << 16;
		context->MBuffer[i] |= message[i * 4 + 3] << 24;
	}
}

unsigned int
MD5_CircularLeftShift(unsigned int number, int bits)
{
	if ((bits < 0) || (bits > 32))
		return 0;
	return (number << bits) | (number >> (32 - bits));
}

void
MD5_HashFunction(unsigned int *a,
		 unsigned int b,
		 unsigned int c,
		 unsigned int d,
		 unsigned int x, unsigned int t, int s, int round)
{
	switch (round) {
	case 0:
		*a += F(b, c, d) + x + t;
		break;
	case 1:
		*a += G(b, c, d) + x + t;
		break;
	case 2:
		*a += H(b, c, d) + x + t;
		break;
	case 3:
		*a += I(b, c, d) + x + t;
		break;
	default:
		break;
	}
	*a = MD5_CircularLeftShift(*a, s);
	*a += b;
}
