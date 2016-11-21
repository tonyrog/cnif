#ifndef __CNIF_MISC_H__
#define __CNIF_MISC_H__

#include "cnif.h"

//  1,2,3     4     5     6     7      8      9     10     11    12      13
// number < atom < ref < fun < port < pid < tuple < map < nil < list < bitstring

typedef enum {
    ENIF_TYPE_INVALID = 0,
    ENIF_TYPE_INTEGER = 1,
    ENIF_TYPE_FLOAT   = 2,
    ENIF_TYPE_NUMBER  = 3,
    ENIF_TYPE_ATOM    = 4,
    ENIF_TYPE_REF     = 5,
    ENIF_TYPE_FUN     = 6,
    ENIF_TYPE_PORT    = 7,
    ENIF_TYPE_PID     = 8,
    ENIF_TYPE_TUPLE   = 9,
    ENIF_TYPE_MAP     = 10,
    ENIF_TYPE_NIL     = 11,
    ENIF_TYPE_LIST    = 12,
    ENIF_TYPE_BINARY  = 13
} enif_type_t;


static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple0(ErlNifEnv* env)
{
    return enif_make_tuple(env, 0);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list0(ErlNifEnv* env)
{
    return enif_make_list(env, 0);
}

ERL_NIF_API_FUNC_DECL(int,enif_is_float,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_is_integer,(ErlNifEnv*, ERL_NIF_TERM term));

ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_map_from_arrays,(ErlNifEnv* env,ERL_NIF_TERM keys[],ERL_NIF_TERM value[],unsigned cnt));

ERL_NIF_API_FUNC_DECL(enif_type_t,enif_get_type,(ERL_NIF_TERM term, int number));
ERL_NIF_API_FUNC_DECL(int,enif_iolist_size,(ErlNifEnv* env, ERL_NIF_TERM term, size_t* len));
ERL_NIF_API_FUNC_DECL(int,enif_byte_size,(ErlNifEnv* env, ERL_NIF_TERM term, size_t* len));
ERL_NIF_API_FUNC_DECL(int,enif_inline_reverse_list,(ErlNifEnv*, ERL_NIF_TERM term, ERL_NIF_TERM tail, ERL_NIF_TERM *list));

ERL_NIF_API_FUNC_DECL(int, enif_each_element,
		      (ErlNifEnv* env,
		       void (*fun)(ErlNifEnv* env,int i,ERL_NIF_TERM value,void* arg),
		      ERL_NIF_TERM term,
		       void* arg));

ERL_NIF_API_FUNC_DECL(ERL_NIF_UINT,enif_flat_size,(ERL_NIF_TERM src_term));

ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_flat_copy,(ErlNifEnv* dst_env, ERL_NIF_TERM src_term));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_struct_copy,(ErlNifEnv* dst_env, ERL_NIF_TERM src_term));

#endif
