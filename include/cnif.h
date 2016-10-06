//
// C library to handle erlang terms using the nif api
//
#ifndef __CNIF_H__
#define __CNIF_H__

#include <stdlib.h>
#include <stdint.h>

typedef uint64_t ErlNifUInt64;
typedef int64_t  ErlNifSInt64;
typedef uintptr_t ERL_NIF_TERM;

struct enif_environment_t;
typedef struct enif_environment_t ErlNifEnv;

typedef ERL_NIF_TERM ERL_NIF_UINT;
typedef intptr_t     ERL_NIF_INT;

#define UNUSED __attribute__((unused))

#define ERL_NIF_API_FUNC_DECL(RET_TYPE, NAME, ARGS) extern RET_TYPE NAME ARGS

typedef enum
{
    ERL_NIF_LATIN1 = 1
} ErlNifCharEncoding;

typedef struct
{
    size_t size;
    uint8_t* data;

    /* Internals (avert your eyes) */
    ERL_NIF_TERM bin_term;
    void* ref_bin;
} ErlNifBinary;

typedef struct /* All fields all internal and may change */
{
    ERL_NIF_TERM map;
    ERL_NIF_UINT size;
    ERL_NIF_UINT idx;
} ErlNifMapIterator;

typedef enum {
    ERL_NIF_MAP_ITERATOR_FIRST = 1,
    ERL_NIF_MAP_ITERATOR_LAST = 2,

    /* deprecated synonyms (undocumented in 17 and 18-rc) */
    ERL_NIF_MAP_ITERATOR_HEAD = ERL_NIF_MAP_ITERATOR_FIRST,
    ERL_NIF_MAP_ITERATOR_TAIL = ERL_NIF_MAP_ITERATOR_LAST
} ErlNifMapIteratorEntry;

ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM*,cnif_heap_alloc,(ErlNifEnv*,size_t size));
ERL_NIF_API_FUNC_DECL(int,cnif_heap_begin,(ErlNifEnv*, void** mark));
ERL_NIF_API_FUNC_DECL(int,cnif_heap_rewind,(ErlNifEnv*, void** mark));
ERL_NIF_API_FUNC_DECL(int,cnif_heap_commit,(ErlNifEnv*, void** mark));

ERL_NIF_API_FUNC_DECL(ErlNifEnv*,enif_alloc_env,(void));
ERL_NIF_API_FUNC_DECL(void,enif_free_env,(ErlNifEnv* env));
ERL_NIF_API_FUNC_DECL(void,enif_clear_env,(ErlNifEnv* env));

ERL_NIF_API_FUNC_DECL(void*,enif_alloc,(size_t size));
ERL_NIF_API_FUNC_DECL(void,enif_free,(void* ptr));
ERL_NIF_API_FUNC_DECL(int,enif_is_atom,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_is_binary,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_is_ref,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_inspect_binary,(ErlNifEnv*, ERL_NIF_TERM bin_term, ErlNifBinary* bin));
ERL_NIF_API_FUNC_DECL(int,enif_alloc_binary,(size_t size, ErlNifBinary* bin));
ERL_NIF_API_FUNC_DECL(int,enif_realloc_binary,(ErlNifBinary* bin, size_t size));
ERL_NIF_API_FUNC_DECL(void,enif_release_binary,(ErlNifBinary* bin));
ERL_NIF_API_FUNC_DECL(int,enif_get_int,(ErlNifEnv*, ERL_NIF_TERM term, int* ip));
ERL_NIF_API_FUNC_DECL(int,enif_get_ulong,(ErlNifEnv*, ERL_NIF_TERM term, unsigned long* ip));
ERL_NIF_API_FUNC_DECL(int,enif_get_double,(ErlNifEnv*, ERL_NIF_TERM term, double* dp));
ERL_NIF_API_FUNC_DECL(int,enif_get_list_cell,(ErlNifEnv* env, ERL_NIF_TERM term, ERL_NIF_TERM* head, ERL_NIF_TERM* tail));
ERL_NIF_API_FUNC_DECL(int,enif_get_tuple,(ErlNifEnv* env, ERL_NIF_TERM tpl, int* arity, const ERL_NIF_TERM** array));
ERL_NIF_API_FUNC_DECL(int,enif_is_identical,(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs));
ERL_NIF_API_FUNC_DECL(int,enif_compare,(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_binary,(ErlNifEnv* env, ErlNifBinary* bin));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_badarg,(ErlNifEnv* env));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_int,(ErlNifEnv* env, int i));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_ulong,(ErlNifEnv* env, unsigned long i));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_double,(ErlNifEnv* env, double d));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_atom,(ErlNifEnv* env, const char* name));
ERL_NIF_API_FUNC_DECL(int,enif_make_existing_atom,(ErlNifEnv* env, const char* name, ERL_NIF_TERM* atom, ErlNifCharEncoding));

ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_tuple,(ErlNifEnv* env, unsigned cnt, ...));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_list,(ErlNifEnv* env, unsigned cnt, ...));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_list_cell,(ErlNifEnv* env, ERL_NIF_TERM car, ERL_NIF_TERM cdr));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_string,(ErlNifEnv* env, const char* string, ErlNifCharEncoding));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_ref,(ErlNifEnv* env));

ERL_NIF_API_FUNC_DECL(void*,enif_realloc,(void* ptr, size_t size));
ERL_NIF_API_FUNC_DECL(int,enif_inspect_iolist_as_binary,(ErlNifEnv*, ERL_NIF_TERM term, ErlNifBinary* bin));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_sub_binary,(ErlNifEnv*, ERL_NIF_TERM bin_term, size_t pos, size_t size));
ERL_NIF_API_FUNC_DECL(int,enif_get_string,(ErlNifEnv*, ERL_NIF_TERM list, char* buf, unsigned len, ErlNifCharEncoding));
ERL_NIF_API_FUNC_DECL(int,enif_get_atom,(ErlNifEnv*, ERL_NIF_TERM atom, char* buf, unsigned len, ErlNifCharEncoding));
ERL_NIF_API_FUNC_DECL(int,enif_is_fun,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_is_pid,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_is_port,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_get_uint,(ErlNifEnv*, ERL_NIF_TERM term, unsigned* ip));
ERL_NIF_API_FUNC_DECL(int,enif_get_long,(ErlNifEnv*, ERL_NIF_TERM term, long* ip));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_uint,(ErlNifEnv*, unsigned i));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_long,(ErlNifEnv*, long i));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_tuple_from_array,(ErlNifEnv*, const ERL_NIF_TERM arr[], unsigned cnt));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_list_from_array,(ErlNifEnv*, const ERL_NIF_TERM arr[], unsigned cnt));
ERL_NIF_API_FUNC_DECL(int,enif_is_empty_list,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(uint8_t*,enif_make_new_binary,(ErlNifEnv*,size_t size,ERL_NIF_TERM* termp));
ERL_NIF_API_FUNC_DECL(int,enif_is_list,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_is_tuple,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int,enif_get_atom_length,(ErlNifEnv*, ERL_NIF_TERM atom, unsigned* len, ErlNifCharEncoding));
ERL_NIF_API_FUNC_DECL(int,enif_get_list_length,(ErlNifEnv* env, ERL_NIF_TERM term, unsigned* len));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM, enif_make_atom_len,(ErlNifEnv* env, const char* name, size_t len));
ERL_NIF_API_FUNC_DECL(int, enif_make_existing_atom_len,(ErlNifEnv* env, const char* name, size_t len, ERL_NIF_TERM* atom, ErlNifCharEncoding));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_string_len,(ErlNifEnv* env, const char* string, size_t len, ErlNifCharEncoding));
#if SIZEOF_LONG != 8
ERL_NIF_API_FUNC_DECL(int,enif_get_int64,(ErlNifEnv*, ERL_NIF_TERM term, ErlNifSInt64* ip));
ERL_NIF_API_FUNC_DECL(int,enif_get_uint64,(ErlNifEnv*, ERL_NIF_TERM term, ErlNifUInt64* ip));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_int64,(ErlNifEnv*, ErlNifSInt64));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM,enif_make_uint64,(ErlNifEnv*, ErlNifUInt64));
#endif
ERL_NIF_API_FUNC_DECL(int,enif_make_reverse_list,(ErlNifEnv*, ERL_NIF_TERM term, ERL_NIF_TERM *list));
ERL_NIF_API_FUNC_DECL(int,enif_is_number,(ErlNifEnv*, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int, enif_is_map, (ErlNifEnv* env, ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(int, enif_get_map_size, (ErlNifEnv* env, ERL_NIF_TERM term, size_t *size));
ERL_NIF_API_FUNC_DECL(ERL_NIF_TERM, enif_make_new_map, (ErlNifEnv* env));
ERL_NIF_API_FUNC_DECL(int, enif_make_map_put, (ErlNifEnv* env, ERL_NIF_TERM map_in, ERL_NIF_TERM key, ERL_NIF_TERM value, ERL_NIF_TERM* map_out));
ERL_NIF_API_FUNC_DECL(int, enif_get_map_value, (ErlNifEnv* env, ERL_NIF_TERM map, ERL_NIF_TERM key, ERL_NIF_TERM* value));
ERL_NIF_API_FUNC_DECL(int, enif_make_map_update, (ErlNifEnv* env, ERL_NIF_TERM map_in, ERL_NIF_TERM key, ERL_NIF_TERM value, ERL_NIF_TERM* map_out));
ERL_NIF_API_FUNC_DECL(int, enif_make_map_remove, (ErlNifEnv* env, ERL_NIF_TERM map_in, ERL_NIF_TERM key, ERL_NIF_TERM* map_out));
ERL_NIF_API_FUNC_DECL(int, enif_map_iterator_create, (ErlNifEnv *env, ERL_NIF_TERM map, ErlNifMapIterator *iter, ErlNifMapIteratorEntry entry));
ERL_NIF_API_FUNC_DECL(void, enif_map_iterator_destroy, (ErlNifEnv *env, ErlNifMapIterator *iter));
ERL_NIF_API_FUNC_DECL(int, enif_map_iterator_is_head, (ErlNifEnv *env, ErlNifMapIterator *iter));
ERL_NIF_API_FUNC_DECL(int, enif_map_iterator_is_tail, (ErlNifEnv *env, ErlNifMapIterator *iter));
ERL_NIF_API_FUNC_DECL(int, enif_map_iterator_next, (ErlNifEnv *env, ErlNifMapIterator *iter));
ERL_NIF_API_FUNC_DECL(int, enif_map_iterator_prev, (ErlNifEnv *env, ErlNifMapIterator *iter));
ERL_NIF_API_FUNC_DECL(int, enif_map_iterator_get_pair, (ErlNifEnv *env, ErlNifMapIterator *iter, ERL_NIF_TERM *key, ERL_NIF_TERM *value));

#if defined(__GNUC__) && !(defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_))
#define ERL_NIF_INLINE __inline__
#else 
#define ERL_NIF_INLINE
#endif

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple1(ErlNifEnv* env,
						    ERL_NIF_TERM e1)
{
    return enif_make_tuple(env, 1, e1);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple2(ErlNifEnv* env,
						    ERL_NIF_TERM e1,
						    ERL_NIF_TERM e2)
{
    return enif_make_tuple(env, 2, e1, e2);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple3(ErlNifEnv* env,
						    ERL_NIF_TERM e1,
						    ERL_NIF_TERM e2,
						    ERL_NIF_TERM e3)
{
    return enif_make_tuple(env, 3, e1, e2, e3);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple4(ErlNifEnv* env,
						    ERL_NIF_TERM e1,
						    ERL_NIF_TERM e2,
						    ERL_NIF_TERM e3,
						    ERL_NIF_TERM e4)
{
    return enif_make_tuple(env, 4, e1, e2, e3, e4);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple5(ErlNifEnv* env,
						    ERL_NIF_TERM e1,
						    ERL_NIF_TERM e2,
						    ERL_NIF_TERM e3,
						    ERL_NIF_TERM e4,
						    ERL_NIF_TERM e5)
{
    return enif_make_tuple(env, 5, e1, e2, e3, e4, e5);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple6(ErlNifEnv* env,
						    ERL_NIF_TERM e1,
						    ERL_NIF_TERM e2,
						    ERL_NIF_TERM e3,
						    ERL_NIF_TERM e4,
						    ERL_NIF_TERM e5,
						    ERL_NIF_TERM e6)
{
    return enif_make_tuple(env, 6, e1, e2, e3, e4, e5, e6);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple7(ErlNifEnv* env,
						    ERL_NIF_TERM e1,
						    ERL_NIF_TERM e2,
						    ERL_NIF_TERM e3,
						    ERL_NIF_TERM e4,
						    ERL_NIF_TERM e5,
						    ERL_NIF_TERM e6,
						    ERL_NIF_TERM e7)
{
    return enif_make_tuple(env, 7, e1, e2, e3, e4, e5, e6, e7);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple8(ErlNifEnv* env,
						    ERL_NIF_TERM e1,
						    ERL_NIF_TERM e2,
						    ERL_NIF_TERM e3,
						    ERL_NIF_TERM e4,
						    ERL_NIF_TERM e5,
						    ERL_NIF_TERM e6,
						    ERL_NIF_TERM e7,
						    ERL_NIF_TERM e8)
{
    return enif_make_tuple(env, 8, e1, e2, e3, e4, e5, e6, e7, e8);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_tuple9(ErlNifEnv* env,
						    ERL_NIF_TERM e1,
						    ERL_NIF_TERM e2,
						    ERL_NIF_TERM e3,
						    ERL_NIF_TERM e4,
						    ERL_NIF_TERM e5,
						    ERL_NIF_TERM e6,
						    ERL_NIF_TERM e7,
						    ERL_NIF_TERM e8,
						    ERL_NIF_TERM e9)
{
    return enif_make_tuple(env, 9, e1, e2, e3, e4, e5, e6, e7, e8, e9);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list1(ErlNifEnv* env,
						   ERL_NIF_TERM e1)
{
    return enif_make_list(env, 1, e1);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list2(ErlNifEnv* env,
						   ERL_NIF_TERM e1,
						   ERL_NIF_TERM e2)
{
    return enif_make_list(env, 2, e1, e2);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list3(ErlNifEnv* env,
						   ERL_NIF_TERM e1,
						   ERL_NIF_TERM e2,
						   ERL_NIF_TERM e3)
{
    return enif_make_list(env, 3, e1, e2, e3);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list4(ErlNifEnv* env,
						   ERL_NIF_TERM e1,
						   ERL_NIF_TERM e2,
						   ERL_NIF_TERM e3,
						   ERL_NIF_TERM e4)
{
    return enif_make_list(env, 4, e1, e2, e3, e4);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list5(ErlNifEnv* env,
						   ERL_NIF_TERM e1,
						   ERL_NIF_TERM e2,
						   ERL_NIF_TERM e3,
						   ERL_NIF_TERM e4,
						   ERL_NIF_TERM e5)
{
    return enif_make_list(env, 5, e1, e2, e3, e4, e5);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list6(ErlNifEnv* env,
						   ERL_NIF_TERM e1,
						   ERL_NIF_TERM e2,
						   ERL_NIF_TERM e3,
						   ERL_NIF_TERM e4,
						   ERL_NIF_TERM e5,
						   ERL_NIF_TERM e6)
{
    return enif_make_list(env, 6, e1, e2, e3, e4, e5, e6);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list7(ErlNifEnv* env,
						   ERL_NIF_TERM e1,
						   ERL_NIF_TERM e2,
						   ERL_NIF_TERM e3,
						   ERL_NIF_TERM e4,
						   ERL_NIF_TERM e5,
						   ERL_NIF_TERM e6,
						   ERL_NIF_TERM e7)
{
    return enif_make_list(env, 7, e1, e2, e3, e4, e5, e6, e7);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list8(ErlNifEnv* env,
						   ERL_NIF_TERM e1,
						   ERL_NIF_TERM e2,
						   ERL_NIF_TERM e3,
						   ERL_NIF_TERM e4,
						   ERL_NIF_TERM e5,
						   ERL_NIF_TERM e6,
						   ERL_NIF_TERM e7,
						   ERL_NIF_TERM e8)
{
    return enif_make_list(env, 8, e1, e2, e3, e4, e5, e6, e7, e8);
}

static ERL_NIF_INLINE ERL_NIF_TERM enif_make_list9(ErlNifEnv* env,
						   ERL_NIF_TERM e1,
						   ERL_NIF_TERM e2,
						   ERL_NIF_TERM e3,
						   ERL_NIF_TERM e4,
						   ERL_NIF_TERM e5,
						   ERL_NIF_TERM e6,
						   ERL_NIF_TERM e7,
						   ERL_NIF_TERM e8,
						   ERL_NIF_TERM e9)
{
    return enif_make_list(env, 9, e1, e2, e3, e4, e5, e6, e7, e8, e9);
}

#endif
