/*	security/math/bigint.c

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
#include <stdio.h>
#include <stddef.h>

#include <my_memory.h>

#include "bigint.h"

static unsigned int bigint_bitmaps[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000
};

#ifdef BIGINT_DEBUG
static unsigned long memory_used = 0;
#endif

int
bigint_init(struct bigint_t *n, unsigned len)
{
#ifdef BIGINT_DEBUG
	if (n == NULL)
		return 0;
#endif
	memset(n, 0, sizeof (struct bigint_t));
	if (!len)
		len = BIGINT_INIT_DATALEN;
	if ((n->data =
	     (unsigned int *) malloc(len * sizeof (unsigned int))) == NULL)
		return 0;
	n->size = len;
	memset(n->data, 0, n->size * sizeof (unsigned int));
#ifdef BIGINT_DEBUG
	memory_used += n->size * sizeof (unsigned int);
#endif
	SET_VALID(n);
	return 1;
}

int
bigint_clean(struct bigint_t *n)
{
#ifdef BIGINT_DEBUG
	if (!bigint_check(n))
		return 0;
#endif
	if (IS_VALID(n) && (n->data))
		ZFREE(n->data);
#ifdef BIGITN_DEBUG
	memory_used -= n->size * sizeof (unsigned int);
#endif
	memset(n, 0, sizeof (struct bigint_t));
	return 1;
}

struct bigint_t *
bigint_new(unsigned int size)
{
	struct bigint_t *r;
	if ((r =
	     (struct bigint_t *) malloc(sizeof (struct bigint_t))) == NULL)
		return NULL;
	r->flags = 0;
	r->offset = 0;
	r->data = NULL;
	if (size == 0)
		size++;
	if ((r->data =
	     (unsigned int *) malloc(size * sizeof (unsigned int))) == NULL) {
		ZFREE(r);
		return NULL;
	}
	r->size = size;
#ifdef BIGINT_DEBUG
	memory_used += sizeof (struct bigint_t);
	memory_used += r->size * (sizeof (unsigned int));
#endif
	SET_VALID(r);
	return r;
}

void
bigint_free(struct bigint_t *n)
{
	if (n != NULL) {
		if (n->data != NULL) {
			free(n->data);
#ifdef BIGINT_DEBUG
			memory_used -= n->size * (sizeof (unsigned int));
#endif
			free(n);
		}
#ifdef BIGINT_DEBUG
		memory_used -= sizeof (struct bigint_t);
#endif
	}
}

void
bigint_print(const struct bigint_t *n)
{
	int i;

	printf("\n***********big integer***********\n");
	printf("size: %d\n", n->size);
	printf("offset: %d\n", n->offset);
	if (IS_NEGATIVE(n))
		printf("negative\n");
	printf("data:\n");
	for (i = n->offset; i > 0; i--)
		printf("%08x\n", n->data[i - 1]);
	printf("****************end***************\n");
}

#ifdef BIGINT_DEBUG
void
bigint_checkmemory()
{
	printf("\n***********big integer***********\n");
	printf("memory used: %lu byte(s)\n", memory_used);
	printf("****************end***************\n");
}
#endif

int
bigint_check(const struct bigint_t *n)
{
	if (n == NULL)
		return 0;
	if ((n->data == NULL) && (n->offset > 0))
		return 0;
	if (n->offset > n->size)
		return 0;
	return 1;
}

int
bigint_clear(struct bigint_t *n)
{
	n->flags = 0;
	n->offset = 0;
	if (n->size)
		memset(n->data, 0, n->size * sizeof (unsigned int));
	return 1;
}

int
bigint_trim(struct bigint_t *n)
{
	while (n->offset && (n->data[n->offset - 1] == 0))
		n->offset--;
	return 1;
}

int
bigint_extend(struct bigint_t *n, unsigned int size)
{
	unsigned int *temp;

	if (size <= n->size)
		return 1;
	if ((temp =
	     (unsigned int *) malloc(size * sizeof (unsigned int))) == NULL)
		return 0;
#ifdef BIGINT_DEBUG
	memory_used += (size - n->size) * sizeof (unsigned int);
#endif
	memset(temp, 0, sizeof (unsigned int) * size);
	if (n->data) {
		memcpy(temp, n->data, sizeof (unsigned int) * n->offset);
		ZFREE(n->data);
	}
	n->data = temp;
	n->size = size;
	return 1;
}

int
bigint_cpy(struct bigint_t *a, const struct bigint_t *b)
{
	if (a == b)
		return 1;
	a->flags = b->flags;
	if (a->offset < b->offset) {
		if (!bigint_extend(a, b->offset))
			return 0;
	}
	if (b->offset > 0)
		memcpy(a->data, b->data, b->offset * sizeof (unsigned int));
	a->offset = b->offset;
	return 1;
}

unsigned int
bigint_bits(const struct bigint_t *a)
{
	int nw;
	unsigned int d;
	int i;

	nw = 0;
	if (a->offset) {
		nw = (a->offset - 1) * sizeof (unsigned int) * 8;
		d = a->data[a->offset - 1];
		for (i = 32; i > 0; i--) {
			if (bigint_bitmaps[i - 1] & d) {
				nw += i;
				break;
			}
		}
	}
	return nw;
}

unsigned int
bigint_bytes(const struct bigint_t *a)
{
	int nw;
	unsigned int d;

	nw = 0;
	if (a->offset) {
		nw = (a->offset - 1) * sizeof (unsigned int);
		d = a->data[a->offset - 1];
		if (d & 0xFF000000)
			nw += 4;
		else if (d & 0x00FF0000)
			nw += 3;
		else if (d & 0x0000FF00)
			nw += 2;
		else if (d & 0x000000FF)
			nw += 1;
	}
	return nw;
}

int
bigint_cmp(const struct bigint_t *a, const struct bigint_t *b)
{
	int i;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(a)) || (!bigint_check(b)))
		return 0;
#endif

	if (a->offset > b->offset)
		return 1;
	if (a->offset < b->offset)
		return -1;
	for (i = a->offset; i > 0; i--) {
		if (a->data[i - 1] > b->data[i - 1])
			return 1;
		if (a->data[i - 1] < b->data[i - 1])
			return -1;
	}
	return 0;
}

int
bigint_add_inner(struct bigint_t *r,
		 const struct bigint_t *a, const struct bigint_t *b)
{
	const struct bigint_t *tmp;
	struct bigint_t ret;
	unsigned int max;
	unsigned int min;
	unsigned int *ap;
	unsigned int *bp;
	unsigned int *rp;
	unsigned long long carry = 0;	//should be long long in linux
	unsigned int i;

	if (b->offset > a->offset) {
		tmp = a;
		a = b;
		b = tmp;
	}
	max = a->offset;
	min = b->offset;
	if (!bigint_init(&ret, max + 1))
		return 0;
	ap = a->data;
	bp = b->data;
	rp = ret.data;
	for (i = 0; i < min; i++) {
		carry += (unsigned long long) (ap[i]) + (unsigned long long) (bp[i]);	//should be long long in linux
		rp[i] = (unsigned int) (carry & 0xFFFFFFFF);
		carry >>= 32;
	}
	while (carry && (i < max)) {
		carry += (unsigned long long) ap[i];	//should be long long in linux
		rp[i] = (unsigned int) (carry & 0xFFFFFFFF);
		carry >>= 32;
		i++;
	}
	if (!carry) {
		for (; i < max; i++)
			rp[i] = ap[i];
	} else
		rp[i] = (unsigned int) carry;
	ret.offset = max + 1;
	bigint_trim(&ret);
	if (!bigint_cpy(r, &ret)) {
		bigint_clean(&ret);
		return 0;
	}
	bigint_clean(&ret);
	return 1;
}

int
bigint_sub_inner(struct bigint_t *r,
		 const struct bigint_t *a, const struct bigint_t *b)
{
	const struct bigint_t *tmp;
	struct bigint_t ret;
	unsigned int max;
	unsigned int min;
	unsigned int *ap;
	unsigned int *bp;
	unsigned int *rp;
	unsigned int carry = 0;
	unsigned int i;
	int neg;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)) || (!bigint_check(b)))
		return 0;
#endif

	neg = 0;
	if (bigint_cmp(a, b) < 0) {
		neg = 1;
		tmp = a;
		a = b;
		b = tmp;
	}
	max = a->offset;
	min = b->offset;
	if (!bigint_init(&ret, max))
		return 0;
	if (neg)
		SET_NEGATIVE(&ret);
	ap = a->data;
	bp = b->data;
	rp = ret.data;
	for (i = 0; i < min; i++) {
		if (carry) {
			carry = (ap[i] <= bp[i]);
			rp[i] = ap[i] - bp[i] - 1;
		} else {
			carry = (ap[i] < bp[i]);
			rp[i] = ap[i] - bp[i];
		}
	}
	while (carry && (i < max)) {
		rp[i] = ap[i] - 1;
		carry = (!ap[0]);
		i++;
	}
	if (!carry) {
		for (; i < max; i++)
			rp[i] = ap[i];
	}
	ret.offset = max;
	bigint_trim(&ret);
	if (!bigint_cpy(r, &ret)) {
		bigint_clean(&ret);
		return 0;
	}
	bigint_clean(&ret);
	return 1;
}

int
bigint_add(struct bigint_t *r,
	   const struct bigint_t *a, const struct bigint_t *b)
{
#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)) || (!bigint_check(b)))
		return 0;
#endif

	if (IS_NEGATIVE(a) && IS_NEGATIVE(b)) {
		if (bigint_add_inner(r, a, b)) {
			SET_NEGATIVE(r);
			return 1;
		} else
			return 0;
	}
	if (!IS_NEGATIVE(a) && IS_NEGATIVE(b))
		return bigint_sub_inner(r, a, b);
	if (IS_NEGATIVE(a) && !IS_NEGATIVE(b))
		return bigint_sub_inner(r, b, a);
	if (!IS_NEGATIVE(a) && !IS_NEGATIVE(b))
		return bigint_add_inner(r, a, b);
	return 0;
}

int
bigint_sub(struct bigint_t *r,
	   const struct bigint_t *a, const struct bigint_t *b)
{
#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)) || (!bigint_check(b)))
		return 0;
#endif

	if (IS_NEGATIVE(a) && IS_NEGATIVE(b)) {
		if (bigint_sub_inner(r, a, b)) {
			if (IS_NEGATIVE(r))
				SET_POSITIVE(r);
			else
				SET_NEGATIVE(r);
			return 1;
		} else
			return 0;
	}
	if (!IS_NEGATIVE(a) && IS_NEGATIVE(b))
		return bigint_add_inner(r, a, b);
	if (IS_NEGATIVE(a) && !IS_NEGATIVE(b)) {
		if (bigint_add_inner(r, b, a)) {
			SET_NEGATIVE(r);
			return 1;
		} else
			return 0;
	}
	if (!IS_NEGATIVE(a) && !IS_NEGATIVE(b))
		return bigint_sub_inner(r, a, b);
	return 0;
}

int
bigint_mul(struct bigint_t *r,
	   const struct bigint_t *a, const struct bigint_t *b)
{
	const struct bigint_t *tmp;
	struct bigint_t ret;
	unsigned int al;
	unsigned int bl;
	unsigned int rl;
	unsigned int *rp;
	unsigned int *ap;
	unsigned int *bp;
	unsigned int *cp;
	unsigned int i;
	unsigned int j;
	unsigned long long carry;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)) || (!bigint_check(b)))
		return 0;
#endif

	if (IS_ZERO(a) || IS_ZERO(b)) {
		SET_ZERO(r);
		return 1;
	}
	if (IS_ONE(a)) {
		if (!bigint_cpy(r, b))
			return 0;
		if (IS_NEGATIVE(a) || IS_NEGATIVE(b))
			SET_NEGATIVE(r);
		return 1;
	}
	if (IS_ONE(b)) {
		if (!bigint_cpy(r, a))
			return 0;
		if (IS_NEGATIVE(a) || IS_NEGATIVE(b))
			SET_NEGATIVE(r);
		return 1;
	}
	if (b->offset > a->offset) {
		tmp = a;
		a = b;
		b = tmp;
	}
	al = a->offset;
	bl = b->offset;
	rl = al + bl;
	if (!bigint_init(&ret, rl))
		return 0;
	if (IS_NEGATIVE(a) || IS_NEGATIVE(b))
		SET_NEGATIVE(&ret);
	ap = a->data;
	bp = b->data;
	rp = ret.data;
	cp = &(rp[al]);
	for (i = 0; i < bl; i++) {
		carry = 0;
		for (j = 0; j < al; j++) {
			carry +=
			    ((unsigned long long) ap[j]) *
			    ((unsigned long long) bp[i])
			    + ((unsigned long long) rp[i + j]);
			rp[i + j] = (unsigned int) (carry & 0xFFFFFFFF);
			carry >>= 32;
		}
		cp[i] = (unsigned int) carry;
	}
	ret.offset = rl;
	bigint_trim(&ret);
	if (!bigint_cpy(r, &ret)) {
		bigint_clean(&ret);
		return 0;
	}
	bigint_clean(&ret);
	if (IS_NEGATIVE(a) || IS_NEGATIVE(b))
		SET_NEGATIVE(r);
	return 1;
}

int
bigint_fix(struct bigint_t *a, const struct bigint_t *n)
{
	while (IS_NEGATIVE(a)) {
		SET_POSITIVE(a);
		if (!bigint_sub(a, n, a))
			return 0;
	}
	return 1;
}

int
bigint_lsh(struct bigint_t *r, const struct bigint_t *a, unsigned int n)
{
	unsigned int nw;
	unsigned int nb;
	unsigned int rb;
	int i;
	struct bigint_t tmp;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)))
		return 0;
#endif
	if (n == 0)
		return bigint_cpy(r, a);
	nw = n / (sizeof (unsigned int) * 8);
	nb = n % (sizeof (unsigned int) * 8);
	rb = sizeof (unsigned int) * 8 - nb;
	if (!bigint_init(&tmp, a->offset + nw + 1))
		return 0;
	if (!nb)
		memcpy(&(tmp.data[nw]), a->data,
		       a->offset * sizeof (unsigned int));
	else {
		for (i = a->offset; i > 0; i--) {
			tmp.data[i + nw] |= a->data[i - 1] >> rb;
			tmp.data[i + nw - 1] = a->data[i - 1] << nb;
		}
	}
	tmp.offset = a->offset + nw + 1;
	bigint_trim(&tmp);
	bigint_cpy(r, &tmp);
	bigint_clean(&tmp);
	return 1;
}

int
bigint_rsh(struct bigint_t *r, const struct bigint_t *a, unsigned int n)
{
	unsigned int nw;
	unsigned int nb;
	unsigned int rb;
	unsigned int i;
	struct bigint_t tmp;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)))
		return 0;
#endif

	if (n == 0)
		return bigint_cpy(r, a);
	if (bigint_bits(a) <= n) {
		if (!bigint_extend(r, 1))
			return 0;
		SET_ZERO(r);
		return 1;
	}
	nw = n / (sizeof (unsigned int) * 8);
	nb = n % (sizeof (unsigned int) * 8);
	rb = sizeof (unsigned int) * 8 - nb;
	if (!bigint_init(&tmp, a->offset - nw))
		return 0;
	if (!nb)
		memcpy(tmp.data, &(a->data[nw]),
		       sizeof (unsigned int) * (a->offset - nw));
	else {
		for (i = 0; i < a->offset - nw - 1; i++)
			tmp.data[i] |=
			    (a->data[i + nw] >> nb) | (a->
						       data[i + nw + 1] << rb);
		tmp.data[i] = a->data[i + nw] >> nb;
	}
	tmp.offset = a->offset - nw;
	bigint_trim(&tmp);
	bigint_cpy(r, &tmp);
	bigint_clean(&tmp);
	return 1;
}

int
bigint_mod(struct bigint_t *r,
	   const struct bigint_t *a, const struct bigint_t *b)
{
	unsigned int na;
	unsigned int nb;
	int i;
	struct bigint_t tmp;
	struct bigint_t ret;
	int res;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)) || (!bigint_check(b)))
		return 0;
#endif

	res = 0;
	if (IS_ZERO(b))
		return 0;
	if (IS_ONE(b)) {
		if (!bigint_extend(r, 1))
			return 0;
		SET_ZERO(r);
		return 1;
	}
	if (!bigint_init(&ret, 0))
		return 0;
	if (IS_NEGATIVE(a) || IS_NEGATIVE(b))
		SET_NEGATIVE(&ret);
	if (!bigint_cpy(&ret, a)) {
		bigint_clean(&ret);
		return 0;
	}
	if (bigint_cmp(a, b) < 0) {
		bigint_trim(&ret);
		res = 1;
		goto out;
	}
	na = bigint_bits(a);
	nb = bigint_bits(b);
	if (!bigint_init(&tmp, a->offset))
		goto errout;

	if (!bigint_lsh(&tmp, b, na - nb))
		goto errout;

	for (i = na - nb; i >= 0; i--) {
		if (bigint_cmp(&ret, &tmp) >= 0) {
			if (!bigint_sub(&ret, &ret, &tmp))
				goto errout;
		}
		if (!bigint_rsh(&tmp, &tmp, 1))
			goto errout;
	}
	res = 1;
      out:
	if (!bigint_cpy(r, &ret))
		res = 0;
	else
		bigint_trim(r);
	if (IS_NEGATIVE(r))
		res = bigint_fix(r, b);
      errout:
	bigint_clean(&tmp);
	bigint_clean(&ret);
	return res;
}

int
bigint_setbit(struct bigint_t *a, unsigned offset)
{
#ifdef BIGINT_DEBUG
	if (!bigint_check(a))
		return 0;
#endif
	if ((offset / (8 * sizeof (unsigned int))) >= a->offset)
		return 0;

	a->data[offset / (8 * sizeof (unsigned int))] |=
	    1 << (offset % (8 * sizeof (unsigned int)));
	return 1;
}

int
bigint_clearbit(struct bigint_t *a, unsigned offset)
{
#ifdef BIGINT_DEBUG
	if (!bigint_check(a))
		return 0;
#endif
	if ((offset / (8 * sizeof (unsigned int))) >= a->offset)
		return 0;

	a->data[offset / (8 * sizeof (unsigned int))] &=
	    ~(1 << (offset % (8 * sizeof (unsigned int))));
	return 1;
}

/* cut from bit (not include) at offset */
int
bigint_trunc(struct bigint_t *a, unsigned int offset)
{
	unsigned int na;
	unsigned int nb;
	int i;

#ifdef BIGINT_DEBUG
	if (!bigint_check(a))
		return 0;
#endif

	na = offset / (8 * sizeof (unsigned int));
	nb = offset % (8 * sizeof (unsigned int));
	if (na >= a->offset)
		return 1;
	a->data[na] &= (0xFFFFFFFF) << nb;
	for (i = na - 1; i >= 0; i--)
		a->data[i] = 0;
	return 1;
}

/* cut to (included) bit at offset */
int
bigint_chop(struct bigint_t *a, unsigned offset)
{
	unsigned int na;
	unsigned int nb;
	unsigned int i;

#ifdef BIGINT_DEBUG
	if (!bigint_check(a))
		return 0;
#endif
	na = offset / (8 * sizeof (unsigned int));
	nb = offset % (8 * sizeof (unsigned int));
	if (na >= a->offset)
		return 1;
	if (nb)
		a->data[na] &= 0xFFFFFFFF >> (8 * sizeof (unsigned int) - nb);
	else
		a->data[na] = 0x00;
	for (i = a->offset - 1; i > na; i--)
		a->data[i] = 0;
	bigint_trim(a);
	return 1;
}

int
bigint_div(struct bigint_t *r,
	   const struct bigint_t *a, const struct bigint_t *b)
{
	unsigned int na;
	unsigned int nb;
	int i;
	struct bigint_t tmp;
	struct bigint_t ret;
	struct bigint_t ta;
	int res;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)) || (!bigint_check(b)))
		return 0;
#endif

	res = 0;

	if (IS_ZERO(b))
		return 0;

	if (IS_ONE(b))
		return bigint_cpy(r, a);

	if (!bigint_init(&ret, 0))
		return 0;

	if (IS_NEGATIVE(a) || IS_NEGATIVE(b))
		SET_NEGATIVE(&ret);

	if (bigint_cmp(a, b) < 0) {
		SET_ZERO(&ret);
		res = 1;
		goto out;;
	}

	na = bigint_bits(a);
	nb = bigint_bits(b);
	if (!bigint_extend(&ret, a->offset - b->offset + 1))
		goto out;

	ret.offset = a->offset - b->offset + 1;

	if (!bigint_init(&tmp, a->offset))
		goto errout;

	if (!bigint_lsh(&tmp, b, na - nb))
		goto errout;

	if (!bigint_init(&ta, 0))
		goto errout;

	if (!bigint_cpy(&ta, a))
		goto errout;

	for (i = na - nb; i >= 0; i--) {
		if (bigint_cmp(&ta, &tmp) >= 0) {
			bigint_setbit(&ret, i);
			if (!bigint_sub(&ta, &ta, &tmp))
				goto errout;
		}
		if (!bigint_rsh(&tmp, &tmp, 1))
			goto errout;
	}
	res = 1;
      out:
	if (!bigint_cpy(r, &ret))
		res = 0;
	else
		bigint_trim(r);
	if (IS_NEGATIVE(a) || IS_NEGATIVE(b))
		SET_NEGATIVE(r);
      errout:
	bigint_clean(&tmp);
	bigint_clean(&ret);
	bigint_clean(&ta);
	return res;
}

int
bigint_init_uint(struct bigint_t *r, unsigned int n)
{
	if (!bigint_init(r, 1))
		return 0;

	r->data[0] = n;
	r->offset = 1;
	bigint_trim(r);
	return 1;
}

struct bigint_t *
bigint_new_uint(const unsigned int n)
{
	struct bigint_t *r;

	if ((r = bigint_new(1)) == NULL)
		return NULL;
	r->data[0] = n;
	r->offset = 1;
	bigint_trim(r);
	return r;
}

int
bigint_init_bin(struct bigint_t *r, const unsigned char *n, unsigned int len)
{
	unsigned int nw;
	unsigned int nr;
	unsigned int i;

	nw = len / 4;
	nr = len % 4;
	if (!bigint_init(r, nw + ((nr) ? 1 : 0)))
		return 0;

	for (i = 0; i < nw; i++)
		r->data[i] = (unsigned int) n[len - i * 4 - 1] |
		    (unsigned int) n[len - i * 4 - 2] << 8 |
		    (unsigned int) n[len - i * 4 - 3] << 16 |
		    (unsigned int) n[len - i * 4 - 4] << 24;
	switch (nr) {
	case 1:
		r->data[i] = (unsigned int) n[0];
		break;
	case 2:
		r->data[i] = (unsigned int) n[1] | (unsigned int) n[0] << 8;
		break;
	case 3:
		r->data[i] = (unsigned int) n[2] |
		    (unsigned int) n[1] << 8 | (unsigned int) n[0] << 16;
		break;
	}
	r->offset = nw + ((nr) ? 1 : 0);
	bigint_trim(r);
	return 1;
}

struct bigint_t *
bigint_new_bin(const unsigned char *n, unsigned int len)
{
	struct bigint_t *r;
	unsigned int nw;
	unsigned int nr;
	unsigned int i;

	nw = len / 4;
	nr = len % 4;
	if ((r = bigint_new(nw + ((nr) ? 1 : 0))) == NULL)
		return NULL;
	for (i = 0; i < nw; i++)
		r->data[i] = (unsigned int) n[len - i * 4 - 1] |
		    (unsigned int) n[len - i * 4 - 2] << 8 |
		    (unsigned int) n[len - i * 4 - 3] << 16 |
		    (unsigned int) n[len - i * 4 - 4] << 24;
	switch (nr) {
	case 1:
		r->data[i] = (unsigned int) n[0];
		break;
	case 2:
		r->data[i] = (unsigned int) n[1] | (unsigned int) n[0] << 8;
		break;
	case 3:
		r->data[i] = (unsigned int) n[2] |
		    (unsigned int) n[1] << 8 | (unsigned int) n[0] << 16;
		break;
	}
	r->offset = nw + ((nr) ? 1 : 0);
	bigint_trim(r);
	return r;
}

int
bigint_binlen(const struct bigint_t *n)
{
	unsigned int len;

	len = bigint_bytes(n);
	return (!len) ? 1 : len;
}

int
bigint_tobin(const struct bigint_t *n, char *bin)
{
	unsigned int base;
	int i;
	unsigned int nr;

	if (IS_ZERO(n)) {
		bin[0] = 0x00;
		return 1;
	}
	nr = bigint_bytes(n) % 4;
	base = 0;
	switch (nr) {
	case 0:
		bin[base++] =
		    (unsigned char) ((n->data[n->offset - 1] & 0xFF000000) >>
				     24);
		bin[base++] =
		    (unsigned char) ((n->data[n->offset - 1] & 0x00FF0000) >>
				     16);
		bin[base++] =
		    (unsigned char) ((n->data[n->offset - 1] & 0x0000FF00) >>
				     8);
		bin[base++] =
		    (unsigned char) (n->data[n->offset - 1] & 0x000000FF);
		break;
	case 3:
		bin[base++] =
		    (unsigned char) ((n->data[n->offset - 1] & 0x00FF0000) >>
				     16);
		bin[base++] =
		    (unsigned char) ((n->data[n->offset - 1] & 0x0000FF00) >>
				     8);
		bin[base++] =
		    (unsigned char) (n->data[n->offset - 1] & 0x000000FF);
		break;
	case 2:
		bin[base++] =
		    (unsigned char) ((n->data[n->offset - 1] & 0x0000FF00) >>
				     8);
		bin[base++] =
		    (unsigned char) (n->data[n->offset - 1] & 0x000000FF);
		break;
	case 1:
		bin[base++] =
		    (unsigned char) (n->data[n->offset - 1] & 0x000000FF);
		break;
	}
	for (i = n->offset - 2; i >= 0; i--) {
		bin[base++] = (unsigned char) ((n->data[i] & 0xFF000000) >> 24);
		bin[base++] = (unsigned char) ((n->data[i] & 0x00FF0000) >> 16);
		bin[base++] = (unsigned char) ((n->data[i] & 0x0000FF00) >> 8);
		bin[base++] = (unsigned char) (n->data[i] & 0x000000FF);
	}
	return 1;
}

int
bigint_mod_add(struct bigint_t *r,
	       const struct bigint_t *a,
	       const struct bigint_t *b, const struct bigint_t *m)
{
#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) ||
	    (!bigint_check(a)) || (!bigint_check(b)) || (!bigint_check(m)))
		return 0;
#endif

	if (!bigint_add(r, a, b))
		return 0;
	if (!bigint_mod(r, r, m))
		return 0;
	return 1;
}

int
bigint_mod_sub(struct bigint_t *r,
	       const struct bigint_t *a,
	       const struct bigint_t *b, const struct bigint_t *m)
{
#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) ||
	    (!bigint_check(a)) || (!bigint_check(b)) || (!bigint_check(m)))
		return 0;
#endif

	if (!bigint_sub(r, a, b))
		return 0;
	if (!bigint_fix(r, m))
		return 0;
	if (!bigint_mod(r, r, m))
		return 0;
	return 1;
}

int
bigint_mod_mul(struct bigint_t *r,
	       const struct bigint_t *a,
	       const struct bigint_t *b, const struct bigint_t *m)
{
#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) ||
	    (!bigint_check(a)) || (!bigint_check(b)) || (!bigint_check(m)))
		return 0;
#endif
	if (!bigint_mul(r, a, b))
		return 0;
	if (!bigint_mod(r, r, m))
		return 0;
	return 1;
}

int
calculate_seed(int index, struct bigint_t *buffer, const struct bigint_t *m)
{
	int i;
	int j;
	int found;

	if (IS_VALID(&buffer[index]))
		return 1;

	if (!bigint_init(&buffer[index], 0))
		return 0;

	found = 0;
	for (i = index - 1; i >= 0; i--) {
		if (IS_VALID(&buffer[i])) {
			found = 1;
			break;
		}
	}

	if (found) {
		if (!bigint_cpy(&buffer[index], &buffer[i])) {
			bigint_clean(&buffer[index]);
			return 0;
		}
		for (j = 0; j < (index - i); j++) {
			if (!bigint_mod_mul
			    (&buffer[index], &buffer[index], &buffer[0], m)) {
				bigint_clean(&buffer[index]);
				return 0;
			}
		}
	} else
		return 0;
	return 1;
}

int
bigint_mod_exp(struct bigint_t *r,
	       const struct bigint_t *a,
	       const struct bigint_t *b, const struct bigint_t *m)
{
	int i;
	int j;
	unsigned char window;
	struct bigint_t buffer[BIGINT_TABLE_SIZE];
	struct bigint_t tmp;
	struct bigint_t ret;
	int res;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) ||
	    (!bigint_check(a)) || (!bigint_check(b)) || (!bigint_check(m)))
		return 0;
#endif

	if (IS_ZERO(m) || IS_ZERO(a))
		return 0;
	if (IS_ONE(m)) {
		if (!bigint_extend(r, 1))
			return 0;
		SET_ZERO(r);
		return 1;
	}
	if (IS_ZERO(b)) {
		if (!bigint_extend(r, 1))
			return 0;
		SET_ONE(r);
	}
	if (!bigint_init(&ret, 0))
		return 0;
	memset(buffer, 0, BIGINT_TABLE_SIZE * sizeof (struct bigint_t));
	if (!bigint_init(&buffer[0], 0)) {
		res = 0;
		goto end;
	}
	if (!bigint_mod(&buffer[0], a, m)) {
		res = 0;
		goto end;
	}
	if (IS_ZERO(&buffer[0])) {
		if (!bigint_extend(r, 1)) {
			res = 0;
			goto end;
		}
		SET_ZERO(r);
		res = 1;
		goto end;
	}
	if (!bigint_init(&tmp, 0)) {
		res = 0;
		goto end;
	}
	SET_ONE(&ret);
	for (i = b->offset - 1; i >= 0; i--) {
		window = (unsigned char) ((b->data[i] & 0xFF000000) >> 24);
		// printf("step 1\n");
		if (window) {
			window--;
			if (!calculate_seed(window, buffer, m)) {
				res = 0;
				goto end;
			}
			if (!bigint_cpy(&tmp, &buffer[window])) {
				res = 0;
				goto end;
			}
			for (j = 0; j < 24 + i * 32; j++) {
				if (!bigint_mod_mul(&tmp, &tmp, &tmp, m)) {
					res = 0;
					goto end;
				}
			}
			if (!bigint_mod_mul(&ret, &ret, &tmp, m)) {
				res = 0;
				goto end;
			}
		}
		window = (unsigned char) ((b->data[i] & 0x00FF0000) >> 16);
		// printf("step 2\n");
		if (window) {
			window--;
			if (!calculate_seed(window, buffer, m)) {
				res = 0;
				goto end;
			}
			if (!bigint_cpy(&tmp, &buffer[window])) {
				res = 0;
				goto end;
			}
			for (j = 0; j < 16 + i * 32; j++) {
				if (!bigint_mod_mul(&tmp, &tmp, &tmp, m)) {
					res = 0;
					goto end;
				}
			}
			if (!bigint_mod_mul(&ret, &ret, &tmp, m)) {
				res = 0;
				goto end;
			}
		}
		window = (unsigned char) ((b->data[i] & 0x0000FF00) >> 8);
		// printf("step 3\n");
		if (window) {
			window--;
			if (!calculate_seed(window, buffer, m)) {
				res = 0;
				goto end;
			}
			if (!bigint_cpy(&tmp, &buffer[window])) {
				res = 0;
				goto end;
			}
			for (j = 0; j < 8 + i * 32; j++) {
				if (!bigint_mod_mul(&tmp, &tmp, &tmp, m)) {
					res = 0;
					goto end;
				}
			}
			if (!bigint_mod_mul(&ret, &ret, &tmp, m)) {
				res = 0;
				goto end;
			}
		}
		window = (unsigned char) (b->data[i] & 0x000000FF);
		// printf("step 4\n");
		if (window) {
			window--;
			if (!calculate_seed(window, buffer, m)) {
				res = 0;
				goto end;
			}
			if (!bigint_cpy(&tmp, &buffer[window])) {
				res = 0;
				goto end;
			}
			for (j = 0; j < i * 32; j++) {
				if (!bigint_mod_mul(&tmp, &tmp, &tmp, m)) {
					res = 0;
					goto end;
				}
			}
			if (!bigint_mod_mul(&ret, &ret, &tmp, m)) {
				res = 0;
				goto end;
			}
		}
	}
	res = 1;
      end:
	bigint_trim(&ret);
	if (!bigint_cpy(r, &ret))
		res = 0;
	for (i = 0; i < BIGINT_TABLE_SIZE; i++)
		bigint_clean(&buffer[i]);
	bigint_clean(&tmp);
	bigint_clean(&ret);
	return res;
}

/* montgomerty algorithm functions */

int
bigint_product_mont(struct bigint_t *x,
		    const struct bigint_t *a,
		    const struct bigint_t *b,
		    const struct bigint_t *np,
		    const struct bigint_t *n, const unsigned int r)
{
	struct bigint_t t;
	struct bigint_t m;
	struct bigint_t tmp;

	int ret;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(x)) ||
	    (!bigint_check(a)) ||
	    (!bigint_check(b)) || (!bigint_check(np)) || (!bigint_check(n)))
		return 0;
#endif

	ret = 0;
	if (!bigint_init(&t, 0))
		goto out;

	if (!bigint_init(&m, 0))
		goto out;

	if (!bigint_init(&tmp, 0))
		goto out;

	if (!bigint_mul(&t, a, b))
		goto out;

	if (!bigint_mul(&m, &t, np))
		goto out;

	if (!bigint_chop(&m, r))
		goto out;

	if (!bigint_mul(&tmp, &m, n))
		goto out;

	if (!bigint_add(&tmp, &t, &tmp))
		goto out;

	if (!bigint_rsh(&tmp, &tmp, r))
		goto out;

	if (bigint_cmp(&tmp, n) > 0) {
		if (!bigint_sub(&tmp, &tmp, n))
			goto out;
	}

	if (!bigint_cpy(x, &tmp))
		goto out;

	ret = 1;
      out:
	bigint_clean(&t);
	bigint_clean(&m);
	bigint_clean(&tmp);
	return ret;
}

int
bigint_getbit(const struct bigint_t *n, const unsigned int offset)
{
	unsigned int na;
	unsigned int nb;

	na = offset / (8 * sizeof (unsigned int));
	nb = offset % (8 * sizeof (unsigned int));

	if (na + 1 > n->offset)
		return 0;
	else
		return ((n->data[na] & (1 << nb)) == 0) ? 0 : 1;
}

int
bigint_mod_exp_mont(struct bigint_t *r,
		    const struct bigint_t *a,
		    const struct bigint_t *b, const struct bigint_t *m)
{
	int ret;
	struct bigint_t ap;
	struct bigint_t xp;
	struct bigint_t np;
	struct bigint_t tmp;
	struct bigint_t one;
	unsigned int k;
	int i;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) ||
	    (!bigint_check(a)) || (!bigint_check(b)) || (!bigint_check(m)))
		return 0;
#endif
	ret = 0;
	if (!bigint_init(&ap, 0))
		goto out;

	if (!bigint_init(&xp, 0))
		goto out;

	if (!bigint_init(&np, 0))
		goto out;

	if (!bigint_init(&tmp, 0))
		goto out;

	if (!bigint_init(&one, 0))
		goto out;

	SET_ONE(&one);

	if (!bigint_round_mont(&k, m))
		goto out;

	if (!bigint_eea_mont(&np, m, k))
		goto out;

	if (!bigint_lsh(&tmp, a, k))
		goto out;

	if (!bigint_mod(&ap, &tmp, m))
		goto out;

	if (!bigint_lsh(&tmp, &one, k))
		goto out;

	if (!bigint_mod(&xp, &tmp, m))
		goto out;

	for (i = k - 1; i >= 0; i--) {
		if (!bigint_product_mont(&xp, &xp, &xp, &np, m, k))
			goto out;
		if (bigint_getbit(b, i)) {
			if (!bigint_product_mont(&xp, &ap, &xp, &np, m, k))
				goto out;
		}
	}
	if (!bigint_product_mont(r, &xp, &one, &np, m, k))
		goto out;
	ret = 1;
      out:
	bigint_clean(&ap);
	bigint_clean(&xp);
	bigint_clean(&np);
	bigint_clean(&tmp);
	bigint_clean(&one);
	return ret;
}

int
bigint_round_mont(unsigned int *r, const struct bigint_t *a)
{
#ifdef BIGINT_DEBUG
	if (!bigint_check(a))
		return 0;
#endif
	*r = bigint_bits(a);
	return 1;
}

int
bigint_mod_mul_mont(struct bigint_t *r,
		    const struct bigint_t *a,
		    const struct bigint_t *b, const struct bigint_t *m)
{
	int ret;
	struct bigint_t ap;
	struct bigint_t bp;
	struct bigint_t np;
	struct bigint_t tmp;
	struct bigint_t one;
	unsigned int k;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) ||
	    (!bigint_check(a)) || (!bigint_check(b)) || (!bigint_check(m)))
		return 0;
#endif

	ret = 0;

	if (!bigint_init(&ap, 0))
		goto out;

	if (!bigint_init(&bp, 0))
		goto out;

	if (!bigint_init(&np, 0))
		goto out;

	if (!bigint_init(&tmp, 0))
		goto out;

	if (!bigint_init(&one, 0))
		goto out;

	SET_ONE(&one);

	if (!bigint_round_mont(&k, m))
		goto out;

	if (!bigint_eea_mont(&np, m, k))
		goto out;

	if (!bigint_lsh(&tmp, a, k))
		goto out;

	if (!bigint_mod(&ap, &tmp, m))
		goto out;

	if (!bigint_lsh(&tmp, b, k))
		goto out;

	if (!bigint_mod(&bp, &tmp, m))
		goto out;

	if (!bigint_product_mont(r, &ap, &bp, &np, m, k))
		goto out;

	if (!bigint_product_mont(r, r, &one, &np, m, k))
		goto out;

	ret = 1;
      out:
	bigint_clean(&ap);
	bigint_clean(&bp);
	bigint_clean(&np);
	bigint_clean(&tmp);
	return ret;
}

int
bigint_eea_mont(struct bigint_t *np,
		const struct bigint_t *n, const unsigned int k)
{
	int ret;
	struct bigint_t y1;
	struct bigint_t y2;
	struct bigint_t y;
	struct bigint_t q;
	struct bigint_t a;
	struct bigint_t aa;
	struct bigint_t b;
	struct bigint_t tmp;
	struct bigint_t r;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(np)) || (!bigint_check(n)))
		return 0;
#endif

	ret = 0;

	if (IS_ZERO(n))
		return 0;

	if (!bigint_init(&y1, 0))
		goto out;

	if (!bigint_init(&y2, 0))
		goto out;

	if (!bigint_init(&y, 0))
		goto out;

	if (!bigint_init(&q, 0))
		goto out;

	if (!bigint_init(&a, 0))
		goto out;

	if (!bigint_init(&b, 0))
		goto out;

	if (!bigint_init(&aa, 0))
		goto out;

	if (!bigint_init(&tmp, 0))
		goto out;

	if (!bigint_init(&r, 0))
		goto out;

	SET_ONE(&a);

	if (!bigint_lsh(&a, &a, k))
		goto out;

	if (!bigint_cpy(&aa, &a))
		goto out;

	if (!bigint_cpy(&b, n))
		goto out;

	SET_ZERO(&y2);
	SET_ONE(&y1);

	while ((!IS_ZERO(&b)) && (!IS_NEGATIVE(&b))) {
		if (!bigint_div(&q, &a, &b))
			goto out;

		if (!bigint_mod(&r, &a, &b))
			goto out;

		if (!bigint_mul(&tmp, &q, &y1))
			goto out;

		if (!bigint_sub(&y, &y2, &tmp))
			goto out;

		if (!bigint_cpy(&a, &b))
			goto out;

		if (!bigint_cpy(&b, &r))
			goto out;

		if (!bigint_cpy(&y2, &y1))
			goto out;

		if (!bigint_cpy(&y1, &y))
			goto out;

		bigint_trim(&b);
	}

	if (!IS_ONE(&a))
		goto out;

	if (IS_NEGATIVE(&y2))
		SET_POSITIVE(&y2);
	else {
		SET_NEGATIVE(&y2);
		if (!bigint_fix(&y2, &aa))
			goto out;
	}
	if (!bigint_cpy(np, &y2))
		goto out;
	ret = 1;
      out:
	bigint_clean(&y1);
	bigint_clean(&y2);
	bigint_clean(&y);
	bigint_clean(&q);
	bigint_clean(&a);
	bigint_clean(&b);
	bigint_clean(&aa);
	bigint_clean(&tmp);
	bigint_clean(&r);
	return ret;
}

int
bigint_div_mont(struct bigint_t *r,
		const struct bigint_t *a, const unsigned int b)
{
#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)))
		return 0;
#endif

	if (b == 0)
		return bigint_cpy(r, a);

	if (!bigint_rsh(r, a, b))
		return 0;

	bigint_trim(r);
	return 1;
}

int
bigint_mod_mont(struct bigint_t *r,
		const struct bigint_t *a, const unsigned int b)
{
	int ret;
	struct bigint_t tmp;

#ifdef BIGINT_DEBUG
	if ((!bigint_check(r)) || (!bigint_check(a)))
		return 0;
#endif

	ret = 0;
	if (b == 0) {
		if (!bigint_extend(r, 0))
			return 0;
		SET_ZERO(r);
		return 1;
	}
	if (!bigint_init(&tmp, 0))
		return 0;

	if (!bigint_cpy(&tmp, a))
		goto out;

	if (!bigint_chop(&tmp, b))
		goto out;

	if (!bigint_cpy(r, &tmp))
		goto out;

	bigint_trim(r);
	ret = 1;
      out:
	bigint_clean(&tmp);
	return ret;
}
