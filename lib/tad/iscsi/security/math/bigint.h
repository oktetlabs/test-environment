/*	security/math/bigint.h

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

#ifndef BIGINT_H
#define BIGINT_H

struct bigint_t {
	unsigned int *data;
	unsigned int offset;
	unsigned int size;
	unsigned int flags;
} ;

#define BIGINT_DEBUG

#define BIGINT_WINDOW_SIZE		0x08
#define BIGINT_TABLE_SIZE		(1 << BIGINT_WINDOW_SIZE)
#define BIGINT_FLAGS_NONE		0x00000000
#define BIGINT_FLAGS_NEGATIVE	0x00000001
#define BIGINT_FLAGS_VALID		0x10000000

#define BIGINT_INIT_DATALEN		1

#define IS_VALID(x)				((x)->flags & BIGINT_FLAGS_VALID)
#define IS_NEGATIVE(x)			((x)->flags & BIGINT_FLAGS_NEGATIVE)
#define IS_ZERO(x)				((x)->offset == 0)
#define IS_ONE(x)				((x)->offset == 1) && ((x)->data[0] == 1)

#define SET_VALID(x)			(x)->flags |= BIGINT_FLAGS_VALID
#define SET_INVALID(x)			(x)->flags &= ~BIGINT_FLAGS_VALID
#define SET_NEGATIVE(x)			(x)->flags |= BIGINT_FLAGS_NEGATIVE
#define SET_POSITIVE(x)			(x)->flags &= ~BIGINT_FLAGS_NEGATIVE
#define SET_ZERO(x)				(x)->offset = 0
#define SET_ONE(x)				(x)->offset = 1; (x)->data[0] = 1

/* mew functions */
int bigint_init(struct bigint_t *n, unsigned int len);

int bigint_clean(struct bigint_t *n);

/* basic utility functions */
struct bigint_t *bigint_new(unsigned int size);

void bigint_free(struct bigint_t *n);

void bigint_print(const struct bigint_t *n);

int bigint_check(const struct bigint_t *n);

int bigint_clear(struct bigint_t *n);

int bigint_trim(struct bigint_t *n);

int bigint_extend(struct bigint_t *n, unsigned int size);

int bigint_cpy(struct bigint_t *a, const struct bigint_t *b);

unsigned int bigint_bits(const struct bigint_t *a);

unsigned int bigint_bytes(const struct bigint_t *a);

#ifdef BIGINT_DEBUG
void bigint_checkmemory(void);
#endif

/* basic big integer operation functions */
int bigint_cmp(const struct bigint_t *a, const struct bigint_t *b);

int bigint_add(struct bigint_t *r,
	       const struct bigint_t *a, const struct bigint_t *b);

int bigint_sub(struct bigint_t *r,
	       const struct bigint_t *a, const struct bigint_t *b);

int bigint_mul(struct bigint_t *r,
	       const struct bigint_t *a, const struct bigint_t *b);

int bigint_fix(struct bigint_t *a, const struct bigint_t *n);

int bigint_lsh(struct bigint_t *r, const struct bigint_t *a, unsigned int n);

int bigint_rsh(struct bigint_t *r, const struct bigint_t *a, unsigned int n);

int bigint_xor(struct bigint_t *r,
	       const struct bigint_t *a, const struct bigint_t *b);

int bigint_mod(struct bigint_t *r,
	       const struct bigint_t *a, const struct bigint_t *b);

int bigint_div(struct bigint_t *r, const struct bigint_t *a,
	       const struct bigint_t *b);

/* exported big integer operation functions */
int bigint_init_uint(struct bigint_t *r, unsigned int n);

int bigint_init_bin(struct bigint_t *r, const unsigned char *n,
		    unsigned int len);

struct bigint_t *bigint_new_uint(const unsigned int n);

struct bigint_t *bigint_new_bin(const unsigned char *n, unsigned int len);

int bigint_binlen(const struct bigint_t *n);

int bigint_tobin(const struct bigint_t *n, char *bin);

int bigint_mod_add(struct bigint_t *r,
		   const struct bigint_t *a,
		   const struct bigint_t *b, const struct bigint_t *m);

int bigint_mod_sub(struct bigint_t *r,
		   const struct bigint_t *a,
		   const struct bigint_t *b, const struct bigint_t *m);

int bigint_mod_mul(struct bigint_t *r,
		   const struct bigint_t *a,
		   const struct bigint_t *b, const struct bigint_t *m);

int bigint_mod_exp(struct bigint_t *r,
		   const struct bigint_t *a,
		   const struct bigint_t *b, const struct bigint_t *m);

int bigint_mod_exp_mont(struct bigint_t *r, const struct bigint_t *a,
			const struct bigint_t *b, const struct bigint_t *m);
int bigint_mod_mul_mont(struct bigint_t *r, const struct bigint_t *a,
			  const struct bigint_t *b, const struct bigint_t *m);
int bigint_eea_mont(struct bigint_t *np, const struct bigint_t *n,
		      const unsigned int k);
int bigint_round_mont(unsigned int *r, const struct bigint_t *n);
int bigint_div_mont(struct bigint_t *r, const struct bigint_t *a,
		      const unsigned int b);
int bigint_mod_mont(struct bigint_t *r, const struct bigint_t *a,
		      const unsigned int b);

int bigint_product_mont(struct bigint_t *x,
			const struct bigint_t *a,
			const struct bigint_t *b,
			const struct bigint_t *np,
			const struct bigint_t *n, const unsigned int r);

#endif
