/*	chap/chap.c

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stddef.h>

#include <my_memory.h>
#include <iscsi_common.h>
#include "../common/debug.h"

#include "sha1.h"
#include "md5.h"
#include "chap.h"

static int hash_algorithms[] = { MD5_ALGORITHM, SHA1_ALGORITHM };

void
CHAP_PrintContext(struct CHAP_Context * context)
{
	printf("**************************************************\n");
	printf("the current chap context:\n");
	// algorithm
	printf("hash function :");
	switch (context->hash_algorithm) {
	case -1:
		printf("not defined\n");
		break;
	case MD5_ALGORITHM:
		printf("MD5\n");
		break;
	case SHA1_ALGORITHM:
		printf("SHA1\n");
		break;
	default:
		printf("unknown algorithm\n");
		break;
	}
	// name
	printf("name :");
	if (context->name == NULL)
		printf("not defined\n");
	else
		printf("%s\n", context->name);
	// secret
	printf("secret :");
	if (context->secret == NULL)
		printf("not defined\n");
	else
		printf("%s\n", context->secret);
	printf("identifier :%d\n", context->identifier);
	printf("challenge :");
	PrintDataUnit(&context->challenge);
	printf("response :");
	PrintDataUnit(&context->response);
}

struct CHAP_Context *
CHAP_InitializeContext(void)
{
	struct CHAP_Context *context;

	context =
	    (struct CHAP_Context *) malloc(sizeof (struct CHAP_Context));
	if (context == NULL)
		return NULL;

	/* RDR start */
	/* most fields set to 0 or NULL */
	memset(context, 0, sizeof (struct CHAP_Context));
	/* RDR end */

	context->number_format = HEX_FORMAT;
	context->hash_algorithm = -1;

	return context;
}

struct CHAP_Context *
CHAP_CloneContext(struct CHAP_Context * ctx)
{
	struct CHAP_Context *new_ctx;
	int length;

	if ((new_ctx = CHAP_InitializeContext()) == NULL)
		return NULL;

	if (ctx == NULL)
		return new_ctx;

	new_ctx->number_format = ctx->number_format;
	new_ctx->hash_algorithm = ctx->hash_algorithm;

	if (ctx->name != NULL) {
		length = strlen(ctx->name);
		if ((new_ctx->name =
		     (char *) malloc(length + 1)) != NULL) {
			/* RDR start */
			strcpy(new_ctx->name, ctx->name);
			/* RDR end */
		}

	} else
		new_ctx->name = NULL;

	if (ctx->secret != NULL) {
		length = strlen(ctx->secret);
		if ((new_ctx->secret =
		     (char *) malloc(length + 1)) != NULL) {
			/* RDR start */
			strcpy(new_ctx->secret, ctx->secret);
			/* RDR end */
		}
	} else
		new_ctx->secret = NULL;

	new_ctx->challenge.length = ctx->challenge.length;

	return new_ctx;
}

int
CHAP_FinalizeContext(struct CHAP_Context * context)
{
	if (context != NULL) {
		free(context->challenge.data);
		free(context->response.data);
		free(context->name);
		free(context->secret);
		free(context);
	}
	return 1;
}

int
CHAP_SetNumberFormat(int format,struct CHAP_Context * context)
{
	if (context == NULL)
		return 0;
	if ((format == HEX_FORMAT) || (format == BASE64_FORMAT)) {
		context->number_format = format;
		return 1;
	} else
		return 0;
}

int
CHAP_SetChallengeLength(int clength,struct CHAP_Context * context)
{
	if (context == NULL)
		return 0;
	if (clength > 0) {
		context->challenge.length = clength;
		return 1;
	} else
		return 0;
}

int
CHAP_GetChallengeLength(struct CHAP_Context * context)
{
	if (context == NULL)
		return 0;
	return context->challenge.length;
}


/*	Searches built-in "hash_algorithms[]" table to find "algorithm".
 *	If found, sets value in "hash_algorithm" field of 
 * 	"context" and returns 1.
 *	If not found, returns 0.
 */
int
CHAP_SetAlgorithm(int algorithm,struct CHAP_Context * context)
{
	int i;

	if (context == NULL)
		return 0;
	for (i = 0; i < NUMBER_OF_ALGORITHMS; i++) {
		if (algorithm == hash_algorithms[i]) {
			context->hash_algorithm = algorithm;
			return 1;
		}
	}
	return 0;
}

int
CHAP_SetName(char *name,struct CHAP_Context * context)
{
	int length;

	if (context == NULL)
		return 0;
	if (name == NULL)
		return 0;
	free(context->name);
	length = strlen(name);
	context->name = (char *) malloc(length + 1);
	if (context->name == NULL)
		return 0;
	strcpy(context->name, name);
	return 1;
}

char *
CHAP_GetName(struct CHAP_Context * context)
{
	char *name;
	int len;

	if (context == NULL || context->name == NULL)
		return NULL;
	len = strlen(context->name);
	if ((name = (char *) malloc(len + 1)) == NULL)
		return NULL;
	strcpy(name, context->name);
	return name;
}

int
CHAP_SetSecret(char *secret,struct CHAP_Context * context)
{
	int length;

	if (context == NULL)
		return 0;
	if (secret == NULL)
		return 0;
	free(context->secret);
	length = strlen(secret);
	context->secret = (char *) malloc(length + 1);
	if (context->secret == NULL)
		return 0;
	strcpy(context->secret, secret);
	return 1;
}

char *
CHAP_GetSecret(struct CHAP_Context * context)
{
	char *name;
	int len;

	if (context == NULL || context->secret == NULL)
		return NULL;
	len = strlen(context->secret);
	if ((name = (char *) malloc(len + 1)) == NULL)
		return NULL;
	strcpy(name, context->secret);
	return name;
}


int
CHAP_SetIdentifier(unsigned char identifier,struct CHAP_Context * context)
{
	if (context == NULL)
		return 0;
	context->identifier = identifier;
	return 1;
}

unsigned char
CHAP_GetIdentifier(struct CHAP_Context * context)
{
	if (context == NULL)
		return 0;
	RandomNumberGenerate(&context->identifier, 1);
	return context->identifier;
}

char *
CHAP_GetChallenge(struct CHAP_Context * context)
{
	char *temp;
	int length = 0;

	if ((context == NULL) || (context->challenge.length == 0))
		return NULL;

	context->challenge.data = (char *)
	    malloc(context->challenge.length);
	if (context->challenge.data == NULL)
		return NULL;

	RandomNumberGenerate(context->challenge.data,
			     context->challenge.length);
	length =
	    IntegerToStringLength(context->challenge.length,
				  context->number_format);
	temp = (char *) malloc(length);
	if (temp == NULL)
		return NULL;
	IntegerToString(context->challenge.data,
			context->challenge.length, temp,
			context->number_format);
	return temp;
}

/*	
 * generates a hash response to "challenge" using "identifier" and the
 * secret in "context".  The result is stored in dynamically malloced
 * space, and is a printable string in either hex or base64, depending
 * on the format in the "context".  The algorithm used also is in the
 * "context".
*/
char *
CHAP_GetResponse(unsigned char identifier,
		 char *challenge,
		 int max_challenge_length,struct CHAP_Context * context)
{
	int length;
	char *temp;
	char *response_data;
	int hashlen;
	int secretlen;

	if (context == NULL)
		return NULL;
	if (context->secret == NULL)
		return NULL;
	if (challenge == NULL)
		return NULL;

	TRACE(TRACE_DEBUG, "CHAP_GetResponse number format %s",
	      context->number_format == BASE64_FORMAT ? "BASE64" :
	      context->number_format == HEX_FORMAT ? "HEX" : "Unknown");

	length = StringToIntegerLength(challenge);
	if (length > max_challenge_length) {
		TRACE_ERROR("CHAP_C binary length is %d, limit is %d\n", length,
			    max_challenge_length);
		return NULL;
	}
	secretlen = strlen(context->secret);
	length += 1 + secretlen;
	temp = (char *) malloc(length);
	if (temp == NULL)
		return NULL;
	temp[0] = identifier;
	memcpy(temp + 1, context->secret, secretlen);
	StringToInteger(challenge, temp + 1 + secretlen);
	if (context->hash_algorithm == MD5_ALGORITHM) {
		hashlen = 16;
		response_data = (char *) malloc(hashlen);
		if (response_data == NULL) {
			free(temp);
			return NULL;
		}
		MD5_ProcessMessage(temp, 0, length * 8, response_data);
	} else if (context->hash_algorithm == SHA1_ALGORITHM) {
		hashlen = 20;
		response_data = (char *) malloc(hashlen);
		if (response_data == NULL) {
			free(temp);
			return NULL;
		}
		SHA1_ProcessMessage(temp, 0, length * 8, response_data);
	} else {
		free(temp);
		return NULL;
	}
	free(temp);
	length = IntegerToStringLength(hashlen, context->number_format);
	temp = (char *) malloc(length);
	if (temp != NULL) {
		IntegerToString(response_data, hashlen, temp,
				context->number_format);
	}
	free(response_data);
	return temp;
}

/*	
 * called to check a received "response" from a previous challenge in this
 * context".
 * Returns 1 if response is correct; else 0
*/
int
CHAP_CheckResponse(char *response, int max_response_length,
		  struct CHAP_Context * context)
{
	/* RDR start */
	int retval;
	/* RDR end */
	int length;
	char *temp;
	char *response_data;
	int hashlen;
	int secretlen;
	/* RDR start **
	   MD5_Context * md5ctx;
	   SHA1_Context * sha1ctx;
	   ** RDR end */

	if (context == NULL)
		return 0;
	if (response == NULL)
		return 0;

	/*      check to be sure a challenge was previously generated */
	if ((context->challenge.length == 0) ||
	    (context->challenge.data == NULL) || (context->secret == NULL))
		return 0;
	secretlen = strlen(context->secret);
	length = 1 + secretlen + context->challenge.length;
	temp = (char *) malloc(length);
	if (temp == NULL)
		return 0;
	temp[0] = context->identifier;
	memcpy(temp + 1, context->secret, secretlen);
	memcpy(temp + 1 + secretlen,
	       context->challenge.data, context->challenge.length);
	if (context->hash_algorithm == MD5_ALGORITHM) {
		hashlen = 16;
		response_data = (char *) malloc(hashlen);
		if (response_data == NULL) {
			free(temp);
			return 0;
		}
		/* RDR start **
		   md5ctx = MD5_InitializeContext();
		   ** RDR end */
		MD5_ProcessMessage(temp, 0, length * 8, response_data);
	} else if (context->hash_algorithm == SHA1_ALGORITHM) {
		hashlen = 20;
		response_data = (char *) malloc(hashlen);
		if (response_data == NULL) {
			free(temp);
			return 0;
		}
		/* RDR start **
		   sha1ctx = SHA1_InitializeContext();
		   ** RDR end */
		SHA1_ProcessMessage(temp, 0, length * 8, response_data);
	} else {
		free(temp);
		return 0;
	}
	free(temp);

	length = StringToIntegerLength(response);
	if (length > max_response_length) {
		TRACE_ERROR("CHAP_R binary length is %d, limit is %d\n", length,
			    max_response_length);
		free(response_data);
		return 0;
	}
	temp = (char *) malloc(length);
	if (temp == NULL) {
		free(response_data);
		return 0;
	}
	StringToInteger(response, temp);
	retval = IntegerCompare(response_data, hashlen, temp, length);
	free(response_data);
	free(temp);
	return retval;
}

/*	
* called to check a received "challenge" to see if it duplicates a previous
* challenge in this "context".
* Returns 1 if challenge is not a duplicate; else 0 on duplicate or error.
*/
int
CHAP_CheckChallenge(char *challenge,struct CHAP_Context * context)
{
	int retval;
	int length;
	char *temp;

	if (context == NULL)
		return 0;
	if (challenge == NULL)
		return 0;

	/*      check to be sure a challenge was previously generated */
	if ((context->challenge.length == 0) ||
	    (context->challenge.data == NULL) || (context->secret == NULL))
		return 0;

	length = StringToIntegerLength(challenge);
	temp = (char *) malloc(length);
	if (temp == NULL) {
		return 0;
	}
	StringToInteger(challenge, temp);
	retval =
	    !IntegerCompare(context->challenge.data, context->challenge.length,
			    temp, length);
	free(temp);
	return retval;
}

/*	Searches "list" to find first algorithm number we support.
	If found, return than number.
	If not found, return -1.
*/
int
CHAP_SelectAlgorithm(char *list)
{
	int i, n;
	char *comma, *ptr;

	if (list == NULL)
		return -1;

	while (*list != '\0') {	
		/* find next comma-terminated item in remaining list */
		comma = strchr(list, ',');
		if (comma != NULL) {
			/* replace ',' with nul to isolate item */
			*comma++ = '\0';	
		}

		/* convert the isolated item to a number */
		n = strtoul(list, &ptr, 0);

		if (*ptr == '\0') {
			/* this algorithm number has correct 
			 * format, search table of
			 * known hash algorithms 
			 */
			for (i = 0; i < NUMBER_OF_ALGORITHMS; i++) {
				if (n == hash_algorithms[i]) {
					/* found a matching algorithm 
					 * number, accept it 
					 */
					return n;
				}
			}
		}

		/* if loop finishes, we do not support this algorithm number */
		if (comma != NULL)
			list = comma;
		else
			break;
	}

	/*      if loop ends, did not find any algorithms we like in list */
	return -1;
}

/*	
 * Builds list of available CHAP hash algorithms as a character string
 *starting at "list".  Returns strlen(final list).
*/
int
CHAP_GetAlgorithmList(char *list)
{
	int i;
	char *base;

	if ((base = list) == NULL)
		return 0;

	for (i = 0; i < NUMBER_OF_ALGORITHMS; i++) {
		if (i != 0)
			*list++ = ',';
		list += sprintf(list, "%d", hash_algorithms[i]);
	}

	return list - base;
}
