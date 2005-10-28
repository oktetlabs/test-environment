/*	chap/misc_func.c

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

#include <stdlib.h>
#include <stdio.h>
#include <linux/kernel.h>
#include <string.h>
#include <asm/types.h>
#include <linux/stddef.h>
#include <linux/random.h>

#include <linux/spinlock.h>

#include "misc_func.h"

static char hexcode[] = { '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static char base64code[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/',
	'='
};

void
PrintDataUnit(struct dataunit * du)
{
	int i;

	if (du->data == NULL)
		printf("null data (with length %d)\n", du->length);
	else {
		for (i = 0; i < du->length; i++)
			printf("%02x", (unsigned char) du->data[i]);
		printf("(with length %d)\n", du->length);
	}
}

/*	returns number of bytes needed to encode a binary integer with "intlen"
	bytes into a printable string in the specified "format".
	- when format is hex:	 every 4 binary bits takes 1 printable byte
	- when format is base64: every 6 binary bits takes 1 printable byte
	In both cases, there is always a need for 3 extra bytes to store the
	prefix ("0x" or "0b") and the final nul ('\0').
*/
int
IntegerToStringLength(int intlen, int format)
{
	if (format == HEX_FORMAT)
		return intlen * 2 + 3;
	else if (format == BASE64_FORMAT)
		return ((intlen + 2) / 3) * 4 + 3;
	else
		return -1;
}

/* returns number of bytes needed to store a binary integer that is encoded
in the printable string "str".
if "str" is in hex, binary integer needs 4 bits for every byte in "str".
if "str" is in base64 binary integer needs 6 bits for every byte in "str",
	but if last 2 characters in "str" are "=" then last 16 bits not stored
	and if last 1 character in "str" is "=" then last 8 bits are not stored.
*/
int
StringToIntegerLength(char *str)
{
	int len;

	if (str == NULL)
		return -1;
	len = strlen(str);
	if (len == 0)
		return -1;
	if ((len > 2) && (str[0] == '0')) {
		if ((str[1] == 'x') || (str[1] == 'X')) {
			/* hex representation */
			return (len - 1) / 2;
		} else if ((str[1] == 'b') || (str[1] == 'B')) {
			/* base 64 representation */
			if ((len % 4) != 2)
				return -1;
			if (str[len - 1] == '=') {
				if (str[len - 2] == '=')
					return ((len - 2) / 4 * 3) - 2;
				else
					return ((len - 2) / 4 * 3) - 1;
			} else
				return ((len - 2) / 4 * 3);
		} else
			return -1;
	} else
		return -1;
}

void
IntegerToString(unsigned char *intnum, long length, char *string, int format)
{
	if ((intnum == NULL) || (string == NULL) || (length == 0))
		return;
	if (format == HEX_FORMAT) {
		string[0] = '0';
		string[1] = 'x';
		IntegerToHexString(intnum, length, string + 2);
	} else if (format == BASE64_FORMAT) {
		string[0] = '0';
		string[1] = 'b';
		IntegerToBase64String(intnum, length, string + 2);
	}
}

void
StringToInteger(char *string, char *intnum)
{
	char ch;
	int len;

	if ((intnum == NULL) || (string == NULL))
		return;
	len = strlen(string);
	if (len == 0)
		return;
	ch = string[0];
	if ((len > 2) && (ch == '0')) {
		ch = string[1];
		if ((ch == 'x') || (ch == 'X'))
			HexStringToInteger(string + 2, intnum);
		else if ((ch == 'b') || (ch == 'B'))
			Base64StringToInteger(string + 2, intnum);
	}
}

void
IntegerToHexString(unsigned char *intnum, long length, char *string)
{
	unsigned char ch = '\0';
	int ptrstr = 0;
	int ptrint = 0;

	if ((intnum == NULL) || (string == NULL) || (length == 0))
		return;
	while (ptrint < length && (ch = intnum[ptrint]) == '\0')
		ptrint++;
	if (length <= ptrint) {	/* intnum is all zero bits */
		string[0] = '0';
		string[1] = '\0';
	} else {
		if ((ch & 0xF0) == 0) {
			string[ptrstr] = hexcode[ch];
			ptrstr++;
		} else {
			string[ptrstr] = hexcode[ch >> 4];
			ptrstr++;
			string[ptrstr] = hexcode[ch & 0x0F];
			ptrstr++;
		}
		ptrint++;
		while (ptrint < length) {
			ch = intnum[ptrint];
			string[ptrstr] = hexcode[ch >> 4];
			ptrstr++;
			string[ptrstr] = hexcode[ch & 0x0F];
			ptrstr++;
			ptrint++;
		}
		string[ptrstr] = '\0';
	}
}

void
HexStringToInteger(char *string, char *intnum)
{
	int i;
	char ch1;
	char ch2;
	int len;

	if ((string == NULL) || (intnum == NULL))
		return;
	len = strlen(string);
	if (len == 0)
		return;
	i = (len - 1) / 2 + 1;
	while (len > 1) {
		ch1 = string[len - 1];
		ch2 = string[len - 2];
		len -= 2;
		if ((ch1 >= '0') && (ch1 <= '9'))
			ch1 -= '0';
		else if ((ch1 >= 'A') && (ch1 <= 'F'))
			ch1 = ch1 - 'A' + 10;
		else if ((ch1 >= 'a') && (ch1 <= 'f'))
			ch1 = ch1 - 'a' + 10;
		else
			ch1 = 0;
		if ((ch2 >= '0') && (ch2 <= '9'))
			ch2 -= '0';
		else if ((ch2 >= 'A') && (ch2 <= 'F'))
			ch2 = ch2 - 'A' + 10;
		else if ((ch2 >= 'a') && (ch2 <= 'f'))
			ch2 = ch2 - 'a' + 10;
		else
			ch2 = 0;
		intnum[i - 1] = (ch2 << 4) | ch1;
		i--;
	}
	if (len == 1) {
		ch1 = string[0];
		if ((ch1 >= '0') && (ch1 <= '9'))
			ch1 -= '0';
		else if ((ch1 >= 'A') && (ch1 >= 'F'))
			ch1 = ch1 - 'A' + 10;
		else if ((ch1 >= 'a') && (ch1 <= 'f'))
			ch1 = ch1 - 'a' + 10;
		else
			ch1 = 0;
		intnum[i - 1] = ch1;
	}
}

/** FIXME: this function must be rewritten to
    something more adequate
**/
void
get_random_bytes(char *buf, size_t len)
{

    size_t  i;

    for (i = 0; i < len; ++i)
    {
        buf[i] = (rand() >> 4) & 0xFF;
    }
}

/*	stores "length" randomly generated bytes in "number" */
void
RandomNumberGenerate(char *number, int length)
{
	if (number == NULL)
		return;
	else
		get_random_bytes(number, length);
}

/*	Returns 1 if integers len1 == len2 and strings int1 == int2; else 0 */
int
IntegerCompare(char *int1, int len1, char *int2, int len2)
{
	int i;

	if ((int1 == NULL) || (int2 == NULL))
		return 0;
	if (len1 != len2)
		return 0;
	for (i = 0; i < len1; i++) {
		if (int1[i] != int2[i])
			return 0;
	}
	return 1;
}

void
IntegerToBase64String(unsigned char *intnum, long length, char *string)
{
	int count;
	int octets;
	int strptr;
	int delta;

	if ((intnum == NULL) || (string == NULL) || (length == 0))
		return;
	count = 0;
	octets = 0;
	strptr = 0;
	while ((delta = (length - count)) > 2) {
		octets =
		    (intnum[count] << 16) | (intnum[count + 1] << 8) |
		    intnum[count + 2];
		string[strptr] = base64code[(octets & 0xFC0000) >> 18];
		string[strptr + 1] = base64code[(octets & 0x03F000) >> 12];
		string[strptr + 2] = base64code[(octets & 0x000FC0) >> 6];
		string[strptr + 3] = base64code[octets & 0x00003F];
		count += 3;
		strptr += 4;
	}
	if (delta == 1) {
		string[strptr] = base64code[(intnum[count] & 0xFC) >> 2];
		string[strptr + 1] = base64code[(intnum[count] & 0x03) << 4];
		string[strptr + 2] = base64code[64];
		string[strptr + 3] = base64code[64];
		strptr += 4;
	} else if (delta == 2) {
		string[strptr] = base64code[(intnum[count] & 0xFC) >> 2];
		string[strptr + 1] = base64code[((intnum[count] & 0x03) << 4) |
						((intnum[count + 1] & 0xF0) >>
						 4)];
		string[strptr + 2] =
		    base64code[(intnum[count + 1] & 0x0F) << 2];
		string[strptr + 3] = base64code[64];
		strptr += 4;
	}
	string[strptr] = '\0';
}

char
Base64codeToNumber(char base64)
{
	switch (base64) {
	case '=':
		return 64;
	case '/':
		return 63;
	case '+':
		return 62;
	default:
		if ((base64 >= 'A') && (base64 <= 'Z'))
			return base64 - 'A';
		else if ((base64 >= 'a') && (base64 <= 'z'))
			return 26 + (base64 - 'a');
		else if ((base64 >= '0') && (base64 <= '9'))
			return 52 + (base64 - '0');
		else
			return -1;
	}
}

void
Base64StringToInteger(char *string, char *intnum)
{
	int len;
	int count;
	int intptr;
	char num[4];
	int octets;

	if ((string == NULL) || (intnum == NULL))
		return;
	len = strlen(string);
	if (len == 0)
		return;
	if ((len % 4) != 0)
		return;
	count = 0;
	intptr = 0;
	while (count < len - 4) {
		num[0] = Base64codeToNumber(string[count]);
		num[1] = Base64codeToNumber(string[count + 1]);
		num[2] = Base64codeToNumber(string[count + 2]);
		num[3] = Base64codeToNumber(string[count + 3]);
		if ((num[0] < 0) || (num[0] == 65) ||
		    (num[1] < 0) || (num[1] == 65) ||
		    (num[2] < 0) || (num[2] == 65) ||
		    (num[3] < 0) || (num[3] == 65))
			return;
		count += 4;
		octets =
		    (num[0] << 18) | (num[1] << 12) | (num[2] << 6) | num[3];
		intnum[intptr] = (octets & 0xFF0000) >> 16;
		intnum[intptr + 1] = (octets & 0x00FF00) >> 8;
		intnum[intptr + 2] = octets & 0x0000FF;
		intptr += 3;
	}
	num[0] = Base64codeToNumber(string[count]);
	num[1] = Base64codeToNumber(string[count + 1]);
	num[2] = Base64codeToNumber(string[count + 2]);
	num[3] = Base64codeToNumber(string[count + 3]);
	if ((num[0] < 0) || (num[1] < 0) ||
	    (num[2] < 0) || (num[3] < 0) || (num[0] == 64) || (num[1] == 64))
		return;
	if (num[2] == 64) {
		if (num[3] != 64)
			return;
		intnum[intptr] = (num[0] << 2) | (num[1] >> 4);
	} else if (num[3] == 64) {
		intnum[intptr] = (num[0] << 2) | (num[1] >> 4);
		intnum[intptr + 1] = (num[1] << 4) | (num[2] >> 2);
	} else {
		octets =
		    (num[0] << 18) | (num[1] << 12) | (num[2] << 6) | num[3];
		intnum[intptr] = (octets & 0xFF0000) >> 16;
		intnum[intptr + 1] = (octets & 0x00FF00) >> 8;
		intnum[intptr + 2] = octets & 0x0000FF;
	}
}
