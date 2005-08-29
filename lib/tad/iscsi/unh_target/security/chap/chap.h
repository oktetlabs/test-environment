/*	security/chap/chap.h

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

#ifndef CHAP_H
#define CHAP_H

#define NUMBER_OF_ALGORITHMS		0x02
#define MD5_ALGORITHM				0x05
#define SHA1_ALGORITHM				0x07

#include "../misc/misc_func.h"

struct CHAP_Context {
	int number_format;
	int hash_algorithm;
	char *name;
	char *secret;
	unsigned char identifier;
	struct dataunit challenge;
	struct dataunit response;
} ;


struct CHAP_Context *CHAP_InitializeContext(void);

struct CHAP_Context *CHAP_CloneContext(struct CHAP_Context * ctx);

int CHAP_FinalizeContext(struct CHAP_Context *context);

void CHAP_PrintContext(struct CHAP_Context *context);

int CHAP_SetChallengeLength(int clength,struct CHAP_Context *context);

int CHAP_SetNumberFormat(int format,struct CHAP_Context *context);

/*	Searches built-in "hash_algorithms[]" table to find "algorithm".
	If found, sets value in "hash_algorithm" field of "context" and returns 1.
	If not found, returns 0.
*/
int CHAP_SetAlgorithm(int algorithm,struct  CHAP_Context * context);

int CHAP_SetName(char *name,struct  CHAP_Context * context);

char *CHAP_GetName(struct CHAP_Context * context);

int CHAP_SetSecret(char *secret,struct CHAP_Context * context);

int CHAP_SetIdentifier(unsigned char identifier,struct CHAP_Context * context);

unsigned char CHAP_GetIdentifier(struct CHAP_Context * context);

char *CHAP_GetChallenge(struct CHAP_Context * context);

char *CHAP_GetResponse(unsigned char identifier,
		       char *challenge,
		       int max_challenge_length,struct  CHAP_Context * context);

int CHAP_CheckResponse(char *response, int max_response_length,
		      struct  CHAP_Context * context);

/*	called to check a received "challenge" to see if it duplicates 
* 	a previous challenge in this "context".
*	Returns 1 if challenge is not a duplicate; else 0 on duplicate or error.
*/
int CHAP_CheckChallenge(char *challenge,struct CHAP_Context * context);

/*	Searches "list" to find first algorithm number we support.
	If found, return than number.
	If not found, return -1.
*/
int CHAP_SelectAlgorithm(char *list);

/*	Builds list of available CHAP hash algorithms as a character string
	starting at "list".  Returns strlen(final list).
*/
int CHAP_GetAlgorithmList(char *list);

#endif
