/* 	common/text_param.c
 *
 * 	vi: set autoindent tabstop=8 shiftwidth=8 :
 * 
 *  	This defines the generic functions used in the Login phase
 *  	by both the iSCSI target and the iSCSI initiator for 
 *  	parameter negotiation.
 *
 *	Copyright (C) 2001-2004 InterOperability Lab (IOL)
 *	University of New Hampshire (UNH)
 *	Durham, NH 03824
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2, or (at your option)
 *	any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *	USA.
 *
 *	The name IOL and/or UNH may not be used to endorse or promote 
 *	products derived from this software without specific prior 
 * 	written permission.
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "my_memory.h"
#include "iscsi_common.h"
#include "debug.h"

/* chap and srp support -CHONG */
#include "../security/chap.h"
#include "../security/srp.h"

#include "text_param.h"


static struct key_values upper_case_values = {
	none:NONE,
	yes:YES,
	no:NO,
	reject:REJECT,
	irrelevant:IRRELEVANT,
	discovery:DISCOVERY,
	normal:NORMAL,
	notunderstood:NOTUNDERSTOOD
};

static struct key_values *key_table = &upper_case_values;

/*
 * deal with security keysi
 */ 
struct security_key_struct {
	char *name;
	uint32_t bitmask;
};

static struct security_key_struct security_key[] = {
	{CHAP_A, GOT_CHAP_A},
	{CHAP_I, GOT_CHAP_I},
	{CHAP_C, GOT_CHAP_C},
	{CHAP_N, GOT_CHAP_N},
	{CHAP_R, GOT_CHAP_R},
	{SRP_GROUP, GOT_SRP_GROUP},
	{SRP_S, GOT_SRP_S},
	{SRP_B, GOT_SRP_B},
	{SRP_HM, GOT_SRP_HM},
	{SRP_U, GOT_SRP_U},
	{SRP_TARGETAUTH, GOT_SRP_TARGETAUTH},
	{SRP_A, GOT_SRP_A},
	{SRP_M, GOT_SRP_M},
	{NULL, 0}
};

#define	SECURITY_HASH_BITS	5
#define	SECURITY_HASH_SIZE	(1<<SECURITY_HASH_BITS)
#define	SECURITY_HASH_MASK	(SECURITY_HASH_SIZE - 1)

static struct security_key_struct *security_hash_table[SECURITY_HASH_SIZE];

/*	the "perfect" hash function for all the CHAP and SRP keys */
static uint32_t __attribute__ ((no_instrument_function))
security_hash(char *name)
{
	uint32_t len, hash;

	len = strlen(name);
	hash = name[len - 1] + (len << 1);
	return hash & SECURITY_HASH_MASK;
}

/*	returns 0 on failure, else GOT_XXX value for this key */
uint32_t __attribute__ ((no_instrument_function))
is_securitykey(char *keyname)
{
	uint32_t hash;
	struct security_key_struct *key;

	hash = security_hash(keyname);
	if ((key = security_hash_table[hash]) != NULL &&
	    strcmp(keyname, key->name) == 0) {
		return key->bitmask;
	}
	return 0;
}

/*	returns 0 on failure after printing error message,
 *	else GOT_XXX value for this key
 */
uint32_t __attribute__ ((no_instrument_function))
print_bad_security_key(struct unknown_key *key)
{
	uint32_t result;

	if(!(result = is_securitykey(key->keyname))) {
		TRACE_ERROR("unknown key \"%s\" in security phase\n",
			    key->keyname);
	}
	return result;
}

void __attribute__ ((no_instrument_function))
print_not_allowed_security_key(struct unknown_key *key)
{
	TRACE_ERROR("Security key \"%s\" not allowed in this step\n",
		    key->keyname);
}

/*	called once on module load to setup the security key hash table */
void
setup_security_hash_table(void)
{
	uint32_t hash, count, collides;
	struct security_key_struct *key, *prev_key;

	for (count = collides = 0, key = security_key; key->name;
	     count++, key++) {
		hash = security_hash(key->name);
		if ((prev_key = security_hash_table[hash]) != NULL
		    && strcmp(key->name, prev_key->name)) {
			printf
			    ("Duplicate hash value %u for \"%s\" and \"%s\"",
			     hash, security_hash_table[hash]->name, key->name);
			collides++;
		} else if (prev_key == NULL) {
			TRACE(TRACE_DEBUG, "%2u: Add \"%s\" at hash value %u",
			      count + 1, key->name, hash);
			security_hash_table[hash] = key;
		}
	}
	TRACE(TRACE_DEBUG, "%u hash collisions in %u security keys",
	      collides, count);
}

/*	Called to get table entry for key identified by keytext string.
 *	Returns pointer to entry if found, NULL if not found.
 */
struct parameter_type * __attribute__ ((no_instrument_function))
find_parameter(const char *keytext,
               struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS])
{
	struct parameter_type *p, *result = NULL;
	int i;

	TRACE(TRACE_ENTER_LEAVE, "Enter find_parameter %s", keytext);

	for (i = 0, p = p_param_tbl; i < MAX_CONFIG_PARAMS; i++, p++) {
		if (!strcmp(p->parameter_name, keytext)) {
			result = p;
			break;
		}
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave find_parameter, p %p", result);

	return result;
}

/* Called to get pointer to table entry for key identified by its special flag*/
struct parameter_type * __attribute__ ((no_instrument_function))
find_flag_parameter(uint64_t key_flag,
		    struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS])
{
	struct parameter_type *p, *result = NULL;
	int i;

	for (i = 0, p = p_param_tbl; i < MAX_CONFIG_PARAMS; i++, p++) {
		if (p->special_key_flag & key_flag) {
			result = p;
			break;
		}
	}

	return result;
}

/*	Called to set max_recv_length to value we sent to target in a
 *	MaxRecvDataSegmentLength key
 */
void
set_connection_recv_length(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
			   int *max_recv_length)
{
	struct parameter_type *p;

	TRACE(TRACE_ENTER_LEAVE, "Enter set_connection_recv_length");

	if ((p =
	     find_flag_parameter(MAXRECVDATASEGMENTLENGTH_FLAG, p_param_tbl))
	    && IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)) {
		/* we did send this key to the other side */
		*max_recv_length = p->int_value;
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave set_connection_recv_length");

}

void
set_digestflags(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		uint32_t * connection_flags)
{
	struct parameter_type *p;

	if ((p = find_flag_parameter(HEADERDIGEST_FLAG, p_param_tbl))) {
		if (strcmp(p->str_value, CRC32C) == 0) {
			*connection_flags |= USE_HEADERDIGEST;
			TRACE(TRACE_ISCSI, "Enabling Header Digests");
		}
	}
	if ((p = find_flag_parameter(DATADIGEST_FLAG, p_param_tbl))) {
		if (strcmp(p->str_value, CRC32C) == 0) {
			*connection_flags |= USE_DATADIGEST;
			TRACE(TRACE_ISCSI, "Enabling Data Digests");
		}
	}
}

/*	Check that a number is within legal bounds depending on its type.
 *	Returns 1 if ok, 0 if error
 */
int __attribute__ ((no_instrument_function))
check_bounds(struct parameter_type *p, int int_value, int who_called)
{
	int retval = 1;

	if (IS_ONE_TO_65535(p->type)) {
		if ((int_value < 1) || (int_value > 65535)) {
			TRACE_ERROR("illegal number %d - "
				    "should be between 1 and 65535\n",
				    int_value);
			retval = 0;
		}
	} else if (IS_ZERO_TO_65535(p->type)) {
		if ((int_value < 0) || (int_value > 65535)) {
			TRACE_ERROR("illegal number %d - "
				    "should be between 0 and 65535\n",
				    int_value);
			retval = 0;
		}
	} else if (IS_N512_TO_16777215(p->type)) {
		if ((int_value < 512) || (int_value > 16777215)) {
			if (who_called == MANAGEMENT &&
			    (p->special_key_flag & (MAXBURSTLENGTH_FLAG |
						    FIRSTBURSTLENGTH_FLAG))) {
				/* Burst limits depend on draft, 
					do it at run time */
			} else {
				TRACE_ERROR("illegal number %d - "
					    "should be between 512 and 16777215\n",
					    int_value);
				retval = 0;
			}
		}
	} else if (IS_ZERO_TO_3600(p->type)) {
		if ((int_value < 0) || (int_value > 3600)) {
			TRACE_ERROR("illegal number %d - "
				    "should be between 0 and 3600\n",
				    int_value);
			retval = 0;
		}
	} else if (IS_ZERO_TO_2(p->type)) {
		if ((int_value < 0) || (int_value > 2)) {
			TRACE_ERROR("illegal number %d - "
				    "should be between 0 and 2\n", int_value);
			retval = 0;
		}
	}

	return retval;
}

/*	This is a range, check both numbers and their order.
 *	Returns first number in range if ok, -1 if error
 */
int __attribute__ ((no_instrument_function))
check_range(const char *value_list, int value)
{
	char *endptr;
	int lower, upper;

	lower = strtoul(value_list, &endptr, 0);
	if (*endptr == '~') {
		/* first number followed by ~ must be 
			followed by another number */
		upper = strtoul(endptr + 1, &endptr, 0);
	} else {
		/* only 1 value was specified, which is ok */
		upper = lower;
	}

	if (strspn(endptr, WHITE_SPACE) != strlen(endptr) || lower > upper) {
		TRACE_ERROR("illegal range \"%s\"\n", value_list);
		return -1;
	}
	if (value >= 0 && (value < lower || value > upper)) {
		TRACE_ERROR("value %d out of range \"%s\"\n", value,
			    value_list);
		return -1;
	}

	return lower;
}

/*
 * Checks correctness of the type of parameter with the value passed	
 * The main types are defined in "text_param.h"	
 * If the value is NULL, no checks made.
 * On return, a flag is set in p->neg_info if a fatal error occured
 * Called only from check_correctness
 */

static void __attribute__ ((no_instrument_function))
check_type_correctness(struct parameter_type *p,
		       char *value, int who_called, int *int_value)
{
	char *dummy;
	char *endptr = NULL;

	TRACE(TRACE_ENTER_LEAVE, "Enter check_type_correctness");
	TRACE(TRACE_ENTER_LEAVE, "Parameter: %s, value: %s",
	      p->parameter_name, value);

	/* If the value is inquiry, ignore it */
	if (!strcmp(value, "?"))
		goto out;

	/* look for value = "Reject" or "Irrelevant" or "NotUnderstood" */
	if (!strcmp(value, key_table->reject)) {
		if (!IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)) {
			/* Reject always fatal as an offer */
			TRACE_ERROR("illegal offer: %s=%s\n", p->parameter_name,
				    value);
			p->neg_info |= KEY_BAD;
		} else if (p->
			   special_key_flag & (OFMARKINT_FLAG | IFMARKINT_FLAG))
		{
			/* Reject is not fatal as a reply to 
			 * OFMarkInt/IFMarkInt keys 
			 *  Draft 20, Section A.3.2 OFMarkInt, IFMarkInt
			 *  "When the interval is unacceptable the responder 
			 *  answers with "Reject".  Reject is resetting 
			 *  the marker function in the
			 *  specified direction (Output or Input) to No."
			 */
			/* so we can do a reset later */
			p->neg_info |= KEY_REJECT;	
		} else {
			/* Reject always fatal as a reply to all other keys */
			TRACE_ERROR("got key: %s=%s\n", p->parameter_name,
				    value);
			p->neg_info |= KEY_BAD;
		}
		goto out;
	}

	if (!strcmp(value, key_table->irrelevant)) {
		if (!IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)) {
			/* Irrelevant always fatal as an offer */
			TRACE_ERROR("illegal offer: %s=%s\n", p->parameter_name,
				    value);
			p->neg_info |= KEY_BAD;
		} else {
			/* Irrelevant is a legal reply in all negotiations */
            TRACE_WARNING("got key: %s=%s\n",
                          p->parameter_name, value);
			p->neg_info |= KEY_IRRELEVANT;
		}
		goto out;
	}

	/*  Draft 20, Section 5.2 Text Mode Negotiation
	 *  "All keys in this document, except for the X extension formats, 
	 *  MUST be supported by iSCSI initiators and targets when used 
	 *  as specified here.
	 *  If used as specified, these keys MUST NOT be answered with
	 *  NotUnderstood."
	 */
    if (!strcmp(value, key_table->notunderstood)) {
		if (!IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)) {
			/* NotUnderstood always fatal as an offer */
			TRACE_ERROR("illegal offer: %s=%s\n", p->parameter_name,
				    value);
			p->neg_info |= KEY_BAD;
		} else if ((p->special_key_flag & X_EXTENSIONKEY_FLAG)) {
			/* target not expected to understand this 
				anyway, consider its response irrelevant */
			p->neg_info |= KEY_IRRELEVANT;
		} else {
			/* NotUnderstood always fatal as a reply to 
				a standard key */
			TRACE_ERROR
			    ("standard key \"%s\" MUST NOT be answered with "
			     "\"%s\"\n", p->parameter_name,
			     key_table->notunderstood);
			p->neg_info |= KEY_BAD;
		}
		goto out;
	}

	if (IS_STRING(p->type)) {
		/*  key value is a string, just check its total length */

		/*  Draft 20, Section 5.1 Text Format
		 *  "If not otherwise specified, the maximum length of a 
		 *  simple-value (not its encoded representation) is 255 
		 *  bytes not including the delimiter (comma or zero byte)."
		 *
		 *  Draft 20, Section 3.2.6.1  iSCSI Name Properties
		 *  "Each iSCSI node, whether an initiator or target, 
		 *  MUST have an iSCSI name.
		 *
		 *  Initiators and targets MUST support the receipt of iSCSI
		 *  names of up to the maximum length of 223 bytes. "
		 */
		char *tptr;
		int len = strlen(value);
		int t_num;

		if (len > MAX_KEY_VALUE_LENGTH) {
			TRACE_ERROR
			    ("value of key \"%s\" longer than %d characters\n",
			     p->parameter_name,
			     (IS_ISCSI_NAME(p->type) ? MAX_ISCSI_NAME_LENGTH :
			      MAX_KEY_VALUE_LENGTH));
			p->neg_info |= KEY_BAD;
		}

		/* check ISCSI_NAME type */
		if (IS_ISCSI_NAME(p->type)) {
			if (who_called == TARGET &&
			    p->special_key_flag & TARGETNAME_FLAG) {
				/* check TargetName sent by the initiator */
				p->int_value = 0;	/* assume an error */
				if ((strncmp(TARGETNAME_HEADER, value,
					     strlen(TARGETNAME_HEADER)) != 0)
				    || (tptr = strchr(value, ':')) == NULL) {
					TRACE_WARNING
					    ("invalid %s \"%s\". "
					    "Use Discovery session"
					    " to get proper %s.\n",
					     p->parameter_name, value,
					     p->parameter_name);
					goto out;
				}
				tptr++;		/* point at char after ':' */
				if (strspn(tptr, WHITE_SPACE) == strlen(tptr)) {
					TRACE_ERROR
					    ("no target number in %s \"%s\"\n",
					    p->parameter_name, value);
					p->neg_info |= KEY_BAD;
					goto out;
				}

				t_num = strtoul(tptr, &endptr, 10);
				if (t_num < 0 || *endptr != '\0') {
					TRACE_ERROR
					    ("bad target number \"%s\" "
					    "in %s \"%s\"\n",
					    tptr, p->parameter_name, value);
					p->neg_info |= KEY_BAD;
					goto out;
				}
				/* check target number from the target name */
				if (t_num != 0) {
					TRACE_ERROR
					    ("target number %d not in use "
					     "in %s \"%s\"\n",
					     t_num, p->parameter_name, value);
					p->neg_info |= KEY_BAD;
					goto out;
				}
				/* target number ok, save it for pickup later */
				p->int_value = t_num;
			}

			/*  check the format of the iSCSI name */
			/*  Draft 20, 3.2.6.3  iSCSI Name Structure
			 *  "The type designator strings currently defined are:
			 *      iqn.  - iSCSI Qualified name
			 *      eui.  - Remainder of the string is an 
			 *              IEEE EUI-64 identifier, in 
			 *              ASCII-encoded hexadecimal."
			 */
			if (len == 20 && strncmp(value, "eui.", 4) == 0) {
				/* IEEE format world wide unique name -- 
				   16 hex digits must follow */
				if (strspn(value + 4, "0123456789abcdefABCDEF")
				    == 16)
					/* ok, have exactly 16 hex 
						digits following the "eui." */
					goto out;
			} else if (len > 12 && len <= MAX_ISCSI_NAME_LENGTH &&
				   strncmp(value, "iqn.", 4) == 0) {
				/* iSCSI qualified name -- must get "yyyy-mm." 
				   then alphanums, ., - */
				if (strspn(value + 4, "0123456789") == 4
				    && *(value + 8) == '-'
				    && strspn(value + 9, "0123456789") == 2
				    && *(value + 11) == '.'
				    && (int)strspn(value + 12,
					     "0123456789abcdefghijklmnopqrstuvwxyz-.:") 
                                       == len - 12) {
					/* ok, have iqn.yyyy-mm.xxx with xxx 
					 * all digits, lower-case letters, 
					 * * '-', '.', ':' 
					 */
					goto out;
				}
			}

			/* If reach here have invalid iSCSI name */
            TRACE_WARNING("invalid iSCSI name \"%s\"\n", value);
			goto out;
		}
	}

	/* the following are to be tested for each value in the list    */
	do {
		endptr = NULL;

		/* extract just a single value */
		dummy = strchr(value, ',');
		if (dummy) {
			/* found a comma so value contains list of 
				values -- is this ok? */
			if (!IS_KEY_MULTI_VALUE(p->type)
			    && !(p->special_key_flag & TARGETADDRESS_FLAG)) {
				/* this key cannot accept a list of values */
				TRACE_ERROR
				    ("key \"%s\" does not accept a list of values "
				     "\"%s\"\n", p->parameter_name, value);
				p->neg_info |= KEY_BAD;
				break;
			}
			/* terminate first item in the list so it 
				is a string by itself */
			*dummy = '\0';
		}

		TRACE(TRACE_DEBUG, "value: %s", value);

		/*  Check for length of the key value. */
		/*  Draft 20, Section 5.1 Text Format
		 *  "If not otherwise specified, the maximum length of 
		 *  a simple-value (not its encoded representation) is 
		 *  255 bytes not including the
		 *  delimiter (comma or zero byte)."
		 */
		if (strlen(value) > MAX_KEY_VALUE_LENGTH) {
            TRACE_WARNING("value of key \"%s\" "
                          "exceeds %d characters\n",
                          p->parameter_name,MAX_KEY_VALUE_LENGTH);
		}

		if (*value == '\0' && !IS_KEY_NO_VALUE(p->type)) {
			TRACE_ERROR("no value after '=' for key \"%s\"\n",
				    p->parameter_name);
			p->neg_info |= KEY_BAD;
			break;
		}

		else if (IS_NUMBER(p->type) ||
			 (IS_NUMBER_RANGE(p->type) &&
			  IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info))) {
			/* a number or a reply to a range */

			*int_value = strtoul(value, &endptr, 0);

			if (strspn(endptr, WHITE_SPACE) != strlen(endptr)) {
				TRACE_ERROR("illegal number \"%s\"\n", value);
				p->neg_info |= KEY_BAD;
				break;
			}

			if (!check_bounds(p, *int_value, who_called)) {
				p->neg_info |= KEY_BAD;
				break;
			}
		}

		else if (IS_BOOLEAN(p->type)) {
			if (strcmp(value, key_table->yes)
			    && strcmp(value, key_table->no)) {
				TRACE_ERROR("illegal value \"%s\" - expected "
					    "\"%s\" or \"%s\"\n", value,
					    key_table->yes, key_table->no);
				p->neg_info |= KEY_BAD;
				break;
			}
		}

		else if (IS_ENUMERATED(p->type)) {
			if (IS_AUTH_PARAM(p->type)) {
				if (strcmp(value, KRB5) && strcmp(value, SPKM1)
				    && strcmp(value, SPKM2)
				    && strcmp(value, SRP)
				    && strcmp(value, CHAP)
				    && strcmp(value, key_table->none)) {
					TRACE_ERROR
					    ("illegal value \"%s\" - expected "
					 "\"KRB5\" or "
					 "\"SPKM1\" or \"SPKM2\" or \"SRP\" or "
					     "\"CHAP\" or \"%s\"\n", value,
					     key_table->none);
					p->neg_info |= KEY_BAD;
					break;
				}
			} else if (IS_DISCOVERY_NORMAL(p->type)) {
				if (strcmp(value, key_table->discovery)
				    && strcmp(value, key_table->normal)) {
					TRACE_ERROR
					    ("illegal value \"%s\" - expected "
					     "\"%s\" or \"%s\"\n", value,
					     key_table->discovery,
					     key_table->normal);
					p->neg_info |= KEY_BAD;
					break;
				}
			}
		}

		else if (IS_NUMBER_RANGE(p->type)
			 && !IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)) {
			/* is an offer of a numeric range */
			if ((*int_value = check_range(value, -1)) < 0
			    || !check_bounds(p, *int_value, who_called)) {
				p->neg_info |= KEY_BAD;
				break;
			}
		}

		if (dummy) {
			/* restore the comma and goto the next value */
			*dummy++ = ',';
		}

		value = dummy;
	}
	while (value);

      out:
	TRACE(TRACE_ENTER_LEAVE, "Leave check_type_correctness");

	return;
}

/*
 * Checks the correctness of the passed string, checks whether it
 * conforms to the <key>=<value> format
 * extracts the value from the input string and returns it as an
 * output parameter.
 * Returns pointer to parameter in p_param_tbl if ok, else NULL	
 * (but on non-NULL p->neg_info can have flag set on fatal error)		*/
static struct parameter_type * __attribute__ ((no_instrument_function))
check_correctness(char *keytext,
		  char **p_value,
		  struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		  uint32_t who_called,
		  uint32_t when_called,
		  uint32_t flags,
		  int *int_value, 
		  struct unknown_key **unknown_key_list)
{
	char *value;
	struct parameter_type *p = NULL;
	struct unknown_key *uptr;
	uint32_t count;

	TRACE(TRACE_ENTER_LEAVE, "Enter check_correctness");

	TRACE(TRACE_ISCSI, "Got key: %s", keytext);

	if ((value = strchr(keytext, '=')) == NULL) {
		/* could not find '=', don't accept this */
		TRACE_ERROR("key \"%s\" not followed by '='\n", keytext);
		goto out;
	}

	/* terminate the key name string and point at the value string */
	*value++ = '\0';

	/* Check for length of the key name. */

	/*  Draft 20, Section 5.1 Text Format
	 *  "A standard-lable (key-name) MUST begin with a capital letter and
	 *  must not exceed 63 characters."
	 */
	if (strlen(keytext) > MAX_KEY_NAME_LENGTH) {
        TRACE_WARNING("length of key name \"%s\" exceeds %d\n",
                      keytext, MAX_KEY_NAME_LENGTH);
	}

	/*  Draft 20, Section 10.12.10 Login Parametersr
	 *  "All keys in Chapter 12, except for the X extension formats, 
	 *  MUST be supported by iSCSI initiators and targets.  
	 *  Keys in Chapter 11 only
	 *  need to be supported when the function to which they 
	 *  refer is mandatory
	 *  to implement."
	 */
	if (!(p = find_parameter(keytext, p_param_tbl))) {
		/* key not found in our table, look for 
		   value = "Reject" or "Irrelevant" or "NotUnderstood" */
		if (!strcmp(value, key_table->reject)
		    || !strcmp(value, key_table->irrelevant)
		    || !strcmp(value, key_table->notunderstood)) {
			/* These values always fatal as a value for offer 
			   of unknown key */
			TRACE_ERROR("unknown key with illegal value: %s=%s\n",
				    keytext, value);
		} else {
			/* value not bad, but have we seen this 
				unknown key before? */
			for (count = 0, uptr = *unknown_key_list;
			     uptr != NULL; count++, uptr = uptr->next) {
				if (!strcmp(keytext, uptr->keyname)) {
					/* yes, this unknown key is 
						being negotiated twice */
					TRACE_ERROR
					    ("unknown key \"%s\" received twice\n",
					     keytext);
					goto out;
				}
			}

			/* never saw this key before, should we save 
				it for later checks? */
			/* chap support = CHONG */
			if (!is_securitykey(keytext)) {
                TRACE_WARNING("unknown key \"%s\"\n",
                              keytext);
			}

			if (count >= MAX_UNKNOWN_KEYS) {
				/* no, we have seen too many unknown keys, 
						bail out */
				TRACE_ERROR
				    ("%u unknown keys received, too many!\n",
				     count);
				goto out;
			}

			else if ((uptr =
				  malloc(sizeof (struct unknown_key)))
				 == NULL) {
				goto out;
			}

			/* chap and srp support - CHONG */
			else if ((uptr->keyname =
				  malloc(strlen(keytext) + 1))
				 == NULL) {
				free(uptr);
				goto out;
			}

			else if ((uptr->keyvalue =
				  malloc(strlen(value) + 1))
				 == NULL) {
				free(uptr->keyname);
				free(uptr);
				goto out;
			} else {
				strcpy(uptr->keyname, keytext);
				strcpy(uptr->keyvalue, value);
				uptr->processed = 0;
				uptr->next = *unknown_key_list;
				*unknown_key_list = uptr;

				if ((flags & USE_REFLECT_XKEYS)
					&& (strncmp(keytext, "X-", 2) == 0
					    || strncmp(keytext, "X#", 2) == 0))
					/* this is an X key and user wants its
					 * value reflected back to sender
					 */
					*p_value = value;
				else
					/* pass back Notunderstood value for 
					 * unknown key
					 */
					*p_value = key_table->notunderstood;
			}
		}
		goto out;
	}

	if ((who_called == INITIATOR && !IS_USE_BY_TARGET(p->type))
	    || (who_called == TARGET && !IS_USE_BY_INITIATOR(p->type))) {
		TRACE_ERROR("key \"%s\" cannot be sent to %s\n", keytext,
			    who_called == INITIATOR ? "initiator" : "target");
		p->neg_info |= KEY_BAD;
		goto out;
	}

	if (IS_KEY_GOT_FROM_OTHER_SIDE(p->neg_info)) {
		/* attempting to negotiate this key twice */
		/*  Draft 20, Section 5.3 Login Phase
		 *  "Neither the initiator nor the target should attempt to 
		 *  declare or negotiate a parameter more than once 
		 *  during login except for responses to specific keys 
		 *  that explicitly allow repeated key declarations 
		 *  (e.e., TargetAddress).  An attempt to renegotiate/
		 *  redeclare parameters not specifically allowed MUST 
		 *  be detected by the initiator and target.  If such an 
		 *  attempt is detected by the target, the target MUST 
		 *  respond with Login reject (initiator error);
		 *  if detected by the initiator, the initiator MUST drop the
		 *  connection."
		 */
		TRACE_ERROR("key \"%s\" received twice\n", p->parameter_name);
		p->neg_info |= KEY_BAD;
		goto out;
	}

	if (!IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)) {
		/* this is new offer from other side, not reply 
			to one of our offers */
		if (!(p->type & when_called)) {
			/* wrong time to be negotiating this parameter */
			TRACE_ERROR("key \"%s\" cannot be negotiated now\n",
				    keytext);
			p->neg_info |= KEY_BAD;
			goto out;
		}
	}

	check_type_correctness(p, value, who_called, int_value);

	/* Return the value pointer on success */
	*p_value = value;

      out:

	TRACE(TRACE_ENTER_LEAVE, "Leave check_correctness, p %p", p);
	return p;
}

void __attribute__ ((no_instrument_function))
strreplace(char **str, const char *new_str)
{
	if (*str != NULL)
		free(*str);
	*str = (new_str == NULL ? NULL : strdup(new_str));
}

void __attribute__ ((no_instrument_function))
strreplace_upto(char **str, const char *new_str, int delim)
{
    const char *endptr;
	if (*str != NULL)
		free(*str);
    if (new_str == NULL)
        *str = NULL;
    else
    {
        endptr = strchr(new_str, delim);
        if (endptr == NULL)
            *str = strdup(new_str);
        else
        {
            *str = malloc(endptr - new_str);
            memcpy(*str, new_str, endptr - new_str - 1);
            (*str)[endptr - new_str] = '\0';
        }
    }
}


/* Copy src parameter table to dst, duplicating any strings */
void
param_tbl_cpy(struct parameter_type dst[MAX_CONFIG_PARAMS],
	      struct parameter_type src[MAX_CONFIG_PARAMS])
{
	struct parameter_type *dptr;
	int i;

	TRACE(TRACE_ENTER_LEAVE,
	      "Enter param_tbl_cpy, dst %p, src %p, size %d", dst, src,
	      sizeof (struct parameter_type) * MAX_CONFIG_PARAMS);

	/* copy everything in the src table to the dst table "as is" */
	memcpy(dst, src, sizeof (struct parameter_type) * MAX_CONFIG_PARAMS);

	/* now go back and "dup" all the strings */
	for (i = 0, dptr = dst; i < MAX_CONFIG_PARAMS; i++, dptr++) {
		if (dptr->parameter_name) {
			dptr->parameter_name = strdup(dptr->parameter_name);
		}
		if (dptr->str_value) {
			dptr->str_value = strdup(dptr->str_value);
		}
		if (dptr->value_list) {
			dptr->value_list = strdup(dptr->value_list);
		}
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave param_tbl_cpy");
}

/* Copy initial parameter table to dst, duplicating any strings */
void
param_tbl_init(struct parameter_type dst[MAX_CONFIG_PARAMS])
{
	param_tbl_cpy(dst, config_params);
}

/* Free any strings referenced in dst parameter table */
void
param_tbl_uncpy(struct parameter_type dst[MAX_CONFIG_PARAMS])
{
	struct parameter_type *dptr;
	int i;

	TRACE(TRACE_ENTER_LEAVE, "Enter param_tbl_uncpy, dst %p, size %d",
	      dst, sizeof (struct parameter_type) * MAX_CONFIG_PARAMS);

	/* go thru and free all the strings that were "duped" during copy */
	for (i = 0, dptr = dst; i < MAX_CONFIG_PARAMS; i++, dptr++) {
		if (dptr->parameter_name) {
			free(dptr->parameter_name);
		}
		if (dptr->str_value) {
			free(dptr->str_value);
		}
		if (dptr->value_list) {
			free(dptr->value_list);
		}
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave param_tbl_uncpy");
}

void
iscsi_configure_param_value(int param_neg_info,
                            const char *key,
                            const char *value,
                            struct parameter_type *p_param_tbl)
{
	struct parameter_type *param = NULL;
	char *endptr;
	int int_value;

	if ((param = find_parameter(key, p_param_tbl)) != NULL) 
    {
		if (value) 
        {
			if (IS_NUMBER(param->type)) {
				/* parameter is a number, set only the 
				 * int_value field, don't set value_list 
				 * or str_value fields 
				 */

				int_value =
				    strtoul(value, &endptr, 0);
				if (strspn(endptr, WHITE_SPACE) !=
				    strlen(endptr)) {
					TRACE_ERROR("illegal number \"%s\"\n",
						    value);
                    return;
				} else {
					param->int_value = int_value;
				}
			} else {
				/* parameter is a string, enumerated, boolean 
				 * or range,
				 * set both the value_list and str_value fields,
				 * do not set int_value field 
				 */

				if (IS_NUMBER_RANGE(param->type)) {
					/* this is a range, check both 
						numbers and their order */
					if (check_range(value, -1) < 0)
                        return;
				}

				strreplace(&param->value_list, value);
                strreplace_upto(&param->str_value, value, ',');
			}
		}

		/* set the negotiation info only if no error */
		param->neg_info = param_neg_info;
	}
}

void
configure_parameter(int param_neg_info,
		    char *ptr_to_keytext,
		    struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS])
{
	char *value_list = NULL;

	TRACE(TRACE_ENTER_LEAVE, "Enter configure_parameter");

	/* Get the value string */
	value_list = strchr(ptr_to_keytext, '=');
	if (value_list)
		*value_list++ = '\0';	/* terminate the key text string */
    iscsi_configure_param_value(param_neg_info,
                                ptr_to_keytext, value_list,
                                p_param_tbl);
}

/*	Draft 20, Section 5.2 Text Mode Negotiation
 *	"Some parameters may be subject to integrity rules (e.g., parameter-x
 *	must not exceed parameter-y or parameter-u not 1 implies parameter-v
 *	be Yes).  Whenever required, integrity rules are specified with the
 *	keys.  Checking for compliance with the integrity rule must only be
 *	performed after all the parameters are available (the existent and the
 *	newly negotiated).	An iSCSI target MUST perform integrity checking
 *	before the new parameters take effect.	An initiator MAY perform
 *	integrity checking."
 */
void
check_integrity_rules(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		      uint16_t secondary_connection)
{
	struct parameter_type *p, *p2;

	TRACE(TRACE_ENTER_LEAVE, "Enter check_integrity_rules");

	/*  Draft 20, Section A.3.2 OFMarkInt, IFMarkInt
	 *  "Reject is resetting the marker function in the specified
	 *  direction (Output or Input) to No."
	 */
	if ((p = find_flag_parameter(OFMARKINT_FLAG, p_param_tbl)) != NULL) {
		if (p->neg_info & KEY_REJECT) {
			/* got OFMarkInt=Reject, so set OFMarker=No */
			TRACE(TRACE_DEBUG, "Checking %s=%d",
			      p->parameter_name, p->int_value);
			if ((p = find_flag_parameter(OFMARKER_FLAG,
						 p_param_tbl)) != NULL) {
				TRACE(TRACE_DEBUG, "Have %s=%s",
				      p->parameter_name, p->str_value);
				if (!strcmp(p->str_value, key_table->yes)) {
					/* have OFMarker=Yes, change value 
						to No */
					strreplace(&p->str_value,
						   key_table->no);
					TRACE(TRACE_ISCSI, "Reset %s to %s",
					      p->parameter_name, p->str_value);
				}
			}
		}
	}
	if ((p = find_flag_parameter(IFMARKINT_FLAG, p_param_tbl)) != NULL) {
		if (p->neg_info & KEY_REJECT) {
			/* got IFMarkInt=Reject, so set IFMarker=No */
			TRACE(TRACE_DEBUG, "Checking %s=%d",
			      p->parameter_name, p->int_value);
			if ((p = find_flag_parameter(IFMARKER_FLAG,
						 p_param_tbl)) != NULL) {
				TRACE(TRACE_DEBUG, "Have %s=%s",
				      p->parameter_name, p->str_value);
				if (!strcmp(p->str_value, key_table->yes)) {
					/* have IFMarker=Yes, change 
						value to No */
					strreplace(&p->str_value,
						   key_table->no);
					TRACE(TRACE_ISCSI, "Reset %s to %s",
					      p->parameter_name, p->str_value);
				}
			}
		}
	}

	/*  the following checks are done only on first connection in 
	 * a session 
	 */
	if (!secondary_connection) {
		/*  Draft 20, Section 12.14 FirstBurstLength
		 *  "FirstBurstLength MUST NOT exceed MaxBurstLength."
		 */
		if ((p = find_flag_parameter(FIRSTBURSTLENGTH_FLAG,
					 p_param_tbl)) != NULL) {
			if ((p2 = find_flag_parameter(MAXBURSTLENGTH_FLAG,
						 p_param_tbl)) != NULL) {
				if (p->int_value > p2->int_value) {
                    TRACE_WARNING("FirstBurstLength %u"
                                  " exceeds MaxBurstLength %u\n",
                                  p->int_value, p2->int_value);
					/* fix FirstBurstLength */
					p->int_value = p2->int_value;	
				}
			}
		}

		/*  Draft 20, Section 12.21 SessionType
		 *  "The discovery session implies MaxConnections = 1 
		 *  and overrides both
		 *  the default and an explicit setting."
		 */
		if ((p = find_flag_parameter(SESSIONTYPE_FLAG,
					 p_param_tbl)) != NULL) {
			if (!strcmp(p->str_value, key_table->discovery)
			    && (p2 = find_flag_parameter(MAXCONNECTIONS_FLAG,
						    p_param_tbl)) != NULL) {
				if (p2->int_value != 1) {
					/* do not have MaxConnections=1, 
						change value to 1 */
					p2->int_value = 1;
					TRACE(TRACE_ISCSI, "Reset %s to %u",
					      p2->parameter_name,
					      p2->int_value);
				}
			}
		}

		/*  Draft 20, Section 12.19 DataSequenceInOrder
		 *  "If DataSequenceInOrder is set to Yes, a target may 
		 *  retry at most the last R2T, and an initiator may 
		 *  at most request retransmission for the last read 
		 *  data sequence.  For this reason, if
		 *  ErrorRecoveryLevel is not 0 and DataSequenceInOrder is 
		 *  set to Yes then MaxOutstandingR2T MUST be set to 1."
		 */
		if ((p = find_flag_parameter(DATASEQUENCEINORDER_FLAG,
					 p_param_tbl)) != NULL) {
			if (!strcmp(p->str_value, key_table->yes)
			    && (p2 =
				find_flag_parameter(ERRORRECOVERYLEVEL_FLAG,
						    p_param_tbl))
			    != NULL) {	/* have DataSequenceInOrder=Yes */
				if (p2->int_value > 0
				    && (p = find_flag_parameter
					(MAXOUTSTANDINGR2T_FLAG, p_param_tbl))
				    != NULL) {
					/* have ErrorRecoveryLevel>0 */
					if (p->int_value != 1) {
						/* do not have 
						 * MaxOutstandingR2T = 1, 
						 * change it to 1 
						 */
                        TRACE_WARNING
							("%s=%u reset to 1\n",
                             p->parameter_name,
                             p->int_value);
						p->int_value = 1;
					}
				}
			}
		}
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave check_integrity_rules");
}

/*
 * This function receives 'length' bytes into buffer from the socket 
 */
int
iscsi_recv_msg(int sock, int length, char *buffer, int flags)
{
    int retval;

    /* the following code will receive PDU's from the other side */

    UNUSED(flags);

    if (length == 0) return 0;

    TRACE(TRACE_VERBOSE, "Attempting to read %d bytes", length);
    retval = recv(sock, buffer, length, MSG_WAITALL);
    pthread_testcancel();

    if (retval <= 0) {
        if (retval == 0) {
            TRACE_ERROR("remote peer disconnected\n");
        } else {
            TRACE_ERROR("recvmsg error %d\n", retval);
        }
        retval = -1;
        goto out;
    }
    TRACE(TRACE_DEBUG, "Received: %d", retval);

out:
    TRACE(TRACE_ENTER_LEAVE, "Leave iscsi_recv_msg, retval %d", retval);

    return retval;
}

/*
 * returns 1 if needs a break from the loop 
 * else returns a 0 
 */
int __attribute__ ((no_instrument_function))
check_out_length(int out_length, int resp_len)
{

	TRACE(TRACE_ENTER_LEAVE,
	      "Enter check_out_length cur_len: %d, add_len: %d", out_length,
	      resp_len);

	/* Increment the out_length */
	out_length += resp_len + 2;

	TRACE(TRACE_ENTER_LEAVE, "Leave check_out_length");

	if (out_length > MAX_TEXT_LEN)
		return 1;
	else
		return 0;
}

/*
 * sends the PDUs to the other end. 
 */

int
iscsi_send_msg(int sock, struct generic_pdu *outputpdu, int flags)
{
    int tx_loop;
    int data_length;
    uint8_t *buffer;
    size_t   length = ISCSI_HDR_LEN;

    struct iscsi_targ_login_rsp *targ_login_rsp;

    UNUSED(flags);

    TRACE(TRACE_ENTER_LEAVE, "Enter iscsi_send_msg");

    switch (outputpdu->opcode & (ISCSI_OPCODE)) 
    {
        case ISCSI_INIT_LOGIN_CMND:
            /* Send the login Command */
            print_init_login_cmnd((struct iscsi_init_login_cmnd *)
                                  outputpdu);
            break;
        case ISCSI_TARG_LOGIN_RSP:
            /* Send the Login Response */
            targ_login_rsp = (struct iscsi_targ_login_rsp *) outputpdu;
            
            TRACE(TRACE_ISCSI,
                  "Send Login Response, CSG %d, NSG %d, T %d",
                  (outputpdu->flags & CSG) >> CSG_SHIFT,
                  outputpdu->flags & NSG, (outputpdu->flags & T_BIT) >> 7);

            print_targ_login_rsp(targ_login_rsp);
            break;
        default:
            TRACE_ERROR("sending bad opcode 0x%02X during Login phase\n",
                        outputpdu->opcode & ISCSI_OPCODE);
            return -1;
    }			/* switch */
        
    /********************************************************/
    
    /* all pdus store the DSL in the same place */
    outputpdu->length = htonl(outputpdu->text_length);

    if ((data_length = outputpdu->text_length) > 0) {
        /* there is data attached to this pdu */
        
        /* Add padding to end of any attached data */
        data_length += (-data_length) & 3;
        
        length += data_length;
    }
    
    buffer = calloc(sizeof(*buffer), length);
    
    memcpy(buffer, outputpdu, ISCSI_HDR_LEN);
    if (length > ISCSI_HDR_LEN) 
    {
        memcpy(buffer + ISCSI_HDR_LEN, outputpdu->text,
               outputpdu->text_length);
    } 

    print_payload(buffer, length);
    tx_loop = send(sock, buffer, length, 0); 
    TRACE(TRACE_NET, "sent %d bytes", tx_loop);
    
    free(buffer);

    TRACE(TRACE_ENTER_LEAVE, "Leave iscsi_send_msg");

    return 0;
}

/*
 * checks whether the receiving side has received all the responses 
 * for the negotiations it started in the last packet. 
 * Return: 
 *     -1 if error, didn't receive the response 
 *	0 if success 
 */
int
check_neg_responses(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		    uint32_t print_error)
{
	int i = 0;
	int retval = 0;
	struct parameter_type *p = NULL;

	TRACE(TRACE_ENTER_LEAVE, "Enter check_neg_responses");

	for (i = 0; i < MAX_CONFIG_PARAMS; i++) {
		p = &p_param_tbl[i];

		if (IS_KEY_TO_BE_NEGOTIATED(p->neg_info)
		    && IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)
		    && !IS_KEY_GOT_FROM_OTHER_SIDE(p->neg_info)
		    && !IS_KEY_REPLY_OPTIONAL(p->neg_info)) {
			/* parameter was sent, no reply was received 
			 * and reply not optional 
			 */
			if (!IS_INFORMATIONAL_PARAM(p->type)
			    && !(p->
				 special_key_flag &
				 MAXRECVDATASEGMENTLENGTH_FLAG)) {
				/* informational parameters expect no 
				 * response and neither do 
				 * MaxRecvPDULength/MaxRecvDataSegmentLength 
				 */

				TRACE(TRACE_DEBUG,
				      "response not yet received for parameter %s",
				      p->parameter_name);
				if (print_error) {
					TRACE_ERROR
					    ("response expected for parameter %s\n",
					     p->parameter_name);
					/* set got bit so we won't get this message again */
					p->neg_info |= KEY_GOT_FROM_OTHER_SIDE;
				}
				retval = -1;
			}
		}
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave check_neg_responses, retval = %d",
	      retval);

	return retval;
}

/*
 * checks whether the receiving side supports the parameter values
 * offered by the sender.
 * (because in this case this function is not called)	
 * Return:
 *	If supported returns the pointer to the (terminated) value in
 *	the supplied_values_from_sender list, else returns NULL			*/
static char * __attribute__ ((no_instrument_function))
check_for_support(struct parameter_type *p, char *supplied_values_from_sender)
{
	char *dummy1 = NULL;
	char *dummy2 = NULL;
	char *sender_value = NULL;
	char *receiver_value = p->value_list;

	TRACE(TRACE_ENTER_LEAVE, "Enter check_for_support, sender_value %s",
	      supplied_values_from_sender);

	/* No support */
	if (receiver_value == NULL)
		goto out;

	TRACE(TRACE_DEBUG, "%s's value_list: %s", p->parameter_name,
	      receiver_value);

	/* Check whether the parameter's str_value agrees with *
	 * one of the supplied values */

	do {
		/* Get the receiver_value from the parameter's value_list */
		dummy1 = strchr(receiver_value, ',');
		if (dummy1) {
			*dummy1 = '\0';
		}

		TRACE(TRACE_DEBUG, "receiver_value: %s", receiver_value);

		/* Initialise */
		sender_value = supplied_values_from_sender;

		do {
			/* extract just a single value */
			dummy2 = strchr(sender_value, ',');
			if (dummy2) {
				*dummy2 = '\0';
			}

			TRACE(TRACE_DEBUG, "sender_value: %s", sender_value);

			if (!strcmp(receiver_value, sender_value)) {
				/* Found a match, return pointer to it 
					in sender_value */
				if (dummy1)
					*dummy1 = ',';	/* restore the 
							 * comma before 
							 * leaving 
							 */
				goto out;
			}

			/* goto the next value supplied by sender */
			if (dummy2) {
				*dummy2++ = ',';	/* restore comma */
				TRACE(TRACE_DEBUG, "dummy2++sender_value: %s",
				      sender_value);
			} else {
				TRACE(TRACE_DEBUG,
				      "dummy2 null sender_value: %s",
				      sender_value);
			}
			sender_value = dummy2;
		}
		while (sender_value);

		/* restore the comma and goto the next value in 
			receiver's table list */
		if (dummy1) {
			*dummy1++ = ',';
		}

		receiver_value = dummy1;
	}
	while (receiver_value);

      out:

	if (sender_value) {
		TRACE(TRACE_ENTER_LEAVE,
		      "Leave check_for_support, return value %s",
		      sender_value);
	} else {
		TRACE(TRACE_ENTER_LEAVE,
		      "Leave check_for_support, return value NULL");
	}

	return sender_value;
}

static void __attribute__ ((no_instrument_function))
update_key_value(struct parameter_type *p, int int_value, char *value)
{
	if (IS_NUMBER(p->type) || IS_NUMBER_RANGE(p->type)) {
		if ((int)p->int_value != int_value) {
			/* have a new numeric value for this key */
			p->int_value = int_value;
			TRACE(TRACE_ISCSI, "Update key %s, new value %d",
			      p->parameter_name, p->int_value);
		}
	} else if (p->str_value && !strcmp(value, p->str_value)) {
		/* already have this string value in place */
		} else {
			/* have a new string value for this key */
			strreplace(&p->str_value, value);
			TRACE(TRACE_ISCSI, "Update key %s, new value %s",
			      p->parameter_name, p->str_value);
	}
}

static void __attribute__ ((no_instrument_function))
handle_boolean_param(struct parameter_type *p, char **value);

static int __attribute__ ((no_instrument_function))
handle_params_noexch(struct parameter_type *p,
			struct parameter_type p_param_tbl [MAX_CONFIG_PARAMS],
			uint32_t flags,
			int int_value,
			char *value,
			char *string,
			int *resp_len,
			int *FBLength ,
			struct parameter_type **FBp,
			int *MBLength,
			struct parameter_type **MBp);

static int __attribute__ ((no_instrument_function))
handle_params_resp(struct parameter_type *p,
                	char *value,
                	int int_value,
                	int *out_length);

/*
 * scan through the input and process keys
 * p_param_tbl - pointer to the table of parameters
 * sock - pointer to the socket structure
 * process_these_types - types of parameters to be processed.
 * flags_to_be_set - Which flags to set
 * role - who is calling (INITIATOR/TARGET)
 * Return: returns the length of the string added to the output string
 */
int
scan_input_and_process(int sock,
			struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
			int process_these_types,
			int flags_to_be_set,
			int role,
			int *max_send_length,
			uint32_t when_called,
			struct generic_pdu *inputpdu,
			struct generic_pdu *outputpdu,
			uint32_t flags,
			uint64_t * login_flags,
			struct unknown_key **unknown_key_list)
{
	struct parameter_type *p = NULL;
	int resp_len, n;
	char *dummy_string;
	char *input_str, *this_key;
	char *input_string;
	int out_length = 0;
	char *output_string;
	int key_value_len;
	char *value;
	int int_value;
	char *last_input;
	int FBLength = -1, MBLength = -1;
	struct parameter_type *FBp = NULL, *MBp = NULL;

        UNUSED(sock);

	TRACE(TRACE_ENTER_LEAVE, "Enter scan_input_and_process");

	ALLOCATE_MAX_TEXT_LEN(dummy_string);
	ALLOCATE_MAX_TEXT_LEN(input_str);

	output_string = outputpdu->text + outputpdu->text_length;

	/* Copy the input text to a local buffer */
	memcpy(input_str, inputpdu->text, inputpdu->text_length);
	input_string = input_str;	/* point at first char */
	last_input = input_str + inputpdu->text_length;	/* point beyond 
								last char */
	*last_input = '\0';	/* ensure '\0' at end of last */

	while (input_string < last_input) {
		key_value_len = strlen(input_string);
		TRACE(TRACE_DEBUG, "key_value_len = %d", key_value_len);
		TRACE(TRACE_DEBUG, "FBLength %x MBLength %x",
				FBLength,MBLength);

		/* assume no reply will be sent */
		resp_len = 0;

		value = NULL;
		int_value = -1;
		if ((p = check_correctness(input_string, &value, p_param_tbl,
					   role, when_called, flags, &int_value,
					   unknown_key_list)) == NULL) {

			/* We don't understand the request. */
			if (value == NULL) {	/* Error is fatal */
				out_length = -1;
				goto out;
			}
			if (!((process_these_types & SECURITY_PARAM))
			    || (!is_securitykey(input_string))) {
				resp_len = sprintf(dummy_string,
						   "%s=%s", input_string,
						   value);
			}
		} else if (IS_KEY_BAD(p->neg_info)) {
			out_length = -1;
			goto out;
		} else if (!IS_KEY_IRRELEVANT(p->neg_info)) {
			/* p is the pointer to the table entry for the 
			 * received parameter and value points to the 
			 * string following the "=" in the input 
			 */
			TRACE(TRACE_DEBUG,
			      "Process input param: %s, value: %s",
			      p->parameter_name, value);

			/* check if this is a Full Feature Only key */
			if (IS_FULL_FEATURE_ONLY(p->type)) {	
				/* protocol error */
				TRACE_ERROR
				    ("%p only valid in full feature phase\n",
				     p->parameter_name);
				out_length = -1;
				goto out;
			}

			if (p->special_key_flag & flags_to_be_set) {
				/* caller wants to know that this key 
				 * was processed
				 */
				*login_flags |= p->special_key_flag;

				if (p->special_key_flag & SESSIONTYPE_FLAG) {
					/* set the DISCOVERY_FLAG when 
					 * get SessionType=Discovery
					 */
					if (!strcmp
					    (value, key_table->discovery)) {
						*login_flags |= DISCOVERY_FLAG;
					}
				}
			}

			if ((p->type & process_these_types)) {
				/* handle key enquiry by replying with 
					our current value */
				if (!strcmp(value, "?")) {
					/* inquiry no longer accepted in 
						draft 11 and after */
					TRACE_ERROR
					    ("inquiry no longer accepted "
					     "param: %s, value: %s\n",
					     p->parameter_name, value);
					resp_len =
					    sprintf(dummy_string, "%s=%s",
						    p->parameter_name,
						    key_table->reject);
					/* do not consider this param as 
						"gotten from other side" */
					p = NULL;
				} else if (p->str_value
					   && !strcmp(p->str_value, "?")) {
					/* we sent an inquiry and this is 
					 * the response 
					 * Note this should never happen 
					 * for draft 11 and after 
					 */
					printf
					    ("iscsi response to inquiry: %s=%s\n",
					     p->parameter_name, value);
				} else {
					/* neither an inquiry or a reply to 
					 * an inquiry .
					 * First convert boolean to yes or 
					 * no result 
					 */
					if (IS_BOOLEAN(p->type)) {
						handle_boolean_param(p, &value);
					}

					if (p->special_key_flag &
					    MAXRECVDATASEGMENTLENGTH_FLAG) {
						/* this is 
						 * MaxRecvDataSegmentLength 
						 * that other side wants to 
						 * receive, i.e., the max 
						 * size PDU we should send. 
						 * No reply is necessary for 
						 * this key.
						 */
						if (int_value < 512
						    || int_value > 16777215) {
                            TRACE_WARNING
							    ("%s %d out of "
                                 "bounds "
                                 "[512..16777215]\n",
                                 p->parameter_name,
                                 int_value);
						} else {
							/* value is within 
							 * bounds, pass it back
							 * to caller after 
							 * forcing it to be a 
							 * multiple of 512 
							 */
							int_value /= 512;
							*max_send_length =
							    512 * int_value;
						}
					} else
					    if (!IS_KEY_SENT_TO_OTHER_SIDE
						(p->neg_info)) {

						handle_params_noexch(p,
								p_param_tbl,
								flags,
								int_value,
								value,
								dummy_string,
								&resp_len,
								&FBLength ,
								&FBp,
								&MBLength,
								&MBp);

					} else {
						if (handle_params_resp(p,
							value,
							int_value,
							&out_length) < 0 ){
							TRACE_ERROR
					    	("handle_params_resp !\n");
								goto out;
						}
					}
				}
			} else {
				if (IS_SECURITY_PARAM(p->type)) {
					TRACE_ERROR
					    ("not in security phase when "
					      " received input "
					     "param: %s, value: %s\n",
					     p->parameter_name, value);
					out_length = -1;
					goto out;
				} else if (IS_OPERATIONAL_PARAM(p->type)) {
					TRACE_ERROR
					    ("not in operational phase when "
					    	" received input "
					     "param: %s, value: %s\n",
					     p->parameter_name, value);
					out_length = -1;
					goto out;
				} else if (IS_INFORMATIONAL_PARAM(p->type)) {
					TRACE_ERROR
					    ("not processing informational "
					      " parameters when "
					     "recv input param: %s,value: %s\n",
					     p->parameter_name, value);
					out_length = -1;
					goto out;
				} else {
					TRACE_ERROR
					    ("recv unclassied input param: %s, "
					     "value: %s\n", p->parameter_name,
					     value);
					out_length = -1;
					goto out;
				}
			}
		}

		/* Always check if the response length exceeds MAX_TEXT_LEN */
		/* Then the remaining parameters have to be sent in a     */
		/* separate text command/response */

		TRACE(TRACE_DEBUG, "resp_len = %d", resp_len);

		if (resp_len) {
			if (check_out_length(out_length, resp_len)) {
				*login_flags |= MORE_TO_SEND_FLAG;
				break;
			} else {
				/* Update resp_len and the pointer 
					to output_string */
				sprintf(output_string, "%s", dummy_string);
				output_string += resp_len + 1;

				TRACE(TRACE_ISCSI, "Attach key: %s",
				      dummy_string);

				/* update out_length */
				out_length += resp_len + 1;

				if (p) {	/* set sent-to-other-side bit */
					p->neg_info |= KEY_SENT_TO_OTHER_SIDE;
				}

				resp_len = 0;
			}
		}

		if (p) {	/* set got-from-other-side bit */
			p->neg_info |= KEY_GOT_FROM_OTHER_SIDE;
		}

		/* Goto the next key=value pair */
		this_key = input_string;	/* remember where this 
			key started */
		/* this should be start of next key */
		input_string += key_value_len + 1;

		/*  In Drafts 9 and before, Section 3.10.4 Text
		 *  "Every key=value pair (including the last or only pair) MUST
		 *  be followed by at least one null (0x00) delimiter."
		 * 
		 *  This implied there could be more than one null (0x00) at the
		 *  end of a string, so skip over these extra nulls now
		 */
		while (*input_string == '\0' && input_string < last_input) {
			input_string++;
		}

		/*  In final Draft 20, Section 5.1 Text Format
		 *  "Every key=value pair, including the last or only 
		 *   pair in a LTDS,  MUST be followed by one null (0x00) 
		 *  delimiter."
		 * 
		 *  This implies that there can NOT be more than one 
		 *  null (0x00) at the
		 *  end of a string, so if there were extra nulls, 
		 *  give a warning now.
		 */
		n = (input_string - this_key) - (key_value_len + 1);
		if (n > 0) {
            TRACE_WARNING("%d extra nulls (0x00) found after"
                          " key \"%s\"\n", n, this_key);
		}
	}

	/* process FirstBurstLength and MaxBurstLength if their 
		reply was delayed */
	TRACE(TRACE_DEBUG, "FBLength %x MBLength %x",FBLength,MBLength);

	if (FBLength > 0 && MBLength > 0) {
		/* both keys were offered by other side in this pdu */
		if (FBLength <= MBLength) {
			/* ok */
			/* MaxBurstLength is bigger than 
				FirstBurstLength, all ok */
		} else {
			/* FirstBurstLength bigger than MaxBurstLength, 
				fix it */

			FBp->int_value = FBLength = MBLength;
			TRACE (TRACE_DEBUG, "Updated FBp %p\n",FBp);
		}
	} else if (FBLength > 0) {
		/* only FirstBurstLength was received in this pdu */
		if ((MBp =
		     find_flag_parameter(MAXBURSTLENGTH_FLAG,
					 p_param_tbl)) != NULL) {
			if (FBLength > (int)MBp->int_value) {
				/* The received FirstBurstLength is 
					bigger than our current
				   MaxBurstLength */
				if (IS_KEY_SENT_TO_OTHER_SIDE(MBp->neg_info)) {
					/* MaxBurstLength already sent, 
						can't be fixed */
					TRACE_ERROR
					    ("FirstBurstLength %d bigger "
					    " than negotiated "
					     "MaxBurstLength %d", FBLength,
					     MBp->int_value);
					out_length = -1;
					goto out;
				}
				/* The unsent MaxBurstLength is smaller 
				 * than the received FirstBurstLength, 
				 * reduce the FirstBurstLength to the
				 * MaxBurstLength and send both 
				 */
				FBLength = FBp->int_value = MBLength =
				    MBp->int_value;
				TRACE (TRACE_DEBUG, "Updated FBLength %u\n",
					FBLength);
			} else {
				/* The received FirstBurstLength not 
				 * bigger than our current
				 * MaxBurstLength, send MaxBurstLength only if 
				 * negotiation needed. 
				 */
				if ((IS_KEY_TO_BE_NEGOTIATED(MBp->neg_info)
				     || IS_KEY_GOT_FROM_OTHER_SIDE(MBp->
								   neg_info))
				    && !IS_KEY_SENT_TO_OTHER_SIDE(MBp->
								  neg_info)) {
					MBLength = MBp->int_value;
				}
			}
		}
	} else if (MBLength > 0) {
		/* only MaxBurstLength was received in this pdu */
		if ((FBp = find_flag_parameter(FIRSTBURSTLENGTH_FLAG,
					       p_param_tbl)) != NULL) {
			/* must have MaxBurstLength > FirstBurstLength */
			if (MBLength < (int)FBp->int_value) {
				/* MaxBurstLength is smaller than 
					FirstBurstLength */
				if (IS_KEY_SENT_TO_OTHER_SIDE(FBp->neg_info)) {
					/* FirstBurstLength already sent, 
						can't be fixed */
					TRACE_ERROR
					    ("MaxBurstLength %d smaller than "
					      " negotiated "
					     "FirstBurstLength %d", MBLength,
					     FBp->int_value);
					out_length = -1;
					goto out;
				}
				/* FirstBurstLength has not been sent yet,
				 *  so force it to be equal to 
				 * MaxBurstLength and offer it now 
				 */
				FBLength = FBp->int_value = MBLength;
				TRACE (TRACE_DEBUG, "Updated FBLength %u\n",
						FBLength);
			} else {
				/* MaxBurstLength not smaller than 
				 * FirstBurstLength, send
				 * FirstBurstLength only if it still needs 
				 * to be negotiated 
				 */
				if ((IS_KEY_TO_BE_NEGOTIATED(FBp->neg_info)
				     || IS_KEY_GOT_FROM_OTHER_SIDE(FBp->
								   neg_info))
				    && !IS_KEY_SENT_TO_OTHER_SIDE(FBp->
								  neg_info)) {
					FBLength = FBp->int_value;
				}
			}
		}
	}

	if (FBLength > 0) {
		/* reply to or offer the FirstBurstLength */
		TRACE(TRACE_DEBUG, "FBLength %x FBp %p",FBLength,FBp);
		FBp->neg_info |= KEY_SENT_TO_OTHER_SIDE;
		if (!IS_KEY_GOT_FROM_OTHER_SIDE(FBp->neg_info))
			outputpdu->flags &= (~T_BIT);
		resp_len =
		    sprintf(dummy_string, "%s=%d", FIRSTBURSTLENGTH, FBLength);
		sprintf(output_string, "%s", dummy_string);
		output_string += resp_len + 1;

		TRACE(TRACE_ISCSI, "Attach key, %s", dummy_string);

		/* update out_length */
		out_length += resp_len + 1;
		resp_len = 0;
		FBLength = -1;
	}

	if (MBLength > 0) {
		/* reply to or offer the MaxBurstLength */
		TRACE(TRACE_DEBUG, "MBLength %x MBp %p",MBLength,MBp);
		MBp->neg_info |= KEY_SENT_TO_OTHER_SIDE;
		if (!IS_KEY_GOT_FROM_OTHER_SIDE(MBp->neg_info))
			outputpdu->flags &= (~T_BIT);
		resp_len =
		    sprintf(dummy_string, "%s=%d", MAXBURSTLENGTH, MBLength);
		sprintf(output_string, "%s", dummy_string);
		output_string += resp_len + 1;

		TRACE(TRACE_ISCSI, "Attach key, %s", dummy_string);

		/* update out_length */
		out_length += resp_len + 1;
		MBLength = -1;
	}

      out:
	FREE_STRING(dummy_string);
	FREE_STRING(input_str);

	TRACE(TRACE_ENTER_LEAVE,
	      "Leave scan_input_and_process, out_length %d", out_length);

	/* return the length added to the output_string */
	return out_length;
}

/*
 * Populates "value" with the approprite return value
 * that we expect back from the other end.
 */

static void __attribute__ ((no_instrument_function))
handle_boolean_param(struct parameter_type *p, char **value)
{
	int receiver_value;
	int sender_value;

	TRACE(TRACE_ENTER_LEAVE, "Enter handle_boolean_param %s=%s",
		p->parameter_name, *value);

	if (!strcmp(*value, key_table->yes))
		sender_value = 1;
	else
		sender_value = 0;

	if (!strcmp(p->str_value, key_table->yes))
		receiver_value = 1;
	else
		receiver_value = 0;

	if (IS_BOOL_AND(p->type)) {
		/* Boolean AND function */
		if (sender_value & receiver_value) {
			/* both sides said YES so result is YES */
			*value = key_table->yes;
		} else {
			/* either (or both) of the sides said NO */
			*value = key_table->no;
			if (sender_value == 0) {
				/* sender sent NO, so reply is optional
				 *
				 *  Draft 20, Section 5.2.2 Simple-value
				 *  Negotiations
				 *  "Specifically, the two cases in 
				 *  which answers are OPTIONAL are:
				 *
				 *  - The boolean function is "AND" and
				 *    the value "No" is received. The
				 *    outcome of the negotiation is "No".
				 *
				 *  - The boolean function is "OR" and 
				 *     the  value "Yes" is received.
				 *     The outcome of the negotiation 
				 *     is "Yes".
				 *
				 *  Responses are REQUIRED in all other
				 *  cases, and the value chosen and 
				 *  sent by the acceptor becomes the 
				 *  outcome of the  negotiation."
				 */
				p->neg_info |= KEY_REPLY_OPTIONAL;
			} else if (IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)) {
				/* sender sent YES to our offer of NO -- BAD */
				TRACE_ERROR("received 'Yes' to our offer of"
					" 'No' for AND function key \"%s\"\n",
					p->parameter_name);
				p->neg_info |= KEY_BAD;
			}
		}
	} else {
		/* Boolean OR function */
		if (sender_value | receiver_value) {
			/* either (or both) of the sides said YES */
			*value = key_table->yes;
			if (sender_value != 0) {
				/* sender sent YES, so reply is optional
				 *
				 * Draft 20, Section 5.2.2 Simple-value
				 * Negotiations
				 *  "Specifically, the two cases in 
				 *  which answers are OPTIONAL are:
				 *
				 *  - The boolean function is "AND" 
				 *    and the value "No" is received.
				 *    the outcome of the negotiation 
				 *    is "No".
				 *
				 *  - The boolean function is "OR" 
				 *    and the value "Yes" is received.
				 *    The outcome of the negotiation is
				 *    "Yes".
				 *
				 *  Responses are REQUIRED in all other
				 *  cases, and the value chosen and 
				 *  sent by the acceptor becomes the 
				 *  outcome of the
				 *  negotiation."
				 */
				p->neg_info |= KEY_REPLY_OPTIONAL;
			} else if (IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)) {
				/* sender sent NO to our offer of YES -- BAD */
				TRACE_ERROR("received 'No' to our offer of "
					"'Yes' for OR funct key \"%s\"\n",
					p->parameter_name);
				p->neg_info |= KEY_BAD;
			}
		} else {
			/* both of the sides said NO so result is NO */
			*value = key_table->no;
		}
	}
	TRACE(TRACE_ENTER_LEAVE, "Leave handle_boolean_param");
}

/*
 * populates "string" with paramters that do not require
 * a response from the other end. The resultant length is
 * passed back in resp_len.
 * Also ensures that the MaxBurst lengths are not exceeded.
 */
static int __attribute__ ((no_instrument_function))
handle_params_noexch(struct parameter_type *p,
		struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		uint32_t flags,
		int int_value,
		char *value,
		char *string,
		int *resp_len,
		int *FBLength ,
		struct parameter_type **FBp,
		int *MBLength,
		struct parameter_type **MBp )
{
	struct parameter_type *p1 = NULL;

	TRACE(TRACE_ENTER_LEAVE, "Enter handle_params_noexch");

	if (!IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info)) {

		/* parameter not sent previously by us, so this 
		 * is a new offer, formulate our reply 
		 */
		if (IS_INFORMATIONAL_PARAM(p->type)) {
			/* offered parameter is informational, send no 
			   reply */
			update_key_value(p, int_value, value);
		} else if (IS_NUMBER_RANGE(p->type)) {
			/* offered parameter's value should be a 
			 * range <key=value1~value2> 
			 */
			int send_irrelevant = 0;

			if (p->special_key_flag & OFMARKINT_FLAG) {
				/* reply with Irrelevant if OFMARKER is no */
				if ((p1 =
				     find_flag_parameter (OFMARKER_FLAG, 
						p_param_tbl)) != NULL) {
					if (!strcmp
					    (p1->str_value, key_table->no)) {
						send_irrelevant = 1;
					}
				}
			} else if (p->special_key_flag & IFMARKINT_FLAG) {
				/* reply with Irrelevant if IFMARKER is no */
				if ((p1 = find_flag_parameter
				     		(IFMARKER_FLAG, p_param_tbl))
						    != NULL) {
					if (!strcmp
					    (p1->str_value, key_table->no)) {
						send_irrelevant = 1;
					}
				}
			}

			if (send_irrelevant) {
				*resp_len = sprintf (string, "%s=%s",
				     		p->parameter_name, 
						key_table->irrelevant);
			} else if (int_value >= 0) {
				/* keep the offered lower end of range */
				update_key_value(p, int_value, value);
				/* always need a reply to range parameter */
				*resp_len = sprintf (string, "%s=%d", 
						p->parameter_name, 
						p->int_value);
			}
		} else if (!IS_NUMBER(p->type)) {
			/* offered parameter is not informational, 
			 * range or number, check that we support 
			 * the offered val 
			 */
			char *param_value = NULL;

			if (IS_BOOLEAN(p->type)) {
				/* offered param is boolean, support assured */
				param_value = value;
			} else {
				param_value = check_for_support(p, value);
			}

			if (param_value != NULL) {
				/* value received is supported by us, use it */
				if (!IS_KEY_REPLY_OPTIONAL(p->neg_info)
				    || (flags & USE_FULL_REPLIES)) {
					/* reply not optional or full reply 
					 * required 
					 */
					*resp_len = sprintf (string, "%s=%s",
					     		p->parameter_name, 
							param_value);
				} else {
					/* optional reply not 
					 * sent but marked as
					 * sent 
					 */
					p->neg_info |= KEY_SENT_TO_OTHER_SIDE;
				}
				update_key_value(p, int_value, param_value);
			} else {
				/* value not supported by us, send reject */
				*resp_len = sprintf (string, "%s=%s",
				     			p->parameter_name, 
							key_table->reject);
			}
		} else {
			/* offered parameter is a number, apply its 
			 * selection function 
			 */
			if ((IS_MIN_NUMBER(p->type)
			     && (int)p->int_value <= int_value)
			    || (IS_MAX_NUMBER(p->type)
				&& (int)p->int_value >= int_value)) {
				/* correct min or max already in place */
			} else {
				/* the offered value is the one we keep */
				update_key_value(p, int_value, value);
			}

			/*  Delay reply to FirstBurstLength & MaxBurstLength
			 *  until end of the PDU to ensure
			 *  FirstBurstLength <= MaxBurstLength
			 */
			if (p->special_key_flag & FIRSTBURSTLENGTH_FLAG) {
				*FBLength = p->int_value;
				*FBp = p;
				TRACE (TRACE_DEBUG, "Updated FBp %p\n",FBp);
				/* reply to this key at end of this pdu */
			} else if (p->special_key_flag & MAXBURSTLENGTH_FLAG) {
				*MBLength = p->int_value;
				*MBp = p;
				TRACE (TRACE_DEBUG, "Updated MBp %p\n",MBp);
				/* reply to this key at end of this pdu */
			} else {	
				/* always need a reply to numeric parameter */
				*resp_len = sprintf (string, "%s=%d", 
						p->parameter_name, 
						p->int_value);
			}
		}
		TRACE(TRACE_ENTER_LEAVE, "Leave handle_params_noexch");
		return 1;
	}
	return -1;
}

/* 
 * Parameter already sent, so this is the response 
 */
static int __attribute__ ((no_instrument_function))
handle_params_resp(struct parameter_type *p,
                char *value,
                int int_value,
                int *out_length)
{

	TRACE(TRACE_ENTER_LEAVE, "Enter handle_params_resp for %s",
			p->parameter_name);
	if (!IS_NUMBER(p->type)) {
		/* received parameter is not a number */
		char *param_value = NULL;

		/* response value should just be a single string */
		if (strchr(value, ',')) {
			TRACE_ERROR
			    ("multiple values in negotiation "
			     "response for parameter: %s\n", p->parameter_name);
			*out_length = -1;
			TRACE(TRACE_ENTER_LEAVE, 
				"Leave handle_params_resp with error");
			return -1;
		}

		if (IS_BOOLEAN(p->type)) {
			/* offered parameter is boolean, 
			   support assured */
			param_value = value;
		} else if (!IS_NUMBER_RANGE(p->type)) {
			param_value = check_for_support(p, value);
		}
		if (param_value != NULL) {
			/* received value is supported by 
			 * receiver, use it 
			 */
			update_key_value(p, int_value, value);
		} else if (IS_NUMBER_RANGE(p->type)) {
			/* reply must be in range <key=value1~value2> */
			if (IS_KEY_REJECT(p->neg_info)) {
				/*got Reject reply to OFMarkInt/IFMarkInt */
			} else {
				/* reply must be numeric 
				 * within offered range 
				 */
				if (check_range(p->value_list, int_value) < 0) {
					TRACE_ERROR("check_range failed\n");
					*out_length = -1;
					TRACE(TRACE_ENTER_LEAVE, 
				      "Leave handle_params_resp with error");
					return -1;
				}
				if ((int)p->int_value != int_value) {
					/* assign the negotiated value */
					update_key_value(p, int_value, value);
				}
			}
		} else {
			/* received value not among those we offered */
			TRACE_ERROR
			    ("bad response for key "
			     "%s, response %s\n", p->parameter_name, value);
			*out_length = -1;
			TRACE(TRACE_ENTER_LEAVE, 
				"Leave handle_params_resp with error");
			return -1;	
		}
	} else {
		/* received reply parameter is a number */

		/* Numeric param, select min or max as appropriate */
		if (((int)p->int_value < int_value && IS_MIN_NUMBER(p->type))
		    || ((int)p->int_value > int_value && IS_MAX_NUMBER(p->type))) {
			/* Error, reply did not do min/max correctly */
			TRACE_ERROR
			    ("got reply %d to offer of %d for %s"
			     " parameter %s\n",
			     int_value, p->int_value,
			     IS_MIN_NUMBER (p->type) ? "min" : "max",
			     p->parameter_name);
			TRACE(TRACE_ENTER_LEAVE, 
				"Leave handle_params_resp with error");
			*out_length = -1;
		} else {
			/* assign the negotiated value */
			update_key_value(p, int_value, value);
		}
		TRACE(TRACE_ENTER_LEAVE, "Leave handle_params_resp");
		return 1;

	}
	TRACE(TRACE_ENTER_LEAVE, "Leave handle_params_resp expected ");
	return 1;
}

/*
 * scan through the table and count the keys of each type
 * that still remain to be negotiated 
 * p_param_tbl - pointer to the table of parameters
 * returns counts in last 3 parameters 
 */
void
scan_table_and_count(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		     int *nsecurity, int *ninformational, int *noperational)
{
	struct parameter_type *p;
	int i;

	TRACE(TRACE_ENTER_LEAVE, "Enter scan_table_and_count");

	/* initialize all counts to 0 */
	*nsecurity = *ninformational = *noperational = 0;

	for (i = 0, p = p_param_tbl; i < MAX_CONFIG_PARAMS; i++, p++) {
		if ((IS_KEY_TO_BE_NEGOTIATED(p->neg_info))
		    && (!IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info))) {
			/* only count those params that still 
				remain to be negotiated */
			if (IS_SECURITY_PARAM(p->type))
				*nsecurity += 1;
			else if (IS_INFORMATIONAL_PARAM(p->type))
				*ninformational += 1;
			else if (IS_OPERATIONAL_PARAM(p->type))
				*noperational += 1;
		}
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave scan_table_and_count");
}

/*
 * scan through the table and process keys 
 * p_param_tbl - pointer to the table of parameters
 * process_these_types - types of parameters to be processed.
 * Return: returns the length of the string added to the output string
 */
int
scan_table_and_process(int sock,
		       struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		       int process_these_types,
		       int flags_to_be_set,
		       int role,
		       struct generic_pdu *inputpdu,
		       struct generic_pdu *outputpdu,
		       uint32_t flags, 
		       uint64_t * login_flags)
{
	struct parameter_type *p;
	int resp_len = 0;
	char *dummy_string;
	int out_length = 0;
	char *output_string;
	int i;

        UNUSED(sock);
        UNUSED(role);
        UNUSED(inputpdu);
        UNUSED(flags);

	TRACE(TRACE_ENTER_LEAVE, "Enter scan_table_and_process");

	ALLOCATE_MAX_TEXT_LEN(dummy_string);

	output_string = outputpdu->text + outputpdu->text_length;

	for (i = 0, p = p_param_tbl; i < MAX_CONFIG_PARAMS; i++, p++) {
		if ((IS_KEY_TO_BE_NEGOTIATED(p->neg_info))
		    && (p->type & process_these_types)
		    && (!IS_KEY_SENT_TO_OTHER_SIDE(p->neg_info))) {
			TRACE(TRACE_DEBUG, "Process table parameter: %s",
			      p->parameter_name);
			/* Add it to the outgoing data */
			if (IS_NUMBER(p->type)) {
				resp_len = sprintf(dummy_string,
						   "%s=%d", p->parameter_name,
						   p->int_value);
			} else if (p->value_list != NULL) {
				resp_len = sprintf(dummy_string,
						   "%s=%s", p->parameter_name,
						   p->value_list);
			} else if (p->str_value != NULL) {
				resp_len = sprintf(dummy_string,
						   "%s=%s", p->parameter_name,
						   p->str_value);
			}

			if (p->special_key_flag & flags_to_be_set) {
				/* caller wants to know that this key 
				 * was processed
				 */
				*login_flags |= p->special_key_flag;
			}

			/* Always check if the Text length 
			 * exceeds MAX_TEXT_LEN 
			 * Then the remaining parameters have to be sent 
			 * in a separate login command/response 
			 */
			if (check_out_length
			    (outputpdu->text_length + out_length, resp_len)) {
				/* set the more_to_send flag */
				*login_flags |= MORE_TO_SEND_FLAG;
				break;
			} else {
				/* Update resp_len and the pointer to 
					output_string */
				strcpy(output_string, dummy_string);
				output_string += resp_len + 1;

				TRACE(TRACE_ISCSI, "Attach key: %s",
				      dummy_string);

				/* update out_length */
				out_length += resp_len + 1;

				/* set sent bit */
				p->neg_info |= KEY_SENT_TO_OTHER_SIDE;

				/* in 2 special cases, set the reply 
					optional bit */
				if ((IS_BOOL_AND(p->type)
				     && strcmp(p->str_value,
					       key_table->no) == 0)
				    || (IS_BOOL_OR(p->type)
					&& strcmp(p->str_value,
						  key_table->yes) == 0)) {
					p->neg_info |= KEY_REPLY_OPTIONAL;
					TRACE(TRACE_DEBUG,
					      "Reply optional for %s",
					      dummy_string);
				} else if (!IS_INFORMATIONAL_PARAM(p->type)) {
					TRACE(TRACE_DEBUG,
					      "Reply required for %s",
					      dummy_string);
				} else {
					TRACE(TRACE_DEBUG,
					      "Reply not required for %s",
					      dummy_string);
				}
				resp_len = 0;
			}
		}
	}

	FREE_STRING(dummy_string);

	TRACE(TRACE_ENTER_LEAVE,
	      "Leave scan_table_and_process, out_length %d", out_length);

	/* return the length added to the output_string */
	return out_length;
}

/*	
 * Uses the final values negotiated during a successful login of a new session
 * to set up the session-wide operational values used during FFP
 */
void
set_session_parameters(struct session_operational_parameters *oper_param_entry,
		       struct parameter_type login_params[MAX_CONFIG_PARAMS])
{
	struct parameter_type *p;
	uint32_t i;

	TRACE(TRACE_ENTER_LEAVE, "Enter set_session_parameters");

	for (p = login_params, i = 0; i < MAX_CONFIG_PARAMS; p++, i++) {
		/* For MaxConnections */
		if (p->special_key_flag & MAXCONNECTIONS_FLAG)
			oper_param_entry->MaxConnections = p->int_value;

		/* For InitialR2T */
		else if (p->special_key_flag & INITIALR2T_FLAG)
			oper_param_entry->InitialR2T =
			    !strcmp(p->str_value, key_table->yes);

		/* For ImmediateData */
		else if (p->special_key_flag & IMMEDIATEDATA_FLAG)
			oper_param_entry->ImmediateData =
			    !strcmp(p->str_value, key_table->yes);

		/* For MaxBurstLength */
		else if (p->special_key_flag & MAXBURSTLENGTH_FLAG)
			oper_param_entry->MaxBurstLength =
			    (p->int_value / 512) * 512;

		/* For FirstBurstLength */
		else if (p->special_key_flag & FIRSTBURSTLENGTH_FLAG)
			oper_param_entry->FirstBurstLength =
			    (p->int_value / 512) * 512;

		/* For DefaultTime2Wait */
		else if (p->special_key_flag & DEFAULTTIME2WAIT_FLAG)
			oper_param_entry->DefaultTime2Wait = p->int_value;

		/* For DefaultTime2Retain */
		else if (p->special_key_flag & DEFAULTTIME2RETAIN_FLAG)
			oper_param_entry->DefaultTime2Retain = p->int_value;

		/* For MaxOutstandingR2T */
		else if (p->special_key_flag & MAXOUTSTANDINGR2T_FLAG)
			oper_param_entry->MaxOutstandingR2T = p->int_value;

		/* For DataPDUInOrder */
		else if (p->special_key_flag & DATAPDUINORDER_FLAG)
			oper_param_entry->DataPDUInOrder =
			    !strcmp(p->str_value, key_table->yes);

		/* For DataSequenceInOrder */
		else if (p->special_key_flag & DATASEQUENCEINORDER_FLAG)
			oper_param_entry->DataSequenceInOrder =
			    !strcmp(p->str_value, key_table->yes);

		/* For ErrorRecoveryLevel */
		else if (p->special_key_flag & ERRORRECOVERYLEVEL_FLAG)
			oper_param_entry->ErrorRecoveryLevel = p->int_value;

		/* For SessionType */
		else if (p->special_key_flag & SESSIONTYPE_FLAG) {
			if ((oper_param_entry->SessionType =
			     !strcmp(p->str_value, key_table->discovery))) {
				/* discovery session */
				/*  Draft 20, Section 12.21 SessionType
				 *  "The discovery session implies 
				 *  MaxConnections = 1 and overrides both 
				 *  the default and an explicit setting."
				 */
				oper_param_entry->MaxConnections = 1;
			}
		}

		/* For TargetName (on target, need the number from this name) */
		else if (p->special_key_flag & TARGETNAME_FLAG) {
			/* save number from end of TargetName for the target */
			oper_param_entry->TargetName = p->int_value;
		}
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave set_session_parameters");
}

/*
 * Uses the final values negotiated during a successful login of a new
 * connection to set up the connection-specific operational values used
 * during FFP
 */
void
set_connection_parameters(struct connection_operational_parameters
					*oper_param_entry,
			  struct parameter_type
					login_params[MAX_CONFIG_PARAMS])
{

        UNUSED(oper_param_entry);
        UNUSED(login_params);

	TRACE(TRACE_ENTER_LEAVE, "Enter set_connection_parameters");

	/*  for now, there are no connection-specific values used during
	 *  FFP.  These would be the security values and the marker values.
	 */

	TRACE(TRACE_ENTER_LEAVE, "Leave set_connection_parameters");
}

/*
 * Returns -1 on error (duplicate key)
 *	    0 if not a duplicate
 */
int __attribute__ ((no_instrument_function))
check_step_key(struct unknown_key *key, uint32_t * got_keys, uint32_t got_value)
{
	if (*got_keys & got_value) {
		TRACE_ERROR("duplicate key %s, value \"%s\"\n", key->keyname,
			    key->keyvalue);
		return -1;
	}
	TRACE(TRACE_ISCSI_FULL, "got %s: %s", key->keyname, key->keyvalue);
	*got_keys |= got_value;
	return 0;
}

/*	
 * Returns -1 on error (duplicate key or illegal numeric value)
 *	    0 if not a duplicate, numeric_value is set
 */
int __attribute__ ((no_instrument_function))
check_step_key_number(struct unknown_key *key, uint32_t * got_keys,
		      uint32_t got_value, uint32_t limit, uint32_t * numeric_value)
{
	int retval;
	char *tmp_string;

	if ((retval = check_step_key(key, got_keys, got_value)) == 0) {
		*numeric_value = strtoul(key->keyvalue, &tmp_string, 0);
		if (!isdigit(*key->keyvalue)
		    || tmp_string == NULL
		    || *tmp_string != '\0' || *numeric_value > limit) {
			TRACE_ERROR("invalid %s number \"%s\"\n", key->keyname,
				    key->keyvalue);
			retval = -1;
		}
	}

	return retval;
}

static int __attribute__ ((no_instrument_function))
print_int_param(char *buffer, char *name, int value)
{
	return sprintf(buffer, "%30s  %d\n", name, value);
}

static int __attribute__ ((no_instrument_function))
print_boolean_param(char *buffer, char *name, int value)
{
	return sprintf(buffer, "%30s  %s\n", name,
			value ? key_table->yes : key_table->no);
}

static int __attribute__ ((no_instrument_function))
print_string_param(char *buffer, char *name, char *value)
{
	return sprintf(buffer, "%30s  %s\n", name, (value ? value : "<NULL>"));
}

/*	
 * prints all the keys in the param_tbl and their values
 */
int __attribute__ ((no_instrument_function))
print_config_info(struct parameter_type param_tbl[MAX_CONFIG_PARAMS],
                  char *buffer)
{
	struct parameter_type *params;
	int pos;
	int i;

	pos = sprintf(buffer, "    configured parameters\n");

	for (i = 0, params = param_tbl; i < MAX_CONFIG_PARAMS; params++, i++) {
		if (params->type & NUMBER) {
			pos += print_int_param(buffer + pos,
						params->parameter_name,
						params->int_value);

		} else {
			pos += print_string_param(buffer + pos,
						  params->parameter_name,
						  params->value_list);
		}
	}

	pos += sprintf(buffer + pos, "\n");

	return pos;
}

void
iscsi_convert_param_to_str(char *buffer,
                           const char *param,
                           struct parameter_type *param_tbl)
{
    struct parameter_type *found = find_parameter(param, param_tbl);
    
    if (found == NULL)
        *buffer = '\0';
    else
    {
		if (found->type & NUMBER) 
        {
            sprintf(buffer, "%d", found->int_value);
		} 
        else 
        {
            strcpy(buffer, found->value_list);
		}
    }
}

/*	
 * Prints the session-wide parameter values as negotiated during a
 * successful login of the first connection in a new session
 */
int __attribute__ ((no_instrument_function))
print_session_params(struct session_operational_parameters *sop,
		     struct parameter_type param_tbl[MAX_CONFIG_PARAMS],
		     char *buffer)
{
	struct parameter_type *p;
	int pos, i;

	pos = sprintf(buffer, "    session-wide parameters\n");

	for (p = param_tbl, i = 0; i < MAX_CONFIG_PARAMS; p++, i++) {
		/* For InitiatorName 12.5 */
		if (p->special_key_flag & INITIATORNAME_FLAG) {
			pos += print_string_param(buffer + pos,
						  p->parameter_name,
						  p->str_value);

		/* For TargetName 12.4 */
		} else if (p->special_key_flag & TARGETNAME_FLAG) {
			pos += print_string_param(buffer + pos,
						  p->parameter_name,
						  p->str_value);

		/* For TargetPortalGroupTag 12.9 */
		} else if (p->special_key_flag & TARGETPORTALGROUPTAG_FLAG) {
			pos += print_int_param(buffer + pos,
						  p->parameter_name,
						  p->int_value);

		/* For MaxConnections 12.2 */
		} else if (p->special_key_flag & MAXCONNECTIONS_FLAG) {
			pos += print_int_param(buffer + pos,
					       p->parameter_name,
					       sop->MaxConnections);

		/* For InitialR2T 12.10 */
		} else if (p->special_key_flag & INITIALR2T_FLAG) {
			pos += print_boolean_param(buffer + pos,
						   p->parameter_name,
						   sop->InitialR2T);

		/* For ImmediateData 12.11 */
		} else if (p->special_key_flag & IMMEDIATEDATA_FLAG) {
			pos += print_boolean_param(buffer + pos,
						   p->parameter_name,
						   sop->ImmediateData);

		/* For MaxBurstLength 12.13 */
		} else if (p->special_key_flag & MAXBURSTLENGTH_FLAG) {
			pos += print_int_param(buffer + pos,
					       p->parameter_name,
					       sop->MaxBurstLength);

		/* For FirstBurstLength 12.14 */
		} else if (p->special_key_flag & FIRSTBURSTLENGTH_FLAG) {
			pos += print_int_param(buffer + pos,
					       p->parameter_name,
					       sop->FirstBurstLength);

		/* For DefaultTime2Wait 12.15 */
		} else if (p->special_key_flag & DEFAULTTIME2WAIT_FLAG) {
			pos += print_int_param(buffer + pos,
				               p->parameter_name,
					       sop-> DefaultTime2Wait);

		/* For DefaultTime2Retain 12.16 */
		} else if (p->special_key_flag & DEFAULTTIME2RETAIN_FLAG) {
			pos += print_int_param(buffer + pos,
					       p->parameter_name,
					       sop-> DefaultTime2Retain);

		/* For MaxOutstandingR2T 12.17 */
		} else if (p->special_key_flag & MAXOUTSTANDINGR2T_FLAG) {
			pos += print_int_param(buffer + pos,
					       p->parameter_name,
					       sop->MaxOutstandingR2T);

		/* For DataPDUInOrder 12.18 */
		} else if (p->special_key_flag & DATAPDUINORDER_FLAG) {
			pos += print_boolean_param(buffer + pos,
						   p->parameter_name,
						   sop->DataPDUInOrder);

		/* For DataSequenceInOrder 12.19 */
		} else if (p->special_key_flag & DATASEQUENCEINORDER_FLAG) {
			pos += print_boolean_param(buffer + pos,
						   p->parameter_name,
						   sop->DataSequenceInOrder);

		/* For ErrorRecoveryLevel 12.20 */
		} else if (p->special_key_flag & ERRORRECOVERYLEVEL_FLAG) {
			pos += print_int_param(buffer + pos,
					       p->parameter_name,
					       sop->ErrorRecoveryLevel);

		/* For SessionType 12.21 */
		} else if (p->special_key_flag & SESSIONTYPE_FLAG) {
			pos += sprintf(buffer + pos, "%*s  %s\n", 30,
				       p->parameter_name,
				       sop->SessionType
						? key_table->discovery
						: key_table->normal);
		}
	}
	return pos;
}
