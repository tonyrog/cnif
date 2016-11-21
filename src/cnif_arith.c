//
// arithmetic 
//
#include "../include/cnif_big.h"

#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#define MAX(a,b) (((a)>(b)) ? (a) : (b))

/* add a and b with carry in + out */
#define DSUMc(a,b,c,s) do {						\
	ErlNifBigDigit ___cr = (c);					\
	ErlNifBigDigit ___xr = (a)+(___cr);				\
	ErlNifBigDigit ___yr = (b);					\
	___cr = (___xr < ___cr);					\
	___xr = ___yr + ___xr;						\
	___cr += (___xr < ___yr);					\
	s = ___xr;							\
	c = ___cr;							\
    }  while(0)

/* add a and b with carry out */
#define DSUM(a,b,c,s) do {						\
	ErlNifBigDigit ___xr = (a);					\
	ErlNifBigDigit ___yr = (b);					\
	___xr = ___yr + ___xr;						\
	s = ___xr;							\
	c = (___xr < ___yr);						\
    }  while(0)

#define DSUBb(a,b,r,d) do {						\
	ErlNifBigDigit ___cr = (r);					\
	ErlNifBigDigit ___xr = (a);					\
	ErlNifBigDigit ___yr = (b)+___cr;				\
	___cr = (___yr < ___cr);					\
	___yr = ___xr - ___yr;						\
	___cr += (___yr > ___xr);					\
	d = ___yr;							\
	r = ___cr;							\
    } while(0)

#define DSUB(a,b,r,d) do {				\
	ErlNifBigDigit ___xr = (a);			\
	ErlNifBigDigit ___yr = (b);			\
	___yr = ___xr - ___yr;				\
	r = (___yr > ___xr);				\
	d = ___yr;					\
    } while(0)

// calculate dst = src1 + src2 ( + carry)
static inline ErlNifBigDigit add3(ErlNifBigDigit* src1,
				   ErlNifBigDigit* src2, 
				   ErlNifBigDigit* dst, 
				   ErlNifBigDigit carry,
				   ERL_NIF_UINT* ip,
				   ERL_NIF_UINT n)
{
    ERL_NIF_UINT i=*ip;

    while(i < n) {
	ErlNifBigDigit x = src2[i];
	ErlNifBigDigit y = src1[i]+carry;
	carry = (y < carry);
	y = x + y;
	carry += (y < x);
	dst[i] = y;
	i++;
    }
    *ip = i;
    return carry;
}

// calculate dst = src ( + digit )
static inline ErlNifBigDigit add2(ErlNifBigDigit* src,
				  ErlNifBigDigit* dst, 
				  ErlNifBigDigit d,
				  ERL_NIF_UINT* ip,
				  ERL_NIF_UINT n)
{
    ERL_NIF_UINT i=*ip;
    
    while(i < n) {
	ErlNifBigDigit x = src[i]+d;
	d = (x < d);
	dst[i] = x;
	i++;
    }
    *ip = i;
    return d;
}

// calculate dst = src1 - src2 ( - borrow )
// assume that src1 >= (src2+borrow)
static inline ErlNifBigDigit sub3(ErlNifBigDigit* src1,
				  ErlNifBigDigit* src2, 
				  ErlNifBigDigit* dst, 
				  ErlNifBigDigit borrow,
				  ERL_NIF_UINT* ip,
				  ERL_NIF_UINT n)
{
    ERL_NIF_UINT i=*ip;

    while(i < n) {
	ErlNifBigDigit x = src1[i];
	ErlNifBigDigit y = src2[i]+borrow;
	borrow = (y < borrow);
	y = x - y;
	borrow += (y > x);
	dst[i] = y;
	i++;
    }
    *ip = i;
    return borrow;
}

// dst = src - d
static inline ErlNifBigDigit sub2(ErlNifBigDigit* src,
				  ErlNifBigDigit* dst, 
				  ErlNifBigDigit d,
				  ERL_NIF_UINT* ip,
				  ERL_NIF_UINT n)
{
    ERL_NIF_UINT i=*ip;
    
    while(i < n) {
	ErlNifBigDigit x = src[i];
	ErlNifBigDigit y = x - d;
	d = (y > x);
	dst[i] = y;
	i++;
    }
    *ip = i;
    return d;
}

static inline int comp(ErlNifBigDigit* src1, ERL_NIF_UINT n1,
		       ErlNifBigDigit* src2, ERL_NIF_UINT n2)
{
    if (n1 < n2)
	return -1;
    else if (n1 > n2)
	return 1;
    else {
	if (src1 == src2)
	    return 0;
	src1 += (n1-1);
	src2 += (n2-1);
	while((n1 > 0) && (*src1 == *src2)) {
	    src1--;
	    src2--;
	    n1--;
	}
	if (n1 == 0)
	    return 0;
	return (*src1 < *src2) ? -1 : 1;
    }
}

// Remove trailing digits from bitwise operations
// convert negative numbers to one complement

static ERL_NIF_UINT btrail(ErlNifBigDigit* src,ERL_NIF_UINT n,ERL_NIF_UINT sign)
{
    if (sign) { 
	ERL_NIF_UINT i;
	ErlNifBigDigit d;

	while((n>1) && ((d = src[n-1]) == D_MASK))
	    n--;
	if (d == D_MASK)
	    src[n-1] = 0;
	else {
	    ErlNifBigDigit prev_mask = 0;
	    ErlNifBigDigit  mask = (DCONST(1) << (D_EXP-1));

	    while((d & mask) == mask) {
		prev_mask = mask;
		mask = (prev_mask >> 1) | (DCONST(1)<<(D_EXP-1));
	    }
	    src[n-1] = ~d & ~prev_mask;
	}
	for (i=0; i < n-1; i++)
	    src[i] = ~src[i];
	i=0;
	add2(src, src, 1, &i, n);
	return i;
    }
    return cnif_big_trail(src, n);
}


// Assume src1len >= src2len
static int band(ErlNifBigDigit* src1, ERL_NIF_UINT sign1, ERL_NIF_UINT src1len,
		ErlNifBigDigit* src2, ERL_NIF_UINT sign2, ERL_NIF_UINT src2len,
		ErlNifBignum* dst)
{
    ErlNifBigDigit* r  = dst->digits;
    short sign = sign1 && sign2;

    src1len -= src2len;

    if (!sign1) {
	if (!sign2) {
	    while(src2len--)
		*r++ = *src1++ & *src2++;
	}
	else {
	    ErlNifBigDigit b;
	    ErlNifBigDigit c;

	    DSUB(*src2,1,b,c);
	    *r++ = *src1++ & ~c;
	    src2++;
	    src2len--;
	    while(src2len--) {
		DSUBb(*src2,0,b,c);
		*r++ = *src1++ & ~c;
		src2++;
	    }
	    while (src1len--) {
		*r++ = *src1++;
	    }
	}
    }
    else {
	if (!sign2) {
	    ErlNifBigDigit b;
	    ErlNifBigDigit c;

	    DSUB(*src1,1,b,c);
	    *r = ~c & *src2;
	    src1++; src2++; r++;
	    src2len--;
	    while(src2len--) {
		DSUBb(*src1,0,b,c);
		*r++ = ~c & *src2++;
		src1++;
	    }
	}
	else {
	    ErlNifBigDigit b1, b2;
	    ErlNifBigDigit c1, c2;

	    DSUB(*src1,1,b1,c1);
	    DSUB(*src2,1,b2,c2);
	    *r++ = ~c1 & ~c2;
	    src1++; src2++;
	    src2len--;
	    while(src2len--) {
		DSUBb(*src1,0,b1,c1);
		DSUBb(*src2,0,b2,c2);
		*r++ = ~c1 & ~c2;
		src1++; src2++;
	    }
	    while(src1len--)
		*r++ = ~*src1++;
	}
    }
    dst->sign = sign;
    dst->size = btrail(dst->digits,(r - dst->digits), sign);
    return 1;
}

// Assume src1len >= src2len
static int bor(ErlNifBigDigit* src1, ERL_NIF_UINT sign1, ERL_NIF_UINT src1len,
	       ErlNifBigDigit* src2, ERL_NIF_UINT sign2, ERL_NIF_UINT src2len,
	       ErlNifBignum* dst)
{
    ErlNifBigDigit* r  = dst->digits;
    short sign = sign1 || sign2;

    src1len -= src2len;

    if (!sign1) {
	if (!sign2) {
	    while(src2len--)
		*r++ = *src1++ | *src2++;
	    while(src1len--)
		*r++ = *src1++;
	}
	else {
	    ErlNifBigDigit b;
	    ErlNifBigDigit c;

	    DSUB(*src2,1,b,c);
	    *r++ = *src1++ | ~c;
	    src2++;
	    src2len--;
	    while(src2len--) {
		DSUBb(*src2,0,b,c);
		*r++ = *src1++ | ~c;
		src2++;
	    }
	}
    }
    else {
	if (!sign2) {
	    ErlNifBigDigit b;
	    ErlNifBigDigit c;

	    DSUB(*src1,1,b,c);
	    *r++ = ~c | *src2++;
	    src1++;
	    src2len--;
	    while(src2len--) {
		DSUBb(*src1,0,b,c);
		*r++ = ~c | *src2++;
		src1++;
	    }
	    while(src1len--) {
		DSUBb(*src1,0,b,c);
 		*r++ = ~c;
 		src1++;
	    }
	}
	else {
	    ErlNifBigDigit b1, b2;
	    ErlNifBigDigit c1, c2;

	    DSUB(*src1,1,b1,c1);
	    DSUB(*src2,1,b2,c2);
	    *r++ = ~c1 | ~c2;
	    src1++; src2++;
	    src2len--;
	    while(src2len--) {
		DSUBb(*src1,0,b1,c1);
		DSUBb(*src2,0,b2,c2);
		*r++ = ~c1 | ~c2;
		src1++; src2++;
	    }
	}
    }
    dst->sign = sign;
    dst->size = btrail(dst->digits, (r - dst->digits), sign);
    return 1;
}


// Assume src1len >= src2len
static int bxor(ErlNifBigDigit* src1, ERL_NIF_UINT sign1, ERL_NIF_UINT src1len,
		ErlNifBigDigit* src2, ERL_NIF_UINT sign2, ERL_NIF_UINT src2len,
		ErlNifBignum* dst)
{
    ErlNifBigDigit* r  = dst->digits;
    short sign = sign1 != sign2;

    src1len -= src2len;

    if (!sign1) {
	if (!sign2) {
	    while(src2len--)
		*r++ = *src1++ ^ *src2++;
	    while(src1len--)
		*r++ = *src1++;
	}
	else {
	    ErlNifBigDigit b;
	    ErlNifBigDigit c;

	    DSUB(*src2,1,b,c);
	    *r++ = *src1++ ^ ~c;
	    src2++;
	    src2len--;
	    while(src2len--) {
		DSUBb(*src2,0,b,c);
		*r++ = *src1++ ^ ~c;
		src2++;
	    }
	    while(src2len--)
		*r++ = ~*src1++;
	}
    }
    else {
	if (!sign2) {
	    ErlNifBigDigit b;
	    ErlNifBigDigit c;

	    DSUB(*src1,1,b,c);
	    *r++ = ~c ^ *src2++;
	    src1++;
	    src2len--;
	    while(src2len--) {
		DSUBb(*src1,0,b,c);
		*r++ = ~c ^ *src2++;
		src1++;
	    }
	    while(src1len--)
 		*r++ = ~*src1++;
	}
	else {
	    ErlNifBigDigit b1, b2;
	    ErlNifBigDigit c1, c2;

	    DSUB(*src1,1,b1,c1);
	    DSUB(*src2,1,b2,c2);
	    *r++ = ~c1 ^ ~c2;
	    src1++; src2++;
	    src2len--;
	    while(src2len--) {
		DSUBb(*src1,0,b1,c1);
		DSUBb(*src2,0,b2,c2);
		*r++ = ~c1 ^ ~c2;
		src1++; src2++;
	    }
	    while(src1len--) {
		*r++ = *src1++;
	    }	    
	}
    }
    dst->sign = sign;
    dst->size = btrail(dst->digits,(r - dst->digits), sign);
    return 1;
}

static int addsub(ErlNifBigDigit* src1,ERL_NIF_UINT sign1,ERL_NIF_UINT src1len,
		  ErlNifBigDigit* src2,ERL_NIF_UINT sign2,ERL_NIF_UINT src2len,
		  ErlNifBignum* dst)
{
    if (MAX(src1len, src2len) > dst->asize)
	return 0;
    if (sign1 == sign2) {
	ERL_NIF_UINT i=0;
	ErlNifBigDigit carry = 0;
	ErlNifBigDigit* ds;

	if (src1len < src2len) {
	    carry = add3(src1, src2, dst->digits, carry, &i, src1len);
	    carry = add2(src2, dst->digits, carry, &i, src2len);
	}
	else {
	    carry = add3(src2, src1, dst->digits, carry, &i, src2len);
	    carry = add2(src1, dst->digits, carry, &i, src1len);
	}
	if (carry) {
	    if (i >= dst->asize) return 0;
	    dst->digits[i] = carry;
	    i++;
	}
	dst->size = i;
	dst->sign = sign1;
    }
    else {
	int cmp = comp(src1, src1len, src2, src2len);
	if (cmp == 0) {
	    dst->digits[0] = 0;
	    dst->size = 1;
	    dst->sign = 0;
	}
	else if (cmp > 0) {
	    ERL_NIF_UINT i=0;
	    ErlNifBigDigit borrow = 0;

	    borrow = sub3(src1,src2,dst->digits,borrow,&i,
			  src2len);
	    sub2(src1,dst->digits,borrow,&i,src1len);
	    dst->size = cnif_big_trail(dst->digits,i);
	    dst->sign = cnif_big_is_zero(dst) ? 0 : sign1;
	}
	else {
	    ERL_NIF_UINT i=0;
	    ErlNifBigDigit borrow = 0;

	    borrow = sub3(src2,src1,dst->digits,borrow,&i,src1len);
	    sub2(src2,dst->digits,borrow,&i,src2len);
	    dst->size = cnif_big_trail(dst->digits,i);
	    dst->sign = cnif_big_is_zero(dst) ? 0 : sign2;
	}
    }
    return 1;
}

static int bnot(ErlNifBigDigit* src, ERL_NIF_UINT sign, ERL_NIF_UINT srclen,
		ErlNifBignum* dst)
{
    ErlNifBigDigit one = 1;

    if (!sign)
	addsub(src, sign, srclen, &one, 1, 0, dst);
    else
	addsub(src, sign, srclen, &one, 1, 1, dst);
    dst->sign = !sign;
    return 1;
}

int cnif_big_add(ErlNifBignum* src1, ErlNifBignum* src2, ErlNifBignum* dst)
{
    return addsub(src1->digits, src1->sign, src1->size,
		  src2->digits, src2->sign, src2->size, dst);
}

int cnif_big_sub(ErlNifBignum* src1, ErlNifBignum* src2, ErlNifBignum* dst)
{
    return addsub(src1->digits, src1->sign, src1->size,
		  src2->digits, !src2->sign, src2->size, dst);
}

int cnif_big_neg(ErlNifBignum* src, ErlNifBignum* dst)
{
    int i;
    if (src != dst) {
	if (src->size > dst->asize)
	    return 0;
	for (i = 0; i < src->size; i++)
	    dst->digits[i] = src->digits[i];
	dst->size = src->size;
    }
    dst->sign = !src->sign;
    return 1;
}

int cnif_big_band(ErlNifBignum* src1, ErlNifBignum* src2, ErlNifBignum* dst)
{
    if (src1->size >= src2->size)
	return band(src1->digits, src1->sign, src1->size, 
		    src2->digits, src2->sign, src2->size,
		    dst);
    else
	return band(src2->digits, src2->sign, src2->size,
		    src1->digits, src1->sign, src1->size,
		    dst);
}

int cnif_big_bor(ErlNifBignum* src1, ErlNifBignum* src2, ErlNifBignum* dst)
{
    if (src1->size >= src2->size)
	return bor(src1->digits, src1->sign, src1->size, 
		   src2->digits, src2->sign, src2->size, 
		   dst);
    else
	return bor(src2->digits, src2->sign, src2->size,
		   src1->digits, src1->sign, src1->size, 
		   dst);
}

int cnif_big_bxor(ErlNifBignum* src1, ErlNifBignum* src2, ErlNifBignum* dst)
{
    if (src1->size >= src2->size)
	return bxor(src1->digits, src1->sign, src1->size, 
		    src2->digits, src2->sign, src2->size, 
		    dst);
    else
	return bxor(src2->digits, src2->sign, src2->size,
		    src1->digits, src1->sign, src1->size,
		    dst);
}

int cnif_big_bnot(ErlNifBignum* src, ErlNifBignum* dst)
{
    return bnot(src->digits, src->sign, src->size, dst);
}
