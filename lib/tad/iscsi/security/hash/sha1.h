/*	/security/chap/sha1.h	*/

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

#ifndef SHA1_H
#define SHA1_H

struct SHA1_Context {
	unsigned int HBuffer[5];
	unsigned int Mbuffer[16];
};

void SHA1_ProcessMessage(unsigned char *message,
			 unsigned int lengthHigh,
			 unsigned int lengthLow, unsigned char *digest);

struct SHA1_Context *SHA1_InitializeContext(void);

void SHA1_CalculateDigest(struct SHA1_Context * context);

void SHA1_Interleave(unsigned char *message, unsigned long long length,
		     unsigned char *digest);

void SHA1_PadMessage(struct SHA1_Context * context,
		     int length,
		     unsigned char *message,
		     unsigned int lengthHigh, unsigned int lengthLow);

void SHA1_FillContext(struct SHA1_Context * context, unsigned char *message);

unsigned int SHA1_FunctionF(int index,
			    unsigned int wordB,
			    unsigned int wordC, unsigned int wordD);

unsigned int SHA1_CircularLeftShift(unsigned int number, int times);

#endif
