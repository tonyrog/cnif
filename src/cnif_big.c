
#include "../include/cnif_term.h"
#include "../include/cnif_big.h"
#include "../include/cnif_misc.h"

////////////////////////////////////////////////////////////////////////////////
// BIGNUM
////////////////////////////////////////////////////////////////////////////////

int enif_is_big(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return IS_BIGNUM(term);
}

int enif_inspect_big(ErlNifEnv* env,ERL_NIF_TERM term,ErlNifBignum* big)
{
    if (IS_BOXED(term)) {
	ERL_NIF_TERM* ptr = GET_BOXED(term);
	if (IS_BIGVAL(ptr[0])) {
	    big->sign   = *ptr & _BIG_SIGN_BIT;
	    big->size   = GET_ARITYVAL(*ptr);
	    big->asize  = 0;
	    big->digits = ptr + 1;
	    return 1;
	}
    }
    return 0;
}

// Create a new number (big or small)
ERL_NIF_TERM enif_make_number(ErlNifEnv* env, ErlNifBignum* big)
{
    ERL_NIF_TERM t;
    ERL_NIF_TERM* ptr;
    size_t size;
    int i;

    size = cnif_big_trail(big->digits, big->size);
    if (size == 1) {
	// use make_uint64 / int64 to cover "all" cases
	if (!big->sign)
	    return enif_make_uint64(env, big->digits[0]);
	else if (!(big->digits[0] >> (DIGIT_BITS - 1))) {
	    ErlNifSInt64 d = (ErlNifSInt64) big->digits[0];
	    return enif_make_int64(env, -d);
	}
    }
    ptr = cnif_heap_alloc(env, 1+size);
    ptr[0] = big->sign ? MAKE_NEG_BIGVAL(size) : MAKE_POS_BIGVAL(size);
    for (i = 0; i < (int) size; i++)
	ptr[i+1] = big->digits[i];
    return MAKE_BIGNUM(ptr);
}

// Load numer, big or small as ErlNigBignum
int enif_get_number(ErlNifEnv* env, ERL_NIF_TERM t, ErlNifBignum* big)
{
    if (enif_inspect_big(env, t, big))
	return 1;
    else {
	ErlNifSInt64 digit;
	if (enif_get_int64(env, t, &digit)) {
	    big->size = 1;
	    big->asize = 0;
	    if (digit < 0) {
		big->sign = 1;
		big->ds[0] = -digit;
	    }
	    else {
		big->sign = 0;
		big->ds[0] = digit;
	    }
	    big->digits = big->ds;
	    return 1;
	}
    }
    return 0;
}

// Allocate a big num and initiate digit to zero
int enif_alloc_number(ErlNifEnv* env, ErlNifBignum* big, size_t n)
{
    ErlNifBigDigit* digits;
    int i;

    if (n <= NUM_TMP_DIGITS)
	digits = &big->ds[0];
    else
	digits = (ErlNifBigDigit*) enif_alloc(sizeof(ErlNifBigDigit)*n);
    if (!digits)
	return 0;
    for (i = 0; i < n; i++)
	digits[i] = 0;
    big->size   = n;
    big->asize  = n;
    big->sign   = 0;
    big->digits = digits;
    return 1;
}

// Copy a bignum to a "safe" location, allow modifications of big num digits
int enif_copy_number(ErlNifEnv* env, ErlNifBignum* big, size_t min_size)
{
    size_t n;
    size_t an;
    ErlNifBigDigit* digits;
    int i;

    n = (min_size > big->size) ? min_size : big->size;
    if (n <= NUM_TMP_DIGITS) {
	an = NUM_TMP_DIGITS;
	digits = &big->ds[0];
    }
    else {
	an = n;
	if (!(digits = (ErlNifBigDigit*) enif_alloc(sizeof(ErlNifBigDigit)*n)))
	    return 0;
    }
    i = 0;
    while(i < big->size) {
	digits[i] = big->digits[i];
	i++;
    }
    while(i < an) {
	digits[i] = 0;
	i++;
    }
    big->asize  = an;
    big->digits = digits;
    return 1;
}

//  Release temporary memory associated with ErlNifBignum
void enif_release_number(ErlNifEnv* env, ErlNifBignum* big)
{
    if (big->asize && (big->digits != big->ds))
	enif_free(big->digits);
}

// Get and copy
int enif_get_copy_number(ErlNifEnv* env, ERL_NIF_TERM t, ErlNifBignum* big,
			 size_t min_size)
{
    if (!enif_get_number(env, t, big))
	return 0;
    return enif_copy_number(env, big, min_size);
}

void cnif_big_write(enif_io_t* iop, ErlNifBignum* src)
{
    int i;
    if (src->sign)
	enif_io_format(iop,"-");
    enif_io_format(iop,"16#[");
    for (i = 0; i < src->size; i++) {
	enif_io_format(iop,"%016lx", src->digits[i]);
    }
    enif_io_format(iop,"]");
}
