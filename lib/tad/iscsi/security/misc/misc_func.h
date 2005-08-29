/*	swcurity/misc/misc_func.h

	vi: set autoindent tabstop=8 shiftwidth=4 :

*/
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

#ifndef MISC_FUNC_H
#define MISC_FUNC_H

#include <stdlib.h>
#include <string.h>

#define HEX_FORMAT			0x01
#define BASE64_FORMAT		0x02

struct dataunit {
	int length;
	char *data;
};

struct userinfo {
	char *name;
	char *secret;
	struct userinfo *next;
};

void PrintDataUnit(struct dataunit *du);

int IntegerToStringLength(int intlen, int format);

int StringToIntegerLength(char *str);

void IntegerToString(unsigned char *intnum,
		     long length, char *string, int format);

void StringToInteger(char *string, char *intnum);

void IntegerToHexString(unsigned char *intnum, long length, char *string);

void HexStringToInteger(char *string, char *intnum);

void IntegerToBase64String(unsigned char *intnum, long length, char *string);

void Base64StringToInteger(char *string, char *intnum);

int IntegerCompare(char *int1, int len1, char *int2, int len2);

void RandomNumberGenerate(char *number, int length);

/* user database functions, newly added */

int AddUserInfo(char *name, char *secret);

char *GetUserSecret(char *name);

void CleanAllUserInfo(void);

#endif
