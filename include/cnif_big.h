#ifndef __CNIF_BIG_H__
#define __CNIF_BIG_H__

#include "cnif.h"

typedef ERL_NIF_TERM ErlNifBigDigit;
typedef unsigned long long ErlNifBigDoubleDigit;

#define NUM_TMP_DIGITS 4
#define DIGIT_BITS (sizeof(ErlNifBigDigit)*8)

typedef struct 
{
    unsigned size;          // number of digits 
    unsigned sign;          // 1= negative, 0=none-negative
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

#endif
