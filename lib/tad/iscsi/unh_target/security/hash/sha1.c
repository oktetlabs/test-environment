/*	security/chap/sha1.c	*/

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

#include "../../userland_lib/my_memory.h"
#include "linux/slab.h"
#include "linux/stddef.h"

#include "sha1.h"

void
SHA1_ProcessMessage(unsigned char *message,
		    unsigned int lengthHigh,
		    unsigned int lengthLow, unsigned char *digest)
{
	int i;
	unsigned char messageBlock[64];
	struct SHA1_Context *context;
	long long length = ((((long long) lengthHigh) << 32) + lengthLow) / 8;

	if ((message == NULL) || (digest == NULL))
		return;

	/* RDR start */
	memset(digest, 0, 20);
	context = SHA1_InitializeContext();
	if (context == NULL)
		return;
	/* RDR end */

	while (length >= 64) {
		memcpy(messageBlock, message, 64);
		SHA1_FillContext(context, messageBlock);
		SHA1_CalculateDigest(context);
		message += 64;
		length -= 64;
	}
	memset(messageBlock, 0, 64);
	if (length != 0)
		memcpy(messageBlock, message, length);
	SHA1_PadMessage(context, length, messageBlock, lengthHigh, lengthLow);
	SHA1_CalculateDigest(context);
	for (i = 0; i < 5; i++) {
		digest[4 * i] = context->HBuffer[i] >> 24;
		digest[4 * i + 1] = context->HBuffer[i] >> 16;
		digest[4 * i + 2] = context->HBuffer[i] >> 8;
		digest[4 * i + 3] = context->HBuffer[i];
	}
	my_free((void *) &context);
}

struct
SHA1_Context *
SHA1_InitializeContext(void)
{
	struct SHA1_Context *newContext;

	if ((newContext =
	     (struct SHA1_Context *) malloc(sizeof (struct SHA1_Context))) == NULL)
		return NULL;
	newContext->HBuffer[0] = 0x67452301;
	newContext->HBuffer[1] = 0xEFCDAB89;
	newContext->HBuffer[2] = 0x98BADCFE;
	newContext->HBuffer[3] = 0x10325476;
	newContext->HBuffer[4] = 0xC3D2E1F0;
	return newContext;
}

void
SHA1_CalculateDigest(struct SHA1_Context * context)
{
	unsigned int A, B, C, D, E, temp;
	int t;
	unsigned int WBuffer[80];
	unsigned int K[] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };

	A = context->HBuffer[0];
	B = context->HBuffer[1];
	C = context->HBuffer[2];
	D = context->HBuffer[3];
	E = context->HBuffer[4];
	for (t = 0; t < 16; t++)
		WBuffer[t] = context->Mbuffer[t];

	for (t = 16; t < 80; t++)
		WBuffer[t] =
		    SHA1_CircularLeftShift(WBuffer[t - 3] ^ WBuffer[t - 8]
					   ^ WBuffer[t - 14] ^ WBuffer[t - 16],
					   1);

	for (t = 0; t < 20; t++) {
		temp =
		    SHA1_CircularLeftShift(A, 5) + SHA1_FunctionF(t, B, C,
								  D) + E +
		    WBuffer[t] + K[0];
		E = D;
		D = C;
		C = SHA1_CircularLeftShift(B, 30);
		B = A;
		A = temp;
	}

	for (t = 20; t < 40; t++) {
		temp =
		    SHA1_CircularLeftShift(A, 5) + SHA1_FunctionF(t, B, C,
								  D) + E +
		    WBuffer[t] + K[1];
		E = D;
		D = C;
		C = SHA1_CircularLeftShift(B, 30);
		B = A;
		A = temp;
	}

	for (t = 40; t < 60; t++) {
		temp =
		    SHA1_CircularLeftShift(A, 5) + SHA1_FunctionF(t, B, C,
								  D) + E +
		    WBuffer[t] + K[2];
		E = D;
		D = C;
		C = SHA1_CircularLeftShift(B, 30);
		B = A;
		A = temp;
	}

	for (t = 60; t < 80; t++) {
		temp =
		    SHA1_CircularLeftShift(A, 5) + SHA1_FunctionF(t, B, C,
								  D) + E +
		    WBuffer[t] + K[3];
		E = D;
		D = C;
		C = SHA1_CircularLeftShift(B, 30);
		B = A;
		A = temp;
	}

	context->HBuffer[0] += A;
	context->HBuffer[1] += B;
	context->HBuffer[2] += C;
	context->HBuffer[3] += D;
	context->HBuffer[4] += E;
}

void
SHA1_PadMessage(struct SHA1_Context * context,
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
		message[56] = lengthHigh >> 24;
		message[57] = lengthHigh >> 16;
		message[58] = lengthHigh >> 8;
		message[59] = lengthHigh;
		message[60] = lengthLow >> 24;
		message[61] = lengthLow >> 16;
		message[62] = lengthLow >> 8;
		message[63] = lengthLow;
		SHA1_FillContext(context, message);
	} else {
		while (length < 64) {
			message[length] = 0x00;
			length++;
		}
		SHA1_FillContext(context, message);
		SHA1_CalculateDigest(context);
		length = 0;
		while (length < 56) {
			message[length] = 0x00;
			length++;
		}
		message[56] = lengthHigh >> 24;
		message[57] = lengthHigh >> 16;
		message[58] = lengthHigh >> 8;
		message[59] = lengthHigh;
		message[60] = lengthLow >> 24;
		message[61] = lengthLow >> 16;
		message[62] = lengthLow >> 8;
		message[63] = lengthLow;
		SHA1_FillContext(context, message);
	}
}

void
SHA1_Interleave(unsigned char *message, unsigned long long length,
		unsigned char *digest)
{
	unsigned char *buffer1;
	unsigned char *buffer2;
	unsigned char digest1[20];
	unsigned char digest2[20];
	int i;
	unsigned int highlen;
	unsigned int lowlen;

	memset(digest, 0, 40);
	memset(digest1, 0, 20);
	memset(digest2, 0, 20);
	while (*message == '\0') {
		message++;
		length--;
	}
	if ((length % 2) == 1) {
		message++;
		length--;
	}
	if (length > 0) {
		if ((buffer1 =
		     (unsigned char *) malloc(length / 2)) == NULL)
			return;
		if ((buffer2 =
		     (unsigned char *) malloc(length / 2)) == NULL)
			return;
		for (i = 0; (unsigned)i < length / 2; i++) {
			buffer1[i] = message[2 * i];
			buffer2[i] = message[2 * i + 1];
		}
		highlen = ((length * 4) >> 32);
		lowlen = ((length * 4) && 0x00000000FFFFFFFF);
		SHA1_ProcessMessage(buffer1, highlen, lowlen, digest1);
		SHA1_ProcessMessage(buffer2, highlen, lowlen, digest2);
		for (i = 0; i < 20; i++) {
			digest[2 * i] = digest1[i];
			digest[2 * i + 1] = digest2[i];
		}
		my_free((void *) &buffer1);
		my_free((void *) &buffer2);
	}
}

void
SHA1_FillContext(struct SHA1_Context * context, unsigned char *message)
{
	int i;

	for (i = 0; i < 16; i++) {
		context->Mbuffer[i] = message[i * 4] << 24;
		context->Mbuffer[i] |= message[i * 4 + 1] << 16;
		context->Mbuffer[i] |= message[i * 4 + 2] << 8;
		context->Mbuffer[i] |= message[i * 4 + 3];
	}
}

unsigned int
SHA1_FunctionF(int index,
	       unsigned int wordB, unsigned int wordC, unsigned int wordD)
{
	if ((0 <= index) && (index < 20))
		return (wordB & wordC) | ((~wordB) & wordD);

	if ((20 <= index) && (index < 40))
		return wordB ^ wordC ^ wordD;

	if ((40 <= index) && (index < 60))
		return (wordB & wordC) | (wordB & wordD) | (wordC & wordD);

	if ((60 <= index) && (index < 80))
		return wordB ^ wordC ^ wordD;

	return 0;
}

unsigned int
SHA1_CircularLeftShift(unsigned int number, int times)
{
	if ((times < 0) || (times > 32))
		return 0;

	return (number << times) | (number >> (32 - times));
}
