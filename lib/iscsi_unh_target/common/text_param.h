/*	common/text_param.h
 *
 *	vi: set autoindent tabstop=8 shiftwidth=8 :
 *
 * 	This defines the generic functions used in the Login phase
 *	by both the iSCSI target and the iSCSI initiator for parameter
 * 	negotiation.
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
 *	The name of IOL and/or UNH may not be used to endorse or promote 
 *	products derived from this software without specific prior 
 * 	written permission.
*/

#ifndef _TEXT_PARAM_H
#define _TEXT_PARAM_H

#include <semaphore.h>
#include <stdint.h>

/*
 * Define the Login/Text Parameters
 */

#define HEADERDIGEST			"HeaderDigest"
#define DATADIGEST			"DataDigest"
#define MAXCONNECTIONS			"MaxConnections"
#define SENDTARGETS			"SendTargets"
#define TARGETNAME			"TargetName"
#define INITIATORNAME			"InitiatorName"
#define TARGETALIAS			"TargetAlias"
#define INITIATORALIAS			"InitiatorAlias"
#define TARGETADDRESS			"TargetAddress"
#define INITIALR2T			"InitialR2T"
#define IMMEDIATEDATA			"ImmediateData"
#define MAXRECVDATASEGMENTLENGTH	"MaxRecvDataSegmentLength"
#define MAXBURSTLENGTH			"MaxBurstLength"
#define FIRSTBURSTLENGTH		"FirstBurstLength"
#define MAXOUTSTANDINGR2T		"MaxOutstandingR2T"
#define DATAPDUINORDER			"DataPDUInOrder"
#define DATASEQUENCEINORDER		"DataSequenceInOrder"
#define ERRORRECOVERYLEVEL		"ErrorRecoveryLevel"
#define SESSIONTYPE			"SessionType"
#define AUTHMETHOD			"AuthMethod"
#define DEFAULTTIME2WAIT		"DefaultTime2Wait"
#define DEFAULTTIME2RETAIN		"DefaultTime2Retain"
#define IFMARKER			"IFMarker"
#define OFMARKER			"OFMarker"
#define IFMARKINT			"IFMarkInt"
#define OFMARKINT			"OFMarkInt"
#define TARGETPORTALGROUPTAG		"TargetPortalGroupTag"
#define X_EXTENSIONKEY			"X-edu.unh.iol-extension-key-1"

/* Following Macros Added for Error Recovery - SAI*/

#define PAYLOAD_DIGERR				0x00000000
#define HEADER_DIGERR				0x00000001
#define SEQUENCE_ERR				0x00000010

/*
 * Define the Options available for the Parameters
 */

/* common for all */
#define NONE				"None"
#define REJECT				"Reject"
#define IRRELEVANT			"Irrelevant"

/* For Digests - (HeaderDigest, DataDigest) */
#define CRC32C				"CRC32C"

/* Different Authentication Methods supported */
#define KRB5				"KRB5"	/* RFC1510 */
#define SPKM1				"SPKM1"	/* RFC2025 */
#define SPKM2				"SPKM2"	/* RFC2025 */
#define SRP				"SRP"	/* RFC2945 */
#define CHAP				"CHAP"	/* RFC1994 */

/* chap and srp support - CHONG */
#define MAX_CHAP_BINARY_LENGTH	1024
#define	MAX_SRP_BINARY_LENGTH	1024

/*	Draft 20, Section 11.1.4 Challenge Handshake Authentication 
 *	Protocol(CHAP)
 */
/*	The keys used at various steps in the handshake exchange */
#define	CHAP_A			"CHAP_A"
#define	CHAP_I			"CHAP_I"
#define	CHAP_C			"CHAP_C"
#define	CHAP_N			"CHAP_N"
#define	CHAP_R			"CHAP_R"

/*	Draft 20, Section 11.1.3 Secure Remote Password (SRP) */
/*	The keys used at various steps in the handshake exchange */
#define	SRP_TARGETAUTH		"TargetAuth"
#define	SRP_U			"SRP_U"
#define	SRP_A			"SRP_A"
#define	SRP_GROUP		"SRP_GROUP"
#define	SRP_S			"SRP_s"
#define	SRP_B			"SRP_B"
#define	SRP_M			"SRP_M"
#define	SRP_HM			"SRP_HM"

/*	Bit masks to mark security keys that we got from other side 
 * during security phase.  These are separate from the security 
 * steps because a single step may require that multiple keys be 
 * received and these keys can come in any order.
 */

/*	Draft 20, Section 11.1.4 Challenge Handshake 
 * Authentication Protocol(CHAP)
 */
#define	GOT_CHAP_A	0x0001
#define	GOT_CHAP_I	0x0002
#define	GOT_CHAP_C	0x0004
#define	GOT_CHAP_N	0x0008
#define	GOT_CHAP_R	0x0010

/*	Draft 20, Section 11.1.3 Secure Remote Password (SRP) */
#define	GOT_SRP_GROUP		0x0020
#define	GOT_SRP_S		0x0040
#define	GOT_SRP_B		0x0080
#define	GOT_SRP_HM		0x0100
#define	GOT_SRP_U		0x0200
#define	GOT_SRP_TARGETAUTH	0x0400
#define	GOT_SRP_A		0x0800
#define	GOT_SRP_M		0x1000

/*******************************************************************/
/* For Parameter types */

/*
 *	byte-3 	byte-2 	byte-1 	byte-0
 *	XX	XX	XX	XX
 */

/* following constants use the byte-3 and byte-2 for specifying type */
#define SECURITY_PARAM		0x80000000
#define OPERATIONAL_PARAM	0x40000000
#define INFORMATIONAL_PARAM	0x20000000
#define LEADING_ONLY		0x10000000
#define INITIAL_ONLY		0x08000000
#define FULL_FEATURE_ONLY	0x04000000
#define ALL			0x02000000
#define USE_BY_INITIATOR	0x01000000
#define USE_BY_TARGET		0x00800000
#define NORMAL_RETURN		0x00400000	/*TAddress in normal loginRPS */
#define REDIRECT_RETURN		0x00200000	/*TAddress for redirection */
#define	SENDTARGET_RETURN	0x00100000	/*TAddr in return to SendTargets */

/* byte-2  for denoting whether an offer of the parameter 
 * can be nothing, just a single value, a 2-value range, or a list of values 
 */
#define KEY_NO_VALUE		0x00080000	/* follow = with <= 1 value */
#define KEY_SINGLE_VALUE	0x00040000	/* follow = with == 1 value */
#define KEY_TWO_VALUE		0x00020000	/* follow = with range low[~top] */
#define KEY_MULTI_VALUE		0x00010000	/* follow = with list >= 1 value */

/* following constants use the byte-1 for specifying type */
#define NUMBER		0x00008000	/* numeric values only */
#define STRING		0x00004000	/* arbitrary strings */
#define ENUMERATED	0x00002000	/* set of specific strings */
#define BOOL_AND	0x00001000	/* boolean values only */
#define BOOL_OR		0x00000800	/* boolean values only */
#define MIN_NUMBER	0x00000400	/* numeric values only */
#define MAX_NUMBER			0x00000200	/* numeric values only */
#define NUMBER_RANGE	0x00000100	/* range of 2 numeric values */

/* byte-0 qualifies the byte-1 */

/* if the byte-1 specifies the type to be NUMBER */
#define ONE_TO_65535		0x00000080
#define N512_TO_16777215	0x00000040
#define ZERO_TO_3600		0x00000020
#define ZERO_TO_2		0x00000008
#define ZERO_TO_65535		0x00000002

/* if the byte-1 specifies the type to be STRING */
#define UTF_8			0x00000080	/* <UTF-8 string> */
#define ISCSI_NAME		0x00000040	/* <iSCSI-Name> */
#define TARGET_ADDRESS_TYPE	0x00000020	/* domainname[:port][,tag] iSCSI-Name */

/* if the byte-1 specifies the type to be ENUMERATED */
#define DIGEST_PARAM		0x00000080	/* Key is a Digest key */
#define AUTH_PARAM		0x00000040	/* Key is a Authentication key */
#define DISCOVERY_NORMAL	0x00000020	/* <Session Type> */

/********************************************************************/

/* NOTE: Whenever a new key is added to the array
 *		struct parameter_type config_params[MAX_CONFIG_PARAMS]
 *		 then MAX_CONFIG_PARAMS must be incremented too!
 */
#define MAX_CONFIG_PARAMS		28


/*	These defines are to be used in the special_key_flag field of the
 *	struct parameter_type to identify only those keys that need "special"
 *	processing.  Introducing these flags eliminates the need for
 *	a lot of costly "strcmp()" operations.  The bit assigned to each
 *	flag is totally arbitrary as long as it is different for each key.
 *	Most, but not every, key has a bit assigned to it -- only the
 *	following 4 keys do not have a bit assigned:
 *		TargetAlias
 *		InitiatorAlias
 *		TargetAddress
 *		X-edu.unh.iol.extension-key
 *
 *	Some of these defines are also used in login_flags.
 *	When asked for, they are set if the corresponding special key is 
 *      found in the input by scan_input_and_process().
 */

/* The first set of 35 flags is used in the special_key_flag field of keys.
 * They must not conflict with the defines in the login_flags
 */
#define TARGETPORTALGROUPTAG_FLAG	0x0080000000LL
#define TARGETNAME_FLAG			0x0040000000LL
#define INITIATORNAME_FLAG		0x0020000000LL
#define SESSIONTYPE_FLAG		0x0010000000LL
#define HEADERDIGEST_FLAG		0x0008000000LL
#define DATADIGEST_FLAG			0x0004000000LL
#define SENDTARGETS_FLAG		0x0002000000LL
#define OFMARKER_FLAG			0x0000080000LL
#define IFMARKER_FLAG			0x0000040000LL
#define OFMARKINT_FLAG			0x0000008000LL
#define IFMARKINT_FLAG			0x0000004000LL
#define DEFAULTTIME2WAIT_FLAG		0x0000000800LL
#define DEFAULTTIME2RETAIN_FLAG		0x0000000400LL
#define AUTHMETHOD_FLAG			0x0000000200LL
#define INITIALR2T_FLAG			0x0000000100LL
#define IMMEDIATEDATA_FLAG		0x0000000040LL
#define DATAPDUINORDER_FLAG		0x0000000020LL
#define DATASEQUENCEINORDER_FLAG	0x0000000010LL
#define MAXCONNECTIONS_FLAG		0x0000000008LL
#define MAXOUTSTANDINGR2T_FLAG		0x0000000004LL
#define ERRORRECOVERYLEVEL_FLAG		0x0000000002LL
#define X_EXTENSIONKEY_FLAG		0x0000000001LL
#define MAXRECVDATASEGMENTLENGTH_FLAG	0x1000000000LL
#define MAXBURSTLENGTH_FLAG		0x2000000000LL
#define FIRSTBURSTLENGTH_FLAG		0x4000000000LL
#define TARGETADDRESS_FLAG		0x8000000000LL

/*	The following defines are used in login_flags only.
 *	They are set and reset during processing, and must not
 *	conflict with the special key flags defined above.
 *
 *	The second set of 2 flags is never used in the special_key_flag field
 *	of keys, only in login_flags.  Therefore, they are 64-bits, as is the
 *	login_flags variable.
 */

/*	set when there are more keys to add to output than fit in one PDU */
#define MORE_TO_SEND_FLAG			0x0100000000LL

/*	set when processing first Login Request/Response, else reset */
#define	FIRST_FLAG					0x0200000000LL

/*	set when in the discovery session, else in normal session */
#define DISCOVERY_FLAG				0x0400000000LL

/************************************************************************/
#define ALLOCATE_MAX_TEXT_LEN(str)	\
do { \
	if( !((str) = (char *)malloc( MAX_TEXT_LEN)) ) \
		{ \
		TRACE_ERROR("unable to allocate memory for" #str "\n"); \
		return -1; \
		} \
	} \
while(0)

#define FREE_STRING(str)	if(str) \
					{ \
					free(str); \
					}

/********************************************************************/

#define YES		"Yes"
#define NO		"No"
#define NORMAL		"Normal"
#define DISCOVERY	"Discovery"
#define NOTUNDERSTOOD	"NotUnderstood"

#define INITIAL_FIRSTBURSTLENGTH	65536
#define INITIAL_MAXBURSTLENGTH		262144


/* This structure holds the parameters required for login-text
 * negotiation
 */
struct parameter_type {
	/* Name of the parameter */
	char *parameter_name;

	/*      Type of the parameter:
	 *      Byte 3 - Security, operational etc..
	 *      Byte 2 - Number of values that can be sent with the key
	 *      Byte 1 - number, string, enumerated, boolean, etc..
	 *      Byte 0 - further qualifies the type depending on Byte-1
	 */
	uint32_t type;

	/*      used only if the type is NUMBER or NUMBER_RANGE,
	 *      not STRING or ENUMERATED or BOOLEAN
	 */
	uint32_t int_value;

	/*      used only if the type is STRING or ENUMERATED or BOOLEAN or
	 *      NUMBER_RANGE, not NUMBER
	 */
	char *str_value;

	/*      used only if the type is STRING or ENUMERATED or BOOLEAN or
	 *      NUMBER_RANGE, not NUMBER
	 * Stores the list of allowed values (for negotiation)
	 */
	char *value_list;

	/*      Contains all the info required for negotiating this parameter
	 *      packed into bits
	 */
	uint32_t neg_info;

	/*      One of the bits in this word will be set if this key
	 *      requires "special" processing  -- each key defines 
	 *	its own special bit
	 */
	uint64_t special_key_flag;

};

/* chap and srp support - CHONG */
struct auth_parameter_type {
	int auth_flags;
	struct CHAP_Context *chap_local_ctx;
	struct CHAP_Context *chap_peer_ctx;
	struct SRP_Context *srp_ctx;
};

/* Macros for checking the type */

#define IS_SECURITY_PARAM(p)		((p) & SECURITY_PARAM)
#define IS_OPERATIONAL_PARAM(p)		((p) & OPERATIONAL_PARAM)
#define IS_INFORMATIONAL_PARAM(p)	((p) & INFORMATIONAL_PARAM)
#define IS_LEADING_ONLY(p)		((p) & LEADING_ONLY)
#define IS_INITIAL_ONLY(p)		((p) & INITIAL_ONLY)
#define IS_FULL_FEATURE_ONLY(p)		((p) & FULL_FEATURE_ONLY)
#define IS_USE_BY_INITIATOR(p)		((p) & USE_BY_INITIATOR)
#define IS_USE_BY_TARGET(p)		((p) & USE_BY_TARGET)
#define IS_KEY_SINGLE_VALUE(p)		((p) & KEY_SINGLE_VALUE)
#define IS_KEY_TWO_VALUE(p)		((p) & KEY_TWO_VALUE)
#define IS_KEY_MULTI_VALUE(p)		((p) & KEY_MULTI_VALUE)
#define IS_KEY_NO_VALUE(p)		((p) & KEY_NO_VALUE)
#define IS_NUMBER(p)			((p) & NUMBER)
#define IS_NUMBER_RANGE(p)		((p) & NUMBER_RANGE)
#define IS_MAX_NUMBER(p)		((p) & MAX_NUMBER)
#define IS_MIN_NUMBER(p)		((p) & MIN_NUMBER)
#define IS_STRING(p)			((p) & STRING)
#define IS_ENUMERATED(p)		((p) & ENUMERATED)
#define IS_BOOL_AND(p)			((p) & BOOL_AND)
#define IS_BOOL_OR(p)			((p) & BOOL_OR)
#define IS_BOOLEAN(p)			((p) & (BOOL_AND|BOOL_OR))

/* If the type is number */
#define IS_ONE_TO_65535(p)		((p) & ONE_TO_65535)
#define IS_ZERO_TO_65535(p)		((p) & ZERO_TO_65535)
#define IS_N512_TO_16777215(p)		((p) & N512_TO_16777215)
#define IS_ZERO_TO_3600(p)		((p) & ZERO_TO_3600)
#define IS_ZERO_TO_2(p)			((p) & ZERO_TO_2)

/* If the type is string */
#define IS_UTF_8(p)			(IS_STRING((p)) && ((p) & UTF_8))
#define IS_ISCSI_NAME(p)		(IS_STRING((p)) && ((p) & ISCSI_NAME))
#define IS_TARGET_ADDRESS_TYPE(p)(IS_STRING((p)) && ((p) & TARGET_ADDRESS_TYPE))

/* If the type is ENUMERATED */
#define IS_DIGEST_PARAM(p)		((p) & DIGEST_PARAM)
#define IS_AUTH_PARAM(p)		((p) & AUTH_PARAM)
#define IS_DISCOVERY_NORMAL(p)		((p) & DISCOVERY_NORMAL)

/**********************************************************************/
/* Macros to test bits used in the "neg_info" variable and key field */

#define IS_KEY_TO_BE_NEGOTIATED(p)	((p) & KEY_TO_BE_NEGOTIATED)
#define IS_KEY_BREAK_CONN_OR_RJT(p)	((p) & KEY_BREAK_CONN_OR_RJT)
#define IS_KEY_SENT_TO_OTHER_SIDE(p)	((p) & KEY_SENT_TO_OTHER_SIDE)
#define IS_KEY_GOT_FROM_OTHER_SIDE(p)	((p) & KEY_GOT_FROM_OTHER_SIDE)
#define IS_KEY_REPLY_OPTIONAL(p)	((p) & KEY_REPLY_OPTIONAL)
#define IS_KEY_IRRELEVANT(p)		((p) & KEY_IRRELEVANT)
#define IS_KEY_BAD(p)			((p) & KEY_BAD)
#define IS_KEY_REJECT(p)		((p) & KEY_REJECT)

struct key_values {
	char *yes;
	char *no;
	char *none;
	char *reject;
	char *irrelevant;
	char *discovery;
	char *normal;
	char *notunderstood;
};

/*	structure used to keep a list of unknown keys sent to us
 *	chap and srp support - CHONG
 */
struct unknown_key {
	uint32_t processed;
	char *keyname;
	char *keyvalue;
	struct unknown_key *next;
};

/* maximum number of unkown keys we will accept before terminating login */
#define	MAX_UNKNOWN_KEYS	8

/************************************************************************/
/* deal with security keys		                                        */
/************************************************************************/

/*	returns 0 on failure, else GOT_XXX value for this key */
uint32_t __attribute__ ((no_instrument_function))
is_securitykey(char *keyname);

/*	returns 0 on failure after printing error message,
 *	else GOT_XXX value for this key
 */
uint32_t __attribute__ ((no_instrument_function))
print_bad_security_key(struct unknown_key *key);

void __attribute__ ((no_instrument_function))
print_not_allowed_security_key(struct unknown_key *key);

/*	called once on module load to setup the security key hash table */
void setup_security_hash_table(void);

/*	Called to get table entry for key identified by keytext string.
 *	Returns pointer to entry if found, NULL if not found.
 */
struct parameter_type * __attribute__ ((no_instrument_function))
find_parameter(const char *keytext,
               struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS]);

/* Called to get pointer to table entry for key identified by its special flag*/
struct parameter_type * __attribute__ ((no_instrument_function))
find_flag_parameter(uint64_t key_flag,
		    struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS]);

/*	Called to set max_recv_length to value we sent to target in a
 *	MaxRecvDataSegmentLength key
 */
void
set_connection_recv_length(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
			   int *max_recv_length);

void
set_digestflags(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		uint32_t * connection_flags);

/*	Check that a number is within legal bounds depending on its type.
 *	Returns 1 if ok, 0 if error
 */
int __attribute__ ((no_instrument_function))
check_bounds(struct parameter_type *p, int int_value, int who_called);

/*	This is a range, check both numbers and their order.
 *	Returns first number in range if ok, -1 if error
 */
int __attribute__ ((no_instrument_function))
check_range(const char *value_list, int value);

void __attribute__ ((no_instrument_function))
strreplace(char **str, const char *new_str);

void __attribute__ ((no_instrument_function))
strreplace_upto(char **str, const char *new_str, int delim);

/* Copy src parameter table to dst, duplicating any strings */
void
param_tbl_cpy(struct parameter_type dst[MAX_CONFIG_PARAMS],
	      struct parameter_type src[MAX_CONFIG_PARAMS]);

/* Copy initial parameter table to dst, duplicating any strings */
void
param_tbl_init(struct parameter_type dst[MAX_CONFIG_PARAMS]);

/* Free any strings referenced in dst parameter table */
void
param_tbl_uncpy(struct parameter_type dst[MAX_CONFIG_PARAMS]);

/** The main function for target configuraton
 *
 *  @param param_neg_info  Negotiation mode
 *  @param key             Parameter name to be set
 *  @param value           Value to be set
 *  @param p_param_tbl     Parameter table to operate on
 */
void iscsi_configure_param_value(int param_neg_info,
                                 const char *key,
                                 const char *value,
                                 struct parameter_type *p_param_tbl);

void
configure_parameter(int param_neg_info,
		    char *ptr_to_keytext,
		    struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS]);

/*	Draft 20, Section 5.2 Text Mode Negotiation
 *	"Some parameters may be subject to integrity rules (e.g., parameter-x
 *	must not exceed parameter-y or parameter-u not 1 implies parameter-v
 *	be Yes).  Whenever required, integrity rules are specified with the
 *	keys.  Checking for compliance with the integrity rule must only be
 *	performed after all the parameters are available (the existent and the
 *	newly negotiated).  An iSCSI target MUST perform integrity checking
 *	before the new parameters take effect.  An initiator MAY perform
 *	integrity checking."
 */
void
check_integrity_rules(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		      uint16_t secondary_connection);


/************************************************************************/
/* This function receives 'length' bytes into buffer from the socket */
/************************************************************************/
int
iscsi_recv_msg(int sock, int length, char *buffer, int flags);

/*
 * returns 1 if needs a break from the loop
 * else returns a 0
 */
int __attribute__ ((no_instrument_function))
check_out_length(int out_length, int resp_len);

/*
 * sends the PDUs to the other end.
 */
int
iscsi_send_msg(int sock, struct generic_pdu *outputpdu, int flags);

/*
 * checks whether the receiving side has received all the responses
 * for the negotiations it started in the last packet.
 * Return:
 * 	-1 if error, didn't receive the response
 * 	0 if success
 */
int
check_neg_responses(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		    uint32_t print_error);

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
			struct unknown_key **unknown_key_list);

/*
 * scan through the table and count the keys of each type
 * that still remain to be negotiated
 * p_param_tbl - pointer to the table of parameters
 * returns counts in last 3 parameters
 */
void
scan_table_and_count(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
		     int *nsecurity, int *ninformational, int *noperational);

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
		       uint64_t * login_flags);

/*	
 * Uses the final values negotiated during a successful login of a new session
 * to set up the session-wide operational values used during FFP
 */
void
set_session_parameters(struct session_operational_parameters *oper_param_entry,
		       struct parameter_type login_params[MAX_CONFIG_PARAMS]);

/*
 * Uses the final values negotiated during a successful login of a new
 * connection to set up the connection-specific operational values used
 * during FFP
 */
void
set_connection_parameters(struct connection_operational_parameters
					*oper_param_entry,
			  struct parameter_type
					login_params[MAX_CONFIG_PARAMS]);

/*
 * Returns -1 on error (duplicate key)
 *	    0 if not a duplicate
 */
int __attribute__ ((no_instrument_function))
check_step_key(struct unknown_key *key, uint32_t * got_keys, uint32_t got_value);

/* Returns -1 on error (duplicate key or illegal numeric value)
 *	    0 if not a duplicate, numeric_value is set
 */
int __attribute__ ((no_instrument_function))
check_step_key_number(struct unknown_key *key, uint32_t * got_keys,
		       uint32_t got_value, uint32_t limit, uint32_t * numeric_value);

/*	
 * prints all the keys in the param_tbl and their values
 */
int __attribute__ ((no_instrument_function))
print_config_info(struct parameter_type param_tbl[MAX_CONFIG_PARAMS],
		  char *buffer);

/** 
 * Puts a textual presentation of a `param' into a `buffer'
 *
 * @param buffer    Output buffer
 * @param param     Parameter name
 * @apram param_tbl Parameter table to operate
 */
void iscsi_convert_param_to_str(char *buffer,
                                const char *param,
                                struct parameter_type *param_tbl);


/*	
 * Prints the session-wide parameter values as negotiated during a
 * successful login of the first connection in a new session
 */
int __attribute__ ((no_instrument_function))
print_session_params(struct session_operational_parameters *sop,
		     struct parameter_type param_tbl[MAX_CONFIG_PARAMS],
		     char *buffer);

extern struct parameter_type config_params[];

void iscsi_restore_default_param (const char *name, 
                                  struct parameter_type *param_tbl);

#endif
