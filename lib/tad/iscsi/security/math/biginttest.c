/*
 * biginttest.c
 * Chong Zhang
 * March 11, 2003
 * testing procedures for 
 * the big integer operation.
 */

#include <stdio.h>

#include "bigint.h"

static unsigned char bin_a[] =
    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
0x0D, 0x0E, 0x0F };
static unsigned char bin_b[] =
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF };
static unsigned char bin_c[] =
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF };

void
basic_test()
{
	struct bigint_t *a;
	struct bigint_t *b;
	struct bigint_t *c;
	struct bigint_t *d;
	struct bigint_t *e;
	struct bigint_t *f;
	struct bigint_t *r;
	char bins[256];
	int len, i;
/*
	printf("testing bigint_new with size 0");
	if ((a = bigint_new(0)) == NULL)
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(a);
		bigint_free(a);
	}

	printf("testing bigint_new with size 1\n");
	if ((a = bigint_new(1)) == NULL)
	{
		printf("error\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(a);
		bigint_free(a);
	}

	printf("testing bigint_new with size 10000000\n");
	if ((a = bigint_new(10000000)) == NULL)
	{
		printf("error\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(a);
		bigint_free(a);
	}
	printf("testing bigint_extend\n");
	printf("create a bigint with size 0");
	if ((a = bigint_new(0)) == NULL)
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(a);
	}
	bigint_checkmemory();
	printf("extend it to size 1000000\n");
	if (!bigint_extend(a, 1000000))
	{
		printf("failed\n");
		bigint_free(a);
		return;
	}
	else
	{
		printf("succeeded\n");
	}
	bigint_checkmemory();
	printf("extend it to size 100\n");
	if (!bigint_extend(a, 100))
	{
		printf("failed\n");
		bigint_free(a);
		return;
	}
	else
	{
		printf("succeeded\n");
	}
	bigint_checkmemory();
	printf("extend it to size 10000\n");
	if (!bigint_extend(a, 10000))
	{
		printf("failed\n");
		bigint_free(a);
		return;
	}
	else
	{
		printf("succeeded\n");
	}
	printf("extend it to size 10000000\n");
	if (!bigint_extend(a, 10000000))
	{
		printf("failed\n");
		bigint_free(a);
		return;
	}
	else
	{
		printf("succeeded\n");
	}
	bigint_free(a);
	bigint_checkmemory();
	printf("create bigint a from a integer 0x12345678\n");
	if ((a = bigint_new_uint(0x12345678)) == NULL)
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(a);
		bigint_free(a);
	}
	printf("create bigint a from a integer 0x00\n");
	if ((a = bigint_new_uint(0x00)) == NULL)
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(a);
		bigint_free(a);
	}
	printf("create bigint a from binary bin_c\n");
	if ((a = bigint_new_bin(bin_c, 15)) == NULL)
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(a);
		bigint_free(a);
	}
	printf("create bigint a from binary bin_a\n");
	if ((a = bigint_new_bin(bin_a, 15)) == NULL)
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(a);
	}
	printf("test non-zero bigint\n");
	printf("check bigint_bits\n");
	printf("total bits: %d\n", bigint_bits(a));
	printf("check bigint_bytes\n");
	printf("total bytes: %d\n", bigint_bytes(a));
	printf("check bigint_binlen\n");
	len = bigint_binlen(a);
	printf("total bytes: %d\n", len);
	printf("check bigint_tobin\n");
	bigint_tobin(a, bins);
	printf("binary is: ");
	for (i = 0;i < len;i ++)
		printf("%02x ", bins[i]);
	printf("\n");
	printf("test bigint_clear");
	if (!bigint_clear(a))
	{
		printf("failed\n");
		bigint_free(a);
	}
	else
	{
		printf("succeeded");
		bigint_print(a);
	}
	printf("test zero bigint\n");
	printf("check bigint_bits\n");
	printf("total bits: %d\n", bigint_bits(a));
	printf("check bigint_bytes\n");
	printf("total bytes: %d\n", bigint_bytes(a));
	printf("check bigint_binlen\n");
	len = bigint_binlen(a);
	printf("total bytes: %d\n", len);
	printf("check bigint_tobin\n");
	bigint_tobin(a, bins);
	printf("binary is: ");
	for (i = 0;i < len;i ++)
		printf("%02x ", bins[i]);
	printf("\n");
	bigint_free(a);
	printf("create bigint a from binary\n");
	if ((a = bigint_new_bin(bin_a, 15)) == NULL)
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(a);
	}
	printf("create bigint b from binary\n");
	if ((b = bigint_new_bin(bin_b, 15)) == NULL)
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(b);
	}
	printf("create bigint c from binary\n");
	if ((c = bigint_new_bin(bin_c, 15)) == NULL)
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(c);
	}
	printf("test bigint_cmp a < b\n");
	if (bigint_cmp(a, b) < 0)
	{
		printf("succeeded\n");
	}
	else
	{
		printf("failed\n");
	}
	printf("test bigint_cmp b > c\n");
	if (bigint_cmp(b, c) > 0)
	{
		printf("succeeded\n");
	}
	else
	{
		printf("failed\n");
	}
	printf("test bigint_cmp b = b\n");
	if (bigint_cmp(b, b) == 0)
	{
		printf("succeeded\n");
	}
	else
	{
		printf("failed\n");
	}
	printf("test bigint_cpy a to b\n");
	if (!bigint_cpy(b, a))
	{
		printf("failed\n");
	}
	else
	{
		if (bigint_cmp(a, b) == 0)
		{
			printf("succeeded\n");
			bigint_print(b);
		}
		else
		{
			printf("failed\n");
		}
	}
	printf("test rsh\n");
	printf("right shit c to b by 1 bit\n");
	printf("before shift\n");
	bigint_print(c);
	printf("after shift\n");
	if (!bigint_rsh(b, c, 1))
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(b);
	}
	printf("right shit c to b by 34 bit\n");
	printf("before shift\n");
	bigint_print(c);
	printf("after shift\n");
	if (!bigint_rsh(b, c, 34))
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(b);
	}
	printf("right shit c to c by 1 bit\n");
	printf("before shift\n");
	bigint_print(c);
	printf("after shift\n");
	if (!bigint_rsh(c, c, 1))
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(c);
	}
	printf("right shit c to c by 34 bit\n");
	printf("before shift\n");
	bigint_print(c);
	printf("after shift\n");
	if (!bigint_rsh(c, c, 34))
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(c);
	}

	printf("test lsh\n");
	printf("left shit c to b by 1 bit\n");
	printf("before shift\n");
	bigint_print(c);
	printf("after shift\n");
	if (!bigint_lsh(b, c, 1))
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(b);
	}
	printf("left shit c to b by 34 bit\n");
	printf("before shift\n");
	bigint_print(c);
	printf("after shift\n");
	if (!bigint_lsh(b, c, 34))
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(b);
	}
	printf("left shit c to c by 1 bit\n");
	printf("before shift\n");
	bigint_print(c);
	printf("after shift\n");
	if (!bigint_lsh(c, c, 1))
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(c);
	}
	printf("left shit c to c by 34 bit\n");
	printf("before shift\n");
	bigint_print(c);
	printf("after shift\n");
	if (!bigint_lsh(c, c, 34))
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(c);
	}
	bigint_free(a);
	bigint_free(b);
	bigint_free(c);
*/
//      printf("test add\n");
	a = bigint_new_bin(bin_a, 15);
	b = bigint_new_bin(bin_b, 15);
	c = bigint_new_bin(bin_c, 15);
	d = bigint_new_uint(0x05);
	e = bigint_new_uint(0x08);
	f = bigint_new_uint(0x37);
	r = bigint_new(0);

/*	printf("test a + d\n");
	for (i = 0;i < 15;i ++)
	{
		if (!bigint_lsh(d, d, 8))
		{
			printf("bigint_lsh failed\n");
			goto out;
		}
		if (!bigint_add(r, a, d))
		{
			printf("failed\n");
		}
		else
		{
			printf("succeeded\n");
			printf("big integer a\n");
			bigint_print(a);
			printf("big integer d\n");
			bigint_print(d);
			printf("big integer r = a + d\n");
			bigint_print(r);
		}
	}
	printf("big integer a\n");
	bigint_print(a);
	printf("big integer b\n");
	bigint_print(b);
	if (!bigint_add(a, b, a))
	{
		printf("failed\n");
	}
	else
	{
		printf("succeeded\n");
		printf("big integer a = a + b");
		bigint_print(a);
	}
	printf("testing sub\n");
	for (i = 0;i < 15;i ++)
	{
		if (!bigint_rsh(d, d, 8))
		{
			printf("bigint_rsh failed\n");
			goto out;
		}
		if (!bigint_sub(r, a, d))
		{
			printf("bigint_sub failed\n");
		}
		else
		{
			printf("succeeded\n");
			printf("bigint a\n");
			bigint_print(a);
			printf("bigint d\n");
			bigint_print(d);
			printf(" r = a - d\n");
			bigint_print(r);
		}
	}
	printf("integer b\n");
	bigint_print(b);
	printf("integer a\n");
	bigint_print(a);
	printf("integer r = a -b\n");
	if (!bigint_sub(r, a, b))
	{
		printf("bigint_sub failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(r);
	}
	printf("integer b\n");
	bigint_print(b);
	printf("integer c\n");
	bigint_print(c);
	printf("integer r = b -c\n");
	if (!bigint_sub(r, c, b))
	{
		printf("bigint_sub failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(r);
	}

	printf("integer a\n");
	bigint_print(a);
	printf("integer d\n");
	bigint_print(d);
	printf("integer r = a * d\n");
	if (!bigint_mul(r, a, d))
	{
		printf("bigint_mul failed\n");
		goto out;
	}
	else
	{
		printf("succeeded\n");
		bigint_print(r);
	}
	printf("integer a\n");
	bigint_print(a);
	printf("integer e\n");
	bigint_print(e);
	printf("integer r = a * e\n");
	if (!bigint_mul(r, a, e))
	{
		printf("bigint_mul failed\n");
		goto out;
	}
	else
	{
		printf("succeeded\n");
		bigint_print(r);
	}

	printf("integer a\n");
	bigint_print(a);
	printf("integer f\n");
	bigint_print(c);
	printf("integer r = a * c\n");
	if (!bigint_mul(r, a, c))
	{
		printf("bigint_mul failed\n");
		goto out;
	}
	else
	{
		printf("succeeded\n");
		bigint_print(r);
	}

	if (!bigint_sub(r, e, d))
	{
		printf("bigint_sub failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(r);
	}
	if (!bigint_fix(r, a))
	{
		printf("bigint_fix failed\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(r);
	}
*/
	printf("integer f\n");
	bigint_print(f);
	printf("integer e\n");
	bigint_print(e);
	printf("integer d\n");
	bigint_print(d);
	printf("r = f^d | e\n");

/*	for (i = 0;i < 10; i++)
	{
	if (!bigint_mod_mul(r, f, d, e))
	{
		printf("bigint_mod failded\n");
	}
	else
	{
		printf("succeeded\n");
		bigint_print(r);
	}
	}

*/
	if (!bigint_mod_exp(r, f, d, e)) {
		printf("bigint_exp_mod failed\n");
	} else {
		printf("test succeeded\n");
		bigint_print(r);
	}

      out:
	bigint_free(a);
	bigint_free(b);
	bigint_free(c);
	bigint_free(d);
	bigint_free(e);
	bigint_free(f);
	bigint_free(r);
	bigint_checkmemory();
}

int
main(void)
{
	basic_test();
	return 1;
}
