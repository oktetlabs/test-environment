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
#include <linux/string.h>
#include <linux/ip.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "my_memory.h"
#include "iscsi_common.h"
#include "debug.h"

/* chap and srp support -CHONG */
#include "../security/chap.h"
#include "../security/srp.h"


#include "text_param.h"

/* Store the information about the configurable parameters in this array.
 * The values in this table are ALWAYS updated to the latest draft (20).
 */
struct parameter_type config_params[MAX_CONFIG_PARAMS] = {
	{
	 parameter_name:HEADERDIGEST,
	 type:
	 OPERATIONAL_PARAM |
	 INITIAL_ONLY |
	 USE_BY_INITIATOR | USE_BY_TARGET |
	 KEY_MULTI_VALUE | ENUMERATED | DIGEST_PARAM,
	 str_value:NONE,
	 value_list:NONE,
	 special_key_flag:HEADERDIGEST_FLAG,
	 },
	{
	 parameter_name:DATADIGEST,
	 type:
	 OPERATIONAL_PARAM |
	 INITIAL_ONLY |
	 USE_BY_INITIATOR | USE_BY_TARGET |
	 KEY_MULTI_VALUE | ENUMERATED | DIGEST_PARAM,
	 str_value:NONE,
	 value_list:NONE,
	 special_key_flag:DATADIGEST_FLAG,
	 },
	{
	 parameter_name:MAXCONNECTIONS,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_INITIATOR | USE_BY_TARGET |
	 KEY_SINGLE_VALUE | NUMBER | ONE_TO_65535 | MIN_NUMBER,
	 int_value:2,
	 special_key_flag:MAXCONNECTIONS_FLAG,
	 },
	{
	 parameter_name:SENDTARGETS,
	 type:
	 FULL_FEATURE_ONLY | USE_BY_INITIATOR | KEY_NO_VALUE | STRING | UTF_8,
	 str_value:NULL,
	 value_list:NULL,
	 special_key_flag:SENDTARGETS_FLAG,
	 },
	{
	 parameter_name:TARGETNAME,
	 type:
	 INFORMATIONAL_PARAM |
	 ALL |
	 USE_BY_INITIATOR | USE_BY_TARGET | KEY_SINGLE_VALUE | STRING |
	 ISCSI_NAME,
	 int_value:0,		/* exceptional use of this field by a string */
	 str_value:NULL,
	 value_list:NULL,
	 special_key_flag:TARGETNAME_FLAG,
	 },
	{
	 parameter_name:INITIATORNAME,
	 type:
	 INFORMATIONAL_PARAM |
	 INITIAL_ONLY | USE_BY_INITIATOR | KEY_SINGLE_VALUE | STRING |
	 ISCSI_NAME,
	 str_value:NULL,
	 value_list:NULL,
	 special_key_flag:INITIATORNAME_FLAG,
	 },
	{
	 parameter_name:TARGETALIAS,
	 type:
	 INFORMATIONAL_PARAM |
	 ALL | USE_BY_TARGET | KEY_SINGLE_VALUE | STRING | UTF_8,
	 str_value:NULL,
	 value_list:NULL,
	 },
	{
	 parameter_name:INITIATORALIAS,
	 type:
	 INFORMATIONAL_PARAM |
	 ALL | USE_BY_INITIATOR | KEY_SINGLE_VALUE | STRING | UTF_8,
	 str_value:NULL,
	 value_list:NULL,
	 },
	{
	 parameter_name:TARGETADDRESS,
	 type:
	 INFORMATIONAL_PARAM |
	 ALL | USE_BY_TARGET | KEY_SINGLE_VALUE | STRING | TARGET_ADDRESS_TYPE,
	 str_value:NULL,
	 value_list:NULL,
	 special_key_flag:TARGETADDRESS_FLAG,
	 },
	{
	 parameter_name:OFMARKER,
	 type:
	 OPERATIONAL_PARAM |
	 INITIAL_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR | KEY_SINGLE_VALUE | BOOL_AND,
	 str_value:NO,
	 value_list:NO,
	 special_key_flag:OFMARKER_FLAG,
	 },
	{
	 parameter_name:IFMARKER,
	 type:
	 OPERATIONAL_PARAM |
	 INITIAL_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR | KEY_SINGLE_VALUE | BOOL_AND,
	 str_value:NO,
	 value_list:NO,
	 special_key_flag:IFMARKER_FLAG,
	 },
	{
	 parameter_name:OFMARKINT,
	 type:
	 OPERATIONAL_PARAM |
	 INITIAL_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR |
	 KEY_TWO_VALUE | NUMBER_RANGE | ONE_TO_65535,
	 int_value:2048,
	 str_value:"2048~2048",
	 value_list:"2048~2048",
	 special_key_flag:OFMARKINT_FLAG,
	 },
	{
	 parameter_name:IFMARKINT,
	 type:
	 OPERATIONAL_PARAM |
	 INITIAL_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR |
	 KEY_TWO_VALUE | NUMBER_RANGE | ONE_TO_65535,
	 int_value:2048,
	 str_value:"2048~2048",
	 value_list:"2048~2048",
	 special_key_flag:IFMARKINT_FLAG,
	 },
	{
	 parameter_name:INITIALR2T,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR | KEY_SINGLE_VALUE | BOOL_OR,
	 str_value:YES,
	 value_list:YES,
	 special_key_flag:INITIALR2T_FLAG,
	 },
	{
	 parameter_name:IMMEDIATEDATA,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR | KEY_SINGLE_VALUE | BOOL_AND,
	 str_value:YES,
	 value_list:YES,
	 special_key_flag:IMMEDIATEDATA_FLAG,
	 },
	{
	 parameter_name:MAXRECVDATASEGMENTLENGTH,
	 type:
	 OPERATIONAL_PARAM |
	 ALL |
	 USE_BY_INITIATOR | USE_BY_TARGET |
	 KEY_SINGLE_VALUE | NUMBER | N512_TO_16777215,
	 int_value:8192,
	 special_key_flag:MAXRECVDATASEGMENTLENGTH_FLAG,
	 },
	{
	 parameter_name:MAXBURSTLENGTH,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_INITIATOR | USE_BY_TARGET |
	 KEY_SINGLE_VALUE | N512_TO_16777215 | MIN_NUMBER | NUMBER,
	 int_value:INITIAL_MAXBURSTLENGTH,
	 special_key_flag:MAXBURSTLENGTH_FLAG,
	 },
	{
	 parameter_name:FIRSTBURSTLENGTH,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_INITIATOR | USE_BY_TARGET |
	 KEY_SINGLE_VALUE | N512_TO_16777215 | MIN_NUMBER | NUMBER,
	 int_value:INITIAL_FIRSTBURSTLENGTH,
	 special_key_flag:FIRSTBURSTLENGTH_FLAG,
	 },
	{
	 parameter_name:DEFAULTTIME2WAIT,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR |
	 KEY_SINGLE_VALUE | NUMBER | ZERO_TO_3600 | MAX_NUMBER,
	 int_value:2,
	 special_key_flag:DEFAULTTIME2WAIT_FLAG,
	 },
	{
	 parameter_name:DEFAULTTIME2RETAIN,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR |
	 KEY_SINGLE_VALUE | NUMBER | ZERO_TO_3600 | MIN_NUMBER,
	 int_value:20,
	 special_key_flag:DEFAULTTIME2RETAIN_FLAG,
	 },
	{
	 parameter_name:MAXOUTSTANDINGR2T,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR |
	 KEY_SINGLE_VALUE | NUMBER | ONE_TO_65535 | MIN_NUMBER,
	 int_value:1,
	 special_key_flag:MAXOUTSTANDINGR2T_FLAG,
	 },
	{
	 parameter_name:DATAPDUINORDER,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_TARGET | USE_BY_INITIATOR | KEY_SINGLE_VALUE | BOOL_OR,
	 str_value:YES,
	 value_list:YES,
	 special_key_flag:DATAPDUINORDER_FLAG,
	 },
	{
	 parameter_name:DATASEQUENCEINORDER,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_INITIATOR | USE_BY_TARGET | KEY_SINGLE_VALUE | BOOL_OR,
	 str_value:YES,
	 value_list:YES,
	 special_key_flag:DATASEQUENCEINORDER_FLAG,
	 },
	{
	 parameter_name:AUTHMETHOD,
	 type:
	 SECURITY_PARAM |
	 INITIAL_ONLY |
	 USE_BY_INITIATOR | USE_BY_TARGET |
	 KEY_MULTI_VALUE | ENUMERATED | AUTH_PARAM,
	 str_value:NONE,
	 value_list:NONE,
	 special_key_flag:AUTHMETHOD_FLAG,
	 },
	{
	 parameter_name:ERRORRECOVERYLEVEL,
	 type:
	 OPERATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_INITIATOR | USE_BY_TARGET |
	 KEY_SINGLE_VALUE | NUMBER | ZERO_TO_2 | MIN_NUMBER,
	 int_value:0,
	 special_key_flag:ERRORRECOVERYLEVEL_FLAG,
	 },
	{
	 parameter_name:SESSIONTYPE,
	 type:
	 INFORMATIONAL_PARAM |
	 LEADING_ONLY |
	 USE_BY_INITIATOR | KEY_SINGLE_VALUE | ENUMERATED | DISCOVERY_NORMAL,
	 str_value:NORMAL,
	 value_list:NORMAL,
	 special_key_flag:SESSIONTYPE_FLAG,
	 },
	{
	 parameter_name:TARGETPORTALGROUPTAG,
	 type:
	 INFORMATIONAL_PARAM |
	 INITIAL_ONLY | USE_BY_TARGET | KEY_SINGLE_VALUE | NUMBER |
	 ZERO_TO_65535,
	 special_key_flag:TARGETPORTALGROUPTAG_FLAG,
	 },
	{/* non-standard key to text X-extension facilities */
	 parameter_name:X_EXTENSIONKEY,
	 type:
	 OPERATIONAL_PARAM |
	 ALL | USE_BY_INITIATOR | USE_BY_TARGET | KEY_SINGLE_VALUE | STRING |
	 UTF_8,
	 str_value:NULL,
	 value_list:NULL,
	 special_key_flag:X_EXTENSIONKEY_FLAG,
	 }
};


