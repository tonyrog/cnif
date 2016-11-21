#ifndef __CNIF_BIG_H__
#define __CNIF_BIG_H__

#include "cnif.h"
#include "cnif_io.h"

typedef ERL_NIF_TERM ErlNifBigDigit;
typedef unsigned long long ErlNifBigDoubleDigit;

#define NUM_TMP_DIGITS 4
#define DIGIT_BITS (sizeof(ErlNifBigDigit)*8)
#define D_EXP      (sizeof(ERL_NIF_TERM)*8)
#define D_MASK     ((ErlNifBigDigit)(-1))
#define DCONST(n) ((ErlNifBigDigit)(n))

typedef struct
{
    ERL_NIF_UINT size;       // number of digits 
    ERL_NIF_UINT sign;       // 1= negative, 0=none-negative
    ERL_NIF_UINT asize;      // allocated size (>=size) or 0 if read only
    ErlNifBigDigit* digits;  // least significant digit first D0 D1 .. Dsize-1
    ErlNifBigDigit  ds[NUM_TMP_DIGITS];
} ErlNifBignum;

// bignum extension to enif api

ERL_NIF_API_FUNC_DECL(int,enif_is_big,(ErlNifEnv* env,ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_inspect_big,(ErlNifEnv* env,ERL_NIF_TERM term,ErlNifBignum* big));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM, enif_make_number, (ErlNifEnv* env, ErlNifBignum* big));
ERL_NIF_API_FUNC_DECL(int,enif_get_number,(ErlNifEnv* env, ERL_NIF_TERM t, ErlNifBignum* big));
ERL_NIF_API_FUNC_DECL(int,enif_copy_number,(ErlNifEnv* env, ErlNifBignum* big, size_t min_size));
ERL_NIF_API_FUNC_DECL(int,enif_alloc_number,(ErlNifEnv* env, ErlNifBignum* big, size_t n));
ERL_NIF_API_FUNC_DECL(void,enif_release_number,(ErlNifEnv* env, ErlNifBignum* big));
ERL_NIF_API_FUNC_DECL(int,enif_get_copy_number,(ErlNifEnv* env, ERL_NIF_TERM t,
						ErlNifBignum* big,size_t min_size));

// arithmentic extension
ERL_NIF_API_FUNC_DECL(int,cnif_big_add,(ErlNifBignum* src1,ErlNifBignum* src2,
					ErlNifBignum* dst));
ERL_NIF_API_FUNC_DECL(int,cnif_big_sub,(ErlNifBignum* src1,ErlNifBignum* src2,
					ErlNifBignum* dst));
ERL_NIF_API_FUNC_DECL(int,cnif_big_neg,(ErlNifBignum* src,ErlNifBignum* dst));

ERL_NIF_API_FUNC_DECL(int,cnif_big_band,(ErlNifBignum* src1,ErlNifBignum* src2,
					ErlNifBignum* dst));
ERL_NIF_API_FUNC_DECL(int,cnif_big_bor,(ErlNifBignum* src1,ErlNifBignum* src2,
					ErlNifBignum* dst));
ERL_NIF_API_FUNC_DECL(int,cnif_big_bxor,(ErlNifBignum* src1,ErlNifBignum* src2,
					 ErlNifBignum* dst));
ERL_NIF_API_FUNC_DECL(int,cnif_big_bnot,(ErlNifBignum* src,ErlNifBignum* dst));

ERL_NIF_API_FUNC_DECL(void,cnif_big_write,(enif_io_t* iop, ErlNifBignum* src));

static inline int cnif_big_is_zero(ErlNifBignum* bn)
{
    return (bn->size==1) && (bn->digits[0]==0);
}

static inline ERL_NIF_UINT cnif_big_trail(ErlNifBigDigit* src, ERL_NIF_UINT n)
{
    while((n>1) && !src[n-1])
	n--;
    return n;
}

#endif
