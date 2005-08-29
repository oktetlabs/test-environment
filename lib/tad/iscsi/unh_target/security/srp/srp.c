/*	security/srp/srp.c

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

#include <string.h>
#include <linux/slab.h>
#include <linux/stddef.h>

#include "../../userland_lib/my_memory.h"
#include "../../common/iscsi_common.h"
#include "../../common/debug.h"

#include "srp.h"

#include "../misc/misc_func.h"
#include "../hash/sha1.h"
#include "../math/bigint.h"

// inner functions

static char SRP_768_N[] = {
	0xB3, 0x44, 0xC7, 0xC4, 0xF8, 0xC4, 0x95, 0x03,
	0x1B, 0xB4, 0xE0, 0x4F, 0xF8, 0xF8, 0x4E, 0xE9,
	0x50, 0x08, 0x16, 0x39, 0x40, 0xB9, 0x55, 0x82,
	0x76, 0x74, 0x4D, 0x91, 0xF7, 0xCC, 0x9F, 0x40,
	0x26, 0x53, 0xBE, 0x71, 0x47, 0xF0, 0x0F, 0x57,
	0x6B, 0x93, 0x75, 0x4B, 0xCD, 0xDF, 0x71, 0xB6,
	0x36, 0xF2, 0x09, 0x9E, 0x6F, 0xFF, 0x90, 0xE7,
	0x95, 0x75, 0xF3, 0xD0, 0xDE, 0x69, 0x4A, 0xFF,
	0x73, 0x7D, 0x9B, 0xE9, 0x71, 0x3C, 0xEF, 0x8D,
	0x83, 0x7A, 0xDA, 0x63, 0x80, 0xB1, 0x09, 0x3E,
	0x94, 0xB6, 0xA5, 0x29, 0xA8, 0xC6, 0xC2, 0xBE,
	0x33, 0xE0, 0x86, 0x7C, 0x60, 0xC3, 0x26, 0x2B
};

static char SRP_768_G[] = { 2 };

static char SRP_1024_N[] = {
	0xEE, 0xAF, 0x0A, 0xB9, 0xAD, 0xB3, 0x8D, 0xD6,
	0x9C, 0x33, 0xF8, 0x0A, 0xFA, 0x8F, 0xC5, 0xE8,
	0x60, 0x72, 0x61, 0x87, 0x75, 0xFF, 0x3C, 0x0B,
	0x9E, 0xA2, 0x31, 0x4C, 0x9C, 0x25, 0x65, 0x76,
	0xD6, 0x74, 0xDF, 0x74, 0x96, 0xEA, 0x81, 0xD3,
	0x38, 0x3B, 0x48, 0x13, 0xD6, 0x92, 0xC6, 0xE0,
	0xE0, 0xD5, 0xD8, 0xE2, 0x50, 0xB9, 0x8B, 0xE4,
	0x8E, 0x49, 0x5C, 0x1D, 0x60, 0x89, 0xDA, 0xD1,
	0x5D, 0xC7, 0xD7, 0xB4, 0x61, 0x54, 0xD6, 0xB6,
	0xCE, 0x8E, 0xF4, 0xAD, 0x69, 0xB1, 0x5D, 0x49,
	0x82, 0x55, 0x9B, 0x29, 0x7B, 0xCF, 0x18, 0x85,
	0xC5, 0x29, 0xF5, 0x66, 0x66, 0x0E, 0x57, 0xEC,
	0x68, 0xED, 0xBC, 0x3C, 0x05, 0x72, 0x6C, 0xC0,
	0x2F, 0xD4, 0xCB, 0xF4, 0x97, 0x6E, 0xAA, 0x9A,
	0xFD, 0x51, 0x38, 0xFE, 0x83, 0x76, 0x43, 0x5B,
	0x9F, 0xC6, 0x1D, 0x2F, 0xC0, 0xEB, 0x06, 0xE3
};

static char SRP_1024_G[] = { 2 };

static char SRP_1280_N[] = {
	0xD7, 0x79, 0x46, 0x82, 0x6E, 0x81, 0x19, 0x14,
	0xB3, 0x94, 0x01, 0xD5, 0x6A, 0x0A, 0x78, 0x43,
	0xA8, 0xE7, 0x57, 0x5D, 0x73, 0x8C, 0x67, 0x2A,
	0x09, 0x0A, 0xB1, 0x18, 0x7D, 0x69, 0x0D, 0xC4,
	0x38, 0x72, 0xFC, 0x06, 0xA7, 0xB6, 0xA4, 0x3F,
	0x3B, 0x95, 0xBE, 0xAE, 0xC7, 0xDF, 0x04, 0xB9,
	0xD2, 0x42, 0xEB, 0xDC, 0x48, 0x11, 0x11, 0x28,
	0x32, 0x16, 0xCE, 0x81, 0x6E, 0x00, 0x4B, 0x78,
	0x6C, 0x5F, 0xCE, 0x85, 0x67, 0x80, 0xD4, 0x18,
	0x37, 0xD9, 0x5A, 0xD7, 0x87, 0xA5, 0x0B, 0xBE,
	0x90, 0xBD, 0x3A, 0x9C, 0x98, 0xAC, 0x0F, 0x5F,
	0xC0, 0xDE, 0x74, 0x4B, 0x1C, 0xDE, 0x18, 0x91,
	0x69, 0x08, 0x94, 0xBC, 0x1F, 0x65, 0xE0, 0x0D,
	0xE1, 0x5B, 0x4B, 0x2A, 0xA6, 0xD8, 0x71, 0x00,
	0xC9, 0xEC, 0xC2, 0x52, 0x7E, 0x45, 0xEB, 0x84,
	0x9D, 0xEB, 0x14, 0xBB, 0x20, 0x49, 0xB1, 0x63,
	0xEA, 0x04, 0x18, 0x7F, 0xD2, 0x7C, 0x1B, 0xD9,
	0xC7, 0x95, 0x8C, 0xD4, 0x0C, 0xE7, 0x06, 0x7A,
	0x9C, 0x02, 0x4F, 0x9B, 0x7C, 0x5A, 0x0B, 0x4F,
	0x50, 0x03, 0x68, 0x61, 0x61, 0xF0, 0x60, 0x5B
};

static char SRP_1280_G[] = { 2 };

static char SRP_1536_N[] = {
	0x9D, 0xEF, 0x3C, 0xAF, 0xB9, 0x39, 0x27, 0x7A,
	0xB1, 0xF1, 0x2A, 0x86, 0x17, 0xA4, 0x7B, 0xBB,
	0xDB, 0xA5, 0x1D, 0xF4, 0x99, 0xAC, 0x4C, 0x80,
	0xBE, 0xEE, 0xA9, 0x61, 0x4B, 0x19, 0xCC, 0x4D,
	0x5F, 0x4F, 0x5F, 0x55, 0x6E, 0x27, 0xCB, 0xDE,
	0x51, 0xC6, 0xA9, 0x4B, 0xE4, 0x60, 0x7A, 0x29,
	0x15, 0x58, 0x90, 0x3B, 0xA0, 0xD0, 0xF8, 0x43,
	0x80, 0xB6, 0x55, 0xBB, 0x9A, 0x22, 0xE8, 0xDC,
	0xDF, 0x02, 0x8A, 0x7C, 0xEC, 0x67, 0xF0, 0xD0,
	0x81, 0x34, 0xB1, 0xC8, 0xB9, 0x79, 0x89, 0x14,
	0x9B, 0x60, 0x9E, 0x0B, 0xE3, 0xBA, 0xB6, 0x3D,
	0x47, 0x54, 0x83, 0x81, 0xDB, 0xC5, 0xB1, 0xFC,
	0x76, 0x4E, 0x3F, 0x4B, 0x53, 0xDD, 0x9D, 0xA1,
	0x15, 0x8B, 0xFD, 0x3E, 0x2B, 0x9C, 0x8C, 0xF5,
	0x6E, 0xDF, 0x01, 0x95, 0x39, 0x34, 0x96, 0x27,
	0xDB, 0x2F, 0xD5, 0x3D, 0x24, 0xB7, 0xC4, 0x86,
	0x65, 0x77, 0x2E, 0x43, 0x7D, 0x6C, 0x7F, 0x8C,
	0xE4, 0x42, 0x73, 0x4A, 0xF7, 0xCC, 0xB7, 0xAE,
	0x83, 0x7C, 0x26, 0x4A, 0xE3, 0xA9, 0xBE, 0xB8,
	0x7F, 0x8A, 0x2F, 0xE9, 0xB8, 0xB5, 0x29, 0x2E,
	0x5A, 0x02, 0x1F, 0xFF, 0x5E, 0x91, 0x47, 0x9E,
	0x8C, 0xE7, 0xA2, 0x8C, 0x24, 0x42, 0xC6, 0xF3,
	0x15, 0x18, 0x0F, 0x93, 0x49, 0x9A, 0x23, 0x4D,
	0xCF, 0x76, 0xE3, 0xFE, 0xD1, 0x35, 0xF9, 0xBB
};

static char SRP_1536_G[] = { 2 };

static char SRP_2048_N[] = {
	0xAC, 0x6B, 0xDB, 0x41, 0x32, 0x4A, 0x9A, 0x9B,
	0xF1, 0x66, 0xDE, 0x5E, 0x13, 0x89, 0x58, 0x2F,
	0xAF, 0x72, 0xB6, 0x65, 0x19, 0x87, 0xEE, 0x07,
	0xFC, 0x31, 0x92, 0x94, 0x3D, 0xB5, 0x60, 0x50,
	0xA3, 0x73, 0x29, 0xCB, 0xB4, 0xA0, 0x99, 0xED,
	0x81, 0x93, 0xE0, 0x75, 0x77, 0x67, 0xA1, 0x3D,
	0xD5, 0x23, 0x12, 0xAB, 0x4B, 0x03, 0x31, 0x0D,
	0xCD, 0x7F, 0x48, 0xA9, 0xDA, 0x04, 0xFD, 0x50,
	0xE8, 0x08, 0x39, 0x69, 0xED, 0xB7, 0x67, 0xB0,
	0xCF, 0x60, 0x95, 0x17, 0x9A, 0x16, 0x3A, 0xB3,
	0x66, 0x1A, 0x05, 0xFB, 0xD5, 0xFA, 0xAA, 0xE8,
	0x29, 0x18, 0xA9, 0x96, 0x2F, 0x0B, 0x93, 0xB8,
	0x55, 0xF9, 0x79, 0x93, 0xEC, 0x97, 0x5E, 0xEA,
	0xA8, 0x0D, 0x74, 0x0A, 0xDB, 0xF4, 0xFF, 0x74,
	0x73, 0x59, 0xD0, 0x41, 0xD5, 0xC3, 0x3E, 0xA7,
	0x1D, 0x28, 0x1E, 0x44, 0x6B, 0x14, 0x77, 0x3B,
	0xCA, 0x97, 0xB4, 0x3A, 0x23, 0xFB, 0x80, 0x16,
	0x76, 0xBD, 0x20, 0x7A, 0x43, 0x6C, 0x64, 0x81,
	0xF1, 0xD2, 0xB9, 0x07, 0x87, 0x17, 0x46, 0x1A,
	0x5B, 0x9D, 0x32, 0xE6, 0x88, 0xF8, 0x77, 0x48,
	0x54, 0x45, 0x23, 0xB5, 0x24, 0xB0, 0xD5, 0x7D,
	0x5E, 0xA7, 0x7A, 0x27, 0x75, 0xD2, 0xEC, 0xFA,
	0x03, 0x2C, 0xFB, 0xDB, 0xF5, 0x2F, 0xB3, 0x78,
	0x61, 0x60, 0x27, 0x90, 0x04, 0xE5, 0x7A, 0xE6,
	0xAF, 0x87, 0x4E, 0x73, 0x03, 0xCE, 0x53, 0x29,
	0x9C, 0xCC, 0x04, 0x1C, 0x7B, 0xC3, 0x08, 0xD8,
	0x2A, 0x56, 0x98, 0xF3, 0xA8, 0xD0, 0xC3, 0x82,
	0x71, 0xAE, 0x35, 0xF8, 0xE9, 0xDB, 0xFB, 0xB6,
	0x94, 0xB5, 0xC8, 0x03, 0xD8, 0x9F, 0x7A, 0xE4,
	0x35, 0xDE, 0x23, 0x6D, 0x52, 0x5F, 0x54, 0x75,
	0x9B, 0x65, 0xE3, 0x72, 0xFC, 0xD6, 0x8E, 0xF2,
	0x0F, 0xA7, 0x11, 0x1F, 0x9E, 0x4A, 0xFF, 0x73
};

static char SRP_2048_G[] = { 2 };

static char *SRP_GROUP_NAMES[SRP_N_GROUPS] =
    { "SRP-768", "SRP-1024", "SRP-1280", "SRP-1536", "SRP-2048" };

void
ClearSRPContext(struct SRP_Context * p_context)
{
	/*      most fields set to 0 or NULL */
	memset(p_context, 0, sizeof (struct SRP_Context));
	p_context->format = HEX_FORMAT;
	p_context->group = SRP_1536;
	SRP_SetSRPGroup(SRP_GROUP_NAMES[p_context->group], p_context);
}

int __attribute__ ((no_instrument_function))
CloneDataUnit(struct dataunit * dst, struct dataunit * src)
{
	if ((src->length > 0) && (src->data)) {
		my_free((void **) &dst->data);
		if ((dst->data = (char *)malloc(src->length)) == NULL)
			return 0;
		memcpy(dst->data, src->data, src->length);
		dst->length = src->length;
	}
	return 1;
}

int
CalculateVerifier(struct SRP_Context * p_context)
{
	struct bigint_t N;
	struct bigint_t X;
	struct bigint_t generator;
	struct bigint_t verifier;
	int ret;

	ret = 0;
	if ((p_context == NULL) ||
	    (p_context->X.length == 0) ||
	    (p_context->N.length == 0) || (p_context->generator.length == 0))
		return 0;

	if (!bigint_init_bin(&X, p_context->X.data, p_context->X.length))
		goto out;

	if (!bigint_init_bin(&N, p_context->N.data, p_context->N.length))
		goto out;

	if (!bigint_init_bin(&generator, p_context->generator.data,
			     p_context->generator.length))
		goto out;

	if (!bigint_init(&verifier, 0))
		goto out;

	if (!bigint_mod_exp_mont(&verifier, &generator, &X, &N))
		goto out;

	p_context->verifier.length = bigint_binlen(&verifier);

	my_free((void **) &p_context->verifier.data);

	if ((p_context->verifier.data =
	     (char *) malloc(p_context->verifier.length)) == NULL)
		goto out;

	if (!bigint_tobin(&verifier, p_context->verifier.data))
		goto out;

	ret = 1;
      out:
	bigint_clean(&N);
	bigint_clean(&X);
	bigint_clean(&generator);
	bigint_clean(&verifier);
	return ret;
}

int
CalculateX(struct SRP_Context * p_context)
{
	unsigned long long len;
	unsigned int namelen;
	unsigned int secretlen;
	char *temp;
	char digest[20];

	if ((p_context == NULL) ||
	    (p_context->name == NULL) || (p_context->secret == NULL))
		return 0;

	namelen = strlen(p_context->name);
	secretlen = strlen(p_context->secret);
	len = namelen + secretlen + 1;

	if ((temp = (char *) malloc(len)) == NULL)
		return 0;

	memcpy(temp, p_context->name, namelen);
	temp[namelen] = ':';
	memcpy(temp + namelen + 1, p_context->secret, secretlen);
	SHA1_ProcessMessage(temp, (len * 8) >> 32, (int) (len * 8), digest);
	len = p_context->salt.length + 20;
	my_free((void **) &temp);
	if ((temp = (char *) malloc(len)) == NULL)
		return 0;
	p_context->X.length = 20;
	if ((p_context->X.data = (char *) malloc(20)) == NULL) {
		my_free((void **) &temp);
		return 0;
	}
	memcpy(temp, p_context->salt.data, p_context->salt.length);
	memcpy(temp + p_context->salt.length, digest, 20);
	SHA1_ProcessMessage(temp, (len * 8) >> 32, (int) (len * 8),
			    p_context->X.data);
	my_free((void **) &temp);
	return 1;
}

int
CalculateA(struct SRP_Context * p_context)
{
	struct bigint_t N;
	struct bigint_t a;
	struct bigint_t generator;
	struct bigint_t A;
	int ret;

	ret = 0;
	if ((p_context == NULL) ||
	    (p_context->N.length == 0) || (p_context->generator.length == 0))
		return 0;

	p_context->a.length = SRP_A_LENGTH;
	if ((p_context->a.data =
	     (char *) malloc(SRP_A_LENGTH)) == NULL)
		goto out;
	RandomNumberGenerate(p_context->a.data, SRP_A_LENGTH);

	if (!bigint_init_bin(&a, p_context->a.data, p_context->a.length))
		goto out;

	if (!bigint_init_bin(&N, p_context->N.data, p_context->N.length))
		goto out;

	if (!bigint_init_bin(&generator, p_context->generator.data,
			     p_context->generator.length))
		goto out;

	if (!bigint_init(&A, 0))
		goto out;

	if (!bigint_mod_exp_mont(&A, &generator, &a, &N))
		goto out;

	p_context->A.length = bigint_binlen(&A);

	my_free((void **) &p_context->A.data);

	if ((p_context->A.data = (char *) malloc(p_context->A.length)) == NULL)
		goto out;

	if (!bigint_tobin(&A, p_context->A.data))
		goto out;

	ret = 1;
      out:
	bigint_clean(&N);
	bigint_clean(&a);
	bigint_clean(&generator);
	bigint_clean(&A);
	return ret;
}

int
CalculateInitiatorS(struct SRP_Context * p_context)
{
	unsigned long long len;
	struct bigint_t N;
	struct bigint_t a;
	struct bigint_t generator;
	struct bigint_t B;
	struct bigint_t u;
	struct bigint_t X;
	struct bigint_t temp1;
	struct bigint_t temp2;
	struct bigint_t temp3;
	char digest[20];
	int ret;

	ret = 0;
	if ((p_context == NULL) ||
	    (p_context->N.length == 0) ||
	    (p_context->a.length == 0) ||
	    (p_context->B.length == 0) ||
	    (p_context->X.length == 0) || (p_context->generator.length == 0))
		return 0;

	if (!bigint_init_bin(&N, p_context->N.data, p_context->N.length))
		goto out;

	if (!bigint_init_bin(&a, p_context->a.data, p_context->a.length))
		goto out;

	if (!bigint_init_bin(&generator, p_context->generator.data,
			     p_context->generator.length))
		goto out;

	if (!bigint_init_bin(&B, p_context->B.data, p_context->B.length))
		goto out;

	if (!bigint_init_bin(&X, p_context->X.data, p_context->X.length))
		goto out;

	if (!bigint_init(&temp1, 0))
		goto out;

	if (!bigint_init(&temp2, 0))
		goto out;

	if (!bigint_init(&temp3, 0))
		goto out;

	len = p_context->B.length * 8;
	SHA1_ProcessMessage(p_context->B.data,
			    (int) (len >> 32), (int) len, digest);

	p_context->u.length = 4;
	if ((p_context->u.data = (char *) malloc(4)) == NULL)
		goto out;

	memcpy(p_context->u.data, digest, 4);

	if (!bigint_init_bin(&u, p_context->u.data, p_context->u.length))
		goto out;

	if (!bigint_mod_exp_mont(&temp2, &generator, &X, &N))
		goto out;

	if (!bigint_mod_sub(&temp1, &B, &temp2, &N))
		goto out;

	if (!bigint_mul(&temp3, &u, &X))
		goto out;

	if (!bigint_add(&temp2, &a, &temp3))
		goto out;

	if (!bigint_mod_exp_mont(&temp3, &temp1, &temp2, &N))
		goto out;

	p_context->S.length = bigint_binlen(&temp3);

	my_free((void **) &p_context->S.data);

	if ((p_context->S.data = (char *) malloc(p_context->S.length)) == NULL)
		goto out;

	if (!bigint_tobin(&temp3, p_context->S.data))
		goto out;

	ret = 1;
      out:
	bigint_clean(&N);
	bigint_clean(&a);
	bigint_clean(&generator);
	bigint_clean(&B);
	bigint_clean(&u);
	bigint_clean(&X);
	bigint_clean(&temp1);
	bigint_clean(&temp2);
	bigint_clean(&temp3);
	return ret;
}

int
CalculateK(struct SRP_Context * p_context)
{
	if ((p_context == NULL) || (p_context->S.length == 0))
		return 0;

	p_context->K.length = 40;
	if ((p_context->K.data = (char *) malloc(40)) == NULL)
		return 0;

	SHA1_Interleave(p_context->S.data,
			p_context->S.length, p_context->K.data);
	return 1;
}

int
CalculateM(struct SRP_Context * p_context)
{
	char *total;
	unsigned long long lentemp;
	unsigned int len;
	int ret;

	ret = 0;
	if ((p_context == NULL) ||
	    (p_context->B.length == 0) || (p_context->A.length == 0)) {
		printf("error : not enough data given\n");
		return 0;
	}

	if (!CalculateK(p_context))
		return 0;

	total = NULL;

	len = p_context->A.length + p_context->B.length + p_context->K.length;

	if ((total = (char *) malloc(len)) == NULL)
		goto out;

	memcpy(total, p_context->A.data, p_context->A.length);
	memcpy(total + p_context->A.length, p_context->B.data,
	       p_context->B.length);
	memcpy(total + p_context->A.length + p_context->B.length,
	       p_context->K.data, p_context->K.length);
	if ((p_context->M.data = (char *) malloc(20)) == NULL)
		goto out;

	p_context->M.length = 20;
	lentemp = len * 8;
	SHA1_ProcessMessage(total, (int) (lentemp >> 32),
			    (int) lentemp, p_context->M.data);
	ret = 1;
      out:
	my_free((void **) &total);
	return ret;
}

int
CalculateB(struct SRP_Context * p_context)
{
	struct bigint_t N;
	struct bigint_t b;
	struct bigint_t generator;
	struct bigint_t B;
	struct bigint_t temp;
	struct bigint_t verifier;
	int ret;

	ret = 0;

	if ((p_context == NULL) ||
	    (p_context->N.length == 0) || (p_context->generator.length == 0))
		return 0;

	if (!CalculateX(p_context))
		return 0;

	if (!CalculateVerifier(p_context))
		return 0;

	p_context->b.length = SRP_B_LENGTH;
	if ((p_context->b.data =
	     (char *) malloc(SRP_B_LENGTH)) == NULL)
		goto out;

	RandomNumberGenerate(p_context->b.data, SRP_B_LENGTH);

	if (!bigint_init_bin(&b, p_context->b.data, p_context->b.length))
		goto out;

	if (!bigint_init_bin(&N, p_context->N.data, p_context->N.length))
		goto out;

	if (!bigint_init_bin(&generator, p_context->generator.data,
			     p_context->generator.length))
		goto out;

	if (!bigint_init_bin(&verifier, p_context->verifier.data,
			     p_context->verifier.length))
		goto out;

	if (!bigint_init(&temp, 0))
		goto out;

	if (!bigint_init(&B, 0))
		goto out;

	if (!bigint_mod_exp_mont(&temp, &generator, &b, &N))
		goto out;

	if (!bigint_mod_add(&B, &temp, &verifier, &N))
		goto out;

	p_context->B.length = bigint_binlen(&B);

	my_free((void **) &p_context->B.data);

	if ((p_context->B.data = (char *) malloc(p_context->B.length)) == NULL)
		goto out;

	if (!bigint_tobin(&B, p_context->B.data))
		goto out;

	ret = 1;

      out:
	bigint_clean(&N);
	bigint_clean(&generator);
	bigint_clean(&temp);
	bigint_clean(&B);
	bigint_clean(&verifier);
	bigint_clean(&b);

	return ret;
}

int
CalculateTargetS(struct SRP_Context * p_context)
{
	unsigned long long len;
	struct bigint_t N;
	struct bigint_t b;
	struct bigint_t generator;
	struct bigint_t A;
	struct bigint_t u;
	struct bigint_t verifier;
	struct bigint_t temp1;
	struct bigint_t temp2;
	struct bigint_t temp3;
	char digest[20];
	int ret;

	ret = 0;
	if ((p_context == NULL) ||
	    (p_context->N.length == 0) ||
	    (p_context->b.length == 0) ||
	    (p_context->A.length == 0) ||
	    (p_context->B.length == 0) ||
	    (p_context->verifier.length == 0) ||
	    (p_context->generator.length == 0))
		return 0;

	len = p_context->B.length * 8;
	SHA1_ProcessMessage(p_context->B.data,
			    (int) (len >> 32), (int) len, digest);
	p_context->u.length = 4;

	if ((p_context->u.data = (char *) malloc(4)) == NULL)
		goto out;
	memcpy(p_context->u.data, digest, 4);

	if (!bigint_init_bin(&N, p_context->N.data, p_context->N.length))
		goto out;

	if (!bigint_init_bin(&b, p_context->b.data, p_context->b.length))
		goto out;

	if (!bigint_init_bin(&generator, p_context->generator.data,
			     p_context->generator.length))
		goto out;

	if (!bigint_init_bin(&A, p_context->A.data, p_context->A.length))
		goto out;

	if (!bigint_init_bin(&u, p_context->u.data, p_context->u.length))
		goto out;

	if (!bigint_init_bin(&verifier, p_context->verifier.data,
			     p_context->verifier.length))
		goto out;

	if (!bigint_init(&temp1, 0))
		goto out;

	if (!bigint_init(&temp2, 0))
		goto out;

	if (!bigint_init(&temp3, 0))
		goto out;

	if (!bigint_mod_exp_mont(&temp1, &verifier, &u, &N))
		goto out;

	if (!bigint_mul(&temp2, &A, &temp1))
		goto out;

	if (!bigint_mod_exp_mont(&temp3, &temp2, &b, &N))
		goto out;

	p_context->S.length = bigint_binlen(&temp3);

	my_free((void **) &p_context->S.data);

	if ((p_context->S.data = (char *) malloc(p_context->S.length)) == NULL)
		goto out;

	if (!bigint_tobin(&temp3, p_context->S.data))
		goto out;

	ret = 1;
      out:
	bigint_clean(&N);
	bigint_clean(&generator);
	bigint_clean(&A);
	bigint_clean(&u);
	bigint_clean(&verifier);
	bigint_clean(&b);
	bigint_clean(&temp1);
	bigint_clean(&temp2);
	bigint_clean(&temp3);

	return ret;
}

int
CalculateHM(struct SRP_Context * p_context)
{
	int len;
	unsigned long long lentemp;
	char *temp;

	if ((p_context == NULL) ||
	    (p_context->A.length == 0) ||
	    (p_context->M.length == 0) || (p_context->K.length == 0))
		return 0;

	len = p_context->A.length + p_context->M.length + p_context->K.length;
	if ((temp = (char *) malloc(len)) == NULL)
		return 0;

	memcpy(temp, p_context->A.data, p_context->A.length);
	memcpy(temp + p_context->A.length,
	       p_context->M.data, p_context->M.length);
	memcpy(temp + p_context->A.length
	       + p_context->M.length, p_context->K.data, p_context->K.length);
	if ((p_context->HM.data = (char *) malloc(20)) == NULL) {
		my_free((void **) &temp);
		return 0;
	}

	p_context->HM.length = 20;
	lentemp = len * 8;
	SHA1_ProcessMessage(temp, (int) (lentemp >> 32),
			    (int) lentemp, p_context->HM.data);
	my_free((void **) &temp);
	return 1;
}

// common functions

struct 
SRP_Context *
SRP_InitializeContext(void)
{
	struct SRP_Context *context;

	if ((context = malloc(sizeof (struct SRP_Context))) == NULL)
		return NULL;
	ClearSRPContext(context);
	return context;
}

void
SRP_FinalizeContext(struct SRP_Context * p_context)
{
	if (!p_context)
		return;

	my_free((void **) &p_context->name);

	my_free((void **) &p_context->secret);

	my_free((void **) &p_context->salt.data);

	my_free((void **) &p_context->verifier.data);

	my_free((void **) &p_context->S.data);

	my_free((void **) &p_context->a.data);

	my_free((void **) &p_context->A.data);

	my_free((void **) &p_context->b.data);

	my_free((void **) &p_context->B.data);

	my_free((void **) &p_context->X.data);

	my_free((void **) &p_context->u.data);

	my_free((void **) &p_context->K.data);

	my_free((void **) &p_context->M.data);

	my_free((void **) &p_context->HM.data);

	my_free((void **) &p_context->N.data);

	my_free((void **) &p_context->generator.data);

	my_free((void **) &p_context);
}

struct SRP_Context *
SRP_CloneContext(struct SRP_Context * p_context)
{
	struct SRP_Context *context;

	if (!p_context)
		return NULL;

	if ((context = SRP_InitializeContext()) == NULL)
		return NULL;

	context->format = p_context->format;
	context->group = p_context->group;

	if (p_context->name) {
		if ((context->name = malloc(strlen(p_context->name) + 1)) == NULL)
			goto out;
		strcpy(context->name, p_context->name);
	}

	if (p_context->secret) {
		if ((context->secret = malloc(strlen(p_context->secret + 1))) == NULL)
			goto out;
		strcpy(context->secret, p_context->secret);
	}

	if (!CloneDataUnit(&context->salt, &p_context->salt))
		goto out;

	if (!CloneDataUnit(&context->verifier, &p_context->verifier))
		goto out;

	if (!CloneDataUnit(&context->S, &p_context->S))
		goto out;

	if (!CloneDataUnit(&context->a, &p_context->a))
		goto out;

	if (!CloneDataUnit(&context->A, &p_context->A))
		goto out;

	if (!CloneDataUnit(&context->b, &p_context->b))
		goto out;

	if (!CloneDataUnit(&context->B, &p_context->B))
		goto out;

	if (!CloneDataUnit(&context->X, &p_context->X))
		goto out;

	if (!CloneDataUnit(&context->u, &p_context->u))
		goto out;

	if (!CloneDataUnit(&context->K, &p_context->K))
		goto out;

	if (!CloneDataUnit(&context->M, &p_context->M))
		goto out;

	if (!CloneDataUnit(&context->HM, &p_context->HM))
		goto out;

	if (!CloneDataUnit(&context->N, &p_context->N))
		goto out;

	if (!CloneDataUnit(&context->generator, &p_context->generator))
		goto out;

	return context;
      out:
	SRP_FinalizeContext(context);
	return NULL;
}

void
SRP_PrintContext(struct SRP_Context * p_context)
{
	printf
	    ("\n******************************************************************\n");
	printf("format\t: ");
	if (p_context->format == HEX_FORMAT)
		printf("HEX\n");
	else if (p_context->format == BASE64_FORMAT)
		printf("BASE64\n");
	else
		printf("Unknown\n");
	printf("name\t: %s\n", p_context->name);
	printf("secret\t: %s\n", p_context->secret);
	printf("salt\t\t: ");
	PrintDataUnit(&p_context->salt);
	printf("verifier\t: ");
	PrintDataUnit(&p_context->verifier);
	printf("S\t\t: ");
	PrintDataUnit(&p_context->S);
	printf("a\t\t: ");
	PrintDataUnit(&p_context->a);
	printf("A\t\t: ");
	PrintDataUnit(&p_context->A);
	printf("b\t\t: ");
	PrintDataUnit(&p_context->b);
	printf("B\t\t: ");
	PrintDataUnit(&p_context->B);
	printf("X\t\t: ");
	PrintDataUnit(&p_context->X);
	printf("u\t\t: ");
	PrintDataUnit(&p_context->u);
	printf("K\t\t: ");
	PrintDataUnit(&p_context->K);
	printf("M\t\t: ");
	PrintDataUnit(&p_context->M);
	printf("HM\t\t: ");
	PrintDataUnit(&p_context->HM);
	printf("N\t\t: ");
	PrintDataUnit(&p_context->N);
	printf("Generator\t: ");
	PrintDataUnit(&p_context->generator);
	printf
	    ("******************************************************************\n");
}

int
SRP_SetName(char *p_username, struct SRP_Context * p_context)
{
	int len;

	if ((p_username == NULL) || (p_context == NULL))
		return 0;

	my_free((void **) &p_context->name);
	len = strlen(p_username);
	if ((p_context->name =
	     (char *) malloc(len + 1)) == NULL)
		return 0;
	strcpy(p_context->name, p_username);

	return 1;
}

int
SRP_SetSecret(char *p_secret, struct SRP_Context * p_context)
{
	int len;

	if ((p_secret == NULL) || (p_context == NULL))
		return 0;

	my_free((void **) &p_context->secret);

	len = strlen(p_secret);
	if ((p_context->secret =
	     (char *) malloc(len + 1)) == NULL)
		return 0;
	strcpy(p_context->secret, p_secret);

	return 1;
}

int
SRP_SetNumberFormat(int p_format, struct SRP_Context * p_context)
{
	if ((p_context->format == HEX_FORMAT) ||
	    (p_context->format == BASE64_FORMAT)) {
		p_context->format = p_format;
		return 1;
	} else
		return 0;
}

int
SRP_SetSRPGroup(char *p_group, struct SRP_Context * p_context)
{
	char *N;
	char *G;
	int N_len;
	int G_len;
	int i;
	int group;

	group = -1;

	if ((p_group == NULL) || (p_context == NULL))
		return 0;

	for (i = SRP_768; i <= SRP_2048; i++) {
		if (!strcmp(SRP_GROUP_NAMES[i], p_group)) {
			group = i;
			break;
		}
	}

	switch (group) {
	case SRP_768:
		N = SRP_768_N;
		G = SRP_768_G;
		N_len = SRP_768_N_LEN;
		G_len = SRP_768_G_LEN;
		break;
	case SRP_1024:
		N = SRP_1024_N;
		G = SRP_1024_G;
		N_len = SRP_1024_N_LEN;
		G_len = SRP_1024_G_LEN;
		break;
	case SRP_1280:
		N = SRP_1280_N;
		G = SRP_1280_G;
		N_len = SRP_1280_N_LEN;
		G_len = SRP_1280_G_LEN;
		break;
	case SRP_1536:
		N = SRP_1536_N;
		G = SRP_1536_G;
		N_len = SRP_1536_N_LEN;
		G_len = SRP_1536_G_LEN;
		break;
	case SRP_2048:
		N = SRP_2048_N;
		G = SRP_2048_G;
		N_len = SRP_2048_N_LEN;
		G_len = SRP_2048_G_LEN;
		break;
	default:
		return 0;
	}

	my_free((void **) &p_context->N.data);
	if ((p_context->N.data = (char *)
	     malloc(N_len)) == NULL)
		return 0;
	memcpy(p_context->N.data, N, N_len);
	p_context->N.length = N_len;

	my_free((void **) &p_context->generator.data);
	if ((p_context->generator.data = (char *)
	     malloc(G_len)) == NULL)
		return 0;
	memcpy(p_context->generator.data, G, G_len);
	p_context->generator.length = G_len;

	p_context->group = group;
	return 1;
}

// target functions

int
SRP_Target_SetA(char *p_A, int max_length, struct SRP_Context * p_context)
{
	int len;

	if ((p_A == NULL) || (p_context == NULL))
		return 0;

	len = StringToIntegerLength(p_A);
	if (len > max_length) {
		TRACE_ERROR("SRP_A binary length is %d, limit is %d\n", len,
			    max_length);
		return 0;
	}

	if ((p_context->A.data =
	     (char *) malloc(len)) == NULL)
		return 0;

	StringToInteger(p_A, p_context->A.data);
	p_context->A.length = len;
	return 1;
}

int
SRP_Target_SetM(char *p_M, int max_length, struct SRP_Context * p_context)
{
	int len;
	char *temp;
	int ret;

	if ((p_M == NULL) || (p_context == NULL))
		return 0;

	if (!CalculateTargetS(p_context))
		return 0;

	if (!CalculateM(p_context))
		return 0;

	len = StringToIntegerLength(p_M);
	if (len > max_length) {
		TRACE_ERROR("SRP_M binary length is %d, limit is %d\n", len,
			    max_length);
		return 0;
	}

	if ((temp = (char *) malloc(len)) == NULL)
		return 0;

	StringToInteger(p_M, temp);
	ret = IntegerCompare(p_context->M.data, p_context->M.length, temp, len);
	my_free((void **) &temp);
	return ret;
}

char *
SRP_Target_GetGroupList(struct SRP_Context * p_context)
{
	char *data;

	if ((data = malloc(44)) == NULL)
		return NULL;

	sprintf(data, "%s,%s,%s,%s,%s",
		SRP_GROUP_NAMES[SRP_768],
		SRP_GROUP_NAMES[SRP_1024],
		SRP_GROUP_NAMES[SRP_1280],
		SRP_GROUP_NAMES[SRP_1536], SRP_GROUP_NAMES[SRP_2048]);
	return data;
}

/*	returns index number of group_name, -1 if not found */
int
SRP_GetGroupIndex(char *group_name, struct SRP_Context * p_context)
{
	int i;

	for (i = 0; i < SRP_N_GROUPS; i++) {
		if (!strcmp(SRP_GROUP_NAMES[i], group_name))
			return i;
	}
	return -1;
}

char *
SRP_Target_GetSalt(struct SRP_Context * p_context)
{
	int len;
	char *temp;

	if (p_context == NULL)
		return NULL;

	if ((p_context->salt.data =
	     (char *) malloc(SRP_SALT_LENGTH)) == NULL)
		return NULL;

	RandomNumberGenerate(p_context->salt.data, SRP_SALT_LENGTH);
	p_context->salt.length = SRP_SALT_LENGTH;

	len = IntegerToStringLength(p_context->salt.length, p_context->format);
	if ((temp = (char *) malloc(len)) == NULL)
		return NULL;
	IntegerToString(p_context->salt.data, p_context->salt.length, temp,
			p_context->format);
	return temp;
}

char *
SRP_Target_GetB(struct SRP_Context * p_context)
{
	int len;
	char *data;

	if (p_context == NULL)
		return NULL;

	if (!CalculateB(p_context))
		return NULL;

	if (p_context->B.length == 0)
		return NULL;

	len = IntegerToStringLength(p_context->B.length, p_context->format);
	if ((data = (char *) malloc(len)) == NULL)
		return NULL;
	IntegerToString(p_context->B.data,
			p_context->B.length, data, p_context->format);
	return data;
}

char *
SRP_Target_GetHM(struct SRP_Context * p_context)
{
	int len;
	char *data;
	if (p_context == NULL)
		return NULL;

	if (!CalculateHM(p_context))
		return NULL;

	if (p_context->HM.length == 0)
		return NULL;

	len = IntegerToStringLength(p_context->HM.length, p_context->format);
	if ((data = (char *) malloc(len)) == NULL)
		return NULL;
	IntegerToString(p_context->HM.data,
			p_context->HM.length, data, p_context->format);
	return data;
}

// initiator functions

int
SRP_Initiator_SetSalt(char *p_salt, int max_length, struct SRP_Context * p_context)
{
	int len;

	if ((p_salt == NULL) && (p_context == NULL))
		return 0;

	len = StringToIntegerLength(p_salt);
	if (len > max_length) {
		TRACE_ERROR("SRP_s binary length is %d, limit is %d\n", len,
			    max_length);
		return 0;
	}

	if ((p_context->salt.data = (char *) malloc(len)) == NULL)
		return 0;

	StringToInteger(p_salt, p_context->salt.data);
	p_context->salt.length = len;
	return 1;
}

int
SRP_Initiator_SetB(char *p_B, int max_length, struct SRP_Context * p_context)
{
	int len;

	if ((p_B == NULL) || (p_context == NULL))
		return 0;

	len = StringToIntegerLength(p_B);
	if (len > max_length) {
		TRACE_ERROR("SRP_B binary length is %d, limit is %d\n", len,
			    max_length);
		return 0;
	}

	if ((p_context->B.data = (char *) malloc(len)) == NULL)
		return 0;
	StringToInteger(p_B, p_context->B.data);
	p_context->B.length = len;
	return 1;
}

int
SRP_Initiator_SetHM(char *p_HM, int max_length, struct SRP_Context * p_context)
{
	int len;
	char *temp;
	int ret;

	if ((p_HM == NULL) || (p_context == NULL))
		return 0;

	if (!CalculateHM(p_context))
		return 0;

	len = StringToIntegerLength(p_HM);
	if (len > max_length) {
		TRACE_ERROR("SRP_HM binary length is %d, limit is %d\n", len,
			    max_length);
		return 0;
	}

	if ((temp = (char *) malloc(len)) == NULL)
		return 0;

	StringToInteger(p_HM, temp);
	ret = IntegerCompare(p_context->HM.data,
			     p_context->HM.length, temp, len);
	my_free((void **) &temp);
	return ret;
}

char *
SRP_Initiator_GetUsername(struct SRP_Context * p_context)
{
	int len;
	char *data;

	if ((!p_context) || (!p_context->name))
		return NULL;
	len = strlen(p_context->name);
	if ((data = (char *) malloc(len + 1)) == NULL)
		return NULL;
	strcpy(data, p_context->name);
	return data;
}

char *
SRP_Initiator_GetGroup(char *p_groups[], struct SRP_Context * p_context)
{
	int i;
	char *data;

	if ((!p_groups) || (!p_context))
		return NULL;

	if ((p_context->group < 0) && (p_context->group >= SRP_N_GROUPS))
		return NULL;

	/* check to make sure SRP_1536 was offered */
	i = 0;
	while (p_groups[i] != NULL) {
		if (!strcmp(p_groups[i], SRP_GROUP_NAMES[SRP_1536]))
			break;;
		i++;
	}
	if (p_groups[i] == NULL) {
		TRACE(TRACE_ISCSI,
		      "Warning, %s not offered in SRP_GROUP list\n",
		      SRP_GROUP_NAMES[SRP_1536]);
	}

	/* now go back and find the prefered choice */
	i = 0;
	while (p_groups[i] != NULL) {
		if (!strcmp(p_groups[i], SRP_GROUP_NAMES[p_context->group]))
			break;;
		i++;
	}
	if (p_groups[i] != NULL) {
		if ((data = (char *) malloc(strlen(p_groups[i]) + 1)) == NULL)
			return NULL;
		strcpy(data, p_groups[i]);
		return data;
	} else
		return NULL;
}

char *
SRP_Initiator_GetA(struct SRP_Context * p_context)
{
	int len;
	char *data;

	if (p_context == NULL)
		return NULL;

	if (!CalculateA(p_context))
		return NULL;

	if (p_context->A.length == 0)
		return NULL;

	len = IntegerToStringLength(p_context->A.length, p_context->format);
	if ((data = (char *) malloc(len)) == NULL)
		return NULL;

	IntegerToString(p_context->A.data,
			p_context->A.length, data, p_context->format);
	return data;
}

char *
SRP_Initiator_GetM(struct SRP_Context * p_context)
{
	int len;
	char *data;

	if (p_context == NULL)
		return NULL;

	if (!CalculateX(p_context))
		return 0;

	if (!CalculateInitiatorS(p_context))
		return 0;

	if (!CalculateM(p_context))
		return NULL;

	if (p_context->M.length == 0)
		return NULL;

	len = IntegerToStringLength(p_context->M.length, p_context->format);
	if ((data = (char *) malloc(len)) == NULL)
		return NULL;

	IntegerToString(p_context->M.data,
			p_context->M.length, data, p_context->format);
	return data;
}
