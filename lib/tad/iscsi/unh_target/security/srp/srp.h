/*	security/srp/srp.h

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

#ifndef SRP_H
#define SRP_H

#include "../misc/misc_func.h"

/*	number of SRP groups supported */
#define	SRP_N_GROUPS					5

/* iSCSI Key="SRP-768" [768 bits] */
#define SRP_768							0
#define SRP_768_N_LEN					96
#define SRP_768_G_LEN					1

/* iSCSI Key="SRP-1024" [1024 bits] */
#define SRP_1024						1
#define SRP_1024_N_LEN					128
#define SRP_1024_G_LEN					1

/* iSCSI Key="SRP-1280" [1280 bits] */
#define SRP_1280						2
#define SRP_1280_N_LEN					160
#define SRP_1280_G_LEN					1

/* iSCSI Key="SRP-1536" [1536 bits] */
#define SRP_1536						3
#define SRP_1536_N_LEN					192
#define SRP_1536_G_LEN					1

/* iSCSI Key="SRP-2048" [2048 bits] */
#define SRP_2048						4
#define SRP_2048_N_LEN					256
#define SRP_2048_G_LEN					1

#define	SRP_SALT_LENGTH					10
#define	SRP_A_LENGTH					16
#define	SRP_B_LENGTH					16

struct SRP_Context {
	int format;
	int group;
	char *name;
	char *secret;
	struct dataunit salt;
	struct dataunit verifier;
	struct dataunit S;
	struct dataunit a;
	struct dataunit A;
	struct dataunit b;
	struct dataunit B;
	struct dataunit X;
	struct dataunit u;
	struct dataunit K;
	struct dataunit M;
	struct dataunit HM;
	struct dataunit N;
	struct dataunit generator;
};


// testing purpose
int CalculateX(struct SRP_Context * p_context);

int CalculateVerifier(struct SRP_Context * p_context);

int CalculateTargetS(struct SRP_Context * p_context);

int CalculateInitiatorS(struct SRP_Context * p_context);

//common functions

// initialize the srp context, should be called before calling
// the other functions
// return value: 0 fail, 1 succeed
struct SRP_Context *SRP_InitializeContext(void);

// finalize the srp context, should be called after srp procedure
void SRP_FinalizeContext(struct SRP_Context * p_context);

// print out the current srp context
void SRP_PrintContext(struct SRP_Context * p_context);

// clone the srp context
struct SRP_Context *SRP_CloneContext(struct SRP_Context * p_context);

// set the username in the current srp context to p_username
// return value: 0 fail, 1 succeed
int SRP_SetName(char *p_username, struct SRP_Context * p_context);

// set the username in the current srp context to p_username
// and lookup the database to find the coresponding secret
// return value: 0 fail, 1 succeed
int SRP_SetUsername_DB(char *p_username, struct SRP_Context * p_context);

// set the secret in the current srp context to p_secret
// return value: 0 fail, 1 succeed
int SRP_SetSecret(char *p_secret, struct SRP_Context * p_context);

// set the srp data format
// return value: 0 fail, 1 succeed
int SRP_SetNumberFormat(int p_format, struct SRP_Context * p_context);

// set the srp groups, and select N and generator
// return value: 0 fail, 1 succeed
int SRP_SetSRPGroup(char *p_group, struct SRP_Context * p_context);

// target functions (host)

// set the A in the current srp context to p_A
// return value: 0 fail, 1 succeed
int SRP_Target_SetA(char *p_A, int max_length, struct SRP_Context * p_context);

// compare the p_M with the M in the current srp context
// return value: 0 fail, 1 succeed
int SRP_Target_SetM(char *p_M, int max_length, struct SRP_Context * p_context);

// return the group list it supportst
// return value: NULL fail, otherwise succeed
char *SRP_Target_GetGroupList(struct SRP_Context * p_context);

/*	returns index number of group_name, -1 if not found */
int SRP_GetGroupIndex(char *group_name, struct SRP_Context * p_context);

// return the salt in the current srp context
// return value: NULL fail, otherwise succeed
char *SRP_Target_GetSalt(struct SRP_Context * p_context);

// return the B in the current srp context
// return value: NULL fail, otherwise succeed
char *SRP_Target_GetB(struct SRP_Context * p_context);

// return the HM in the current srp context
// return value: NULL fail, otherwise succeed
char *SRP_Target_GetHM(struct SRP_Context * p_context);

// initiator functions (client)

// set the salt in the current srp context to p_salt
// return value: 0 fail, 1 succeed
int SRP_Initiator_SetSalt(char *p_salt, int max_length,
			  struct SRP_Context * p_context);

// set the B in the current srp context to p_B
// return value: 0 fail, 1 succeed
int SRP_Initiator_SetB(char *p_B, int max_length, struct SRP_Context * p_context);

// compare the  p_HM with the HM in the current srp context
// return value: 0 fail, 1 succeed
int SRP_Initiator_SetHM(char *p_HM, int max_length, struct SRP_Context * p_context);

// return the username U
// return value: NULL fail, otherwise succeed
char *SRP_Initiator_GetUsername(struct SRP_Context * p_context);

// select the group
// return value: NULL fail, otherwise succeed
char *SRP_Initiator_GetGroup(char *p_groups[], struct SRP_Context * p_context);

// return the A in the current srp context
// return value: NULL fail, otherwise succeed
char *SRP_Initiator_GetA(struct SRP_Context * p_context);

// return the M in the current srp context
// return value: NULL fail, otherwise succeed
char *SRP_Initiator_GetM(struct SRP_Context * p_context);
#endif
