//
// The "missing" api 
//

#include "../include/cnif.h"
#include "../include/cnif_term.h"
#include "../include/cnif_sort.h"
#include "../include/cnif_misc.h"

int enif_is_float(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return IS_FLOAT(term);
}

int enif_is_integer(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return IS_SMALL(term) || IS_BIGNUM(term);
}

ERL_NIF_TERM enif_make_map_from_arrays(ErlNifEnv* env,
				       ERL_NIF_TERM key[],
				       ERL_NIF_TERM value[],
				       unsigned int cnt)
{
    int i;
    size_t n = NWORDS(sizeof(flatmap_t));
    flatmap_t* mp;

    if (!cnif_is_usorted(key, cnt))
	cnt = cnif_quick_usort_aux(key, value, key, value, 0, cnt-1);
    mp =  (flatmap_t*) cnif_heap_alloc(env, n+cnt);
    mp->header = MAKE_MAPVAL(n+cnt-1);
    mp->size = cnt;
    mp->keys = enif_make_tuple_from_array(env, key, cnt);
    for (i = 0; i < cnt; i++)
	mp->value[i] = value[i];
    return MAKE_MAP(mp);
}

static int binary_byte_size(ERL_NIF_TERM term, size_t* sizep)
{
    if (IS_BINARY(term)) {
	ERL_NIF_TERM* ptr = GET_BINARY(term);
	if (IS_SUB_BIN(ptr[0]))
	    *sizep = ((sub_binary_t*) ptr)->size;
	else if (IS_HEAP_BIN(ptr[0]))
	    *sizep = ((heap_binary_t*) ptr)->size;
	else
	    *sizep = ((refc_binary_t*) ptr)->size;
	return 1;
    }
    return 0;
}

int enif_byte_size(ErlNifEnv* env, ERL_NIF_TERM term, size_t* sizep)
{
    return binary_byte_size(term, sizep);
}

// return a type code
// xxxx00  (INVALID)
// xxxx01  LIST
// xxxx10  (BOXED)
// xxxx11  (IMMED1)
//
// xx00|11  PID
// xx01|11  PORT
// xx10|11  (IMMED2)
// 0010|11  ATOM
// 0110|11  (catch)
// 1010|11  (?????)
// 1110|11  NIL
// xx11|11  INTEGER (small)
//
// boxed patterns
// 0000|00  TUPLE  arity value
// 0001|00  INVALID  (match state)
// 0010|00  INTEGER  (big+)
// 0011|00  INTEGER  (big-)
// 0100|00  REF
// 0101|00  FUN
// 0110|00  FLOAT
// 0111|00  FUN (export)
// 1000|00  BINARY (refc)
// 1001|00  BINARY (heap)
// 1010|00  BINARY (sub)
// 1011|00  INVALID
// 1100|00  PID (external)
// 1101|00  PORT (external)
// 1110|00  REF (external)
// 1111|00  MAP
//
static const enif_type_t immed_type[16] =
{
    [0b0000] = ENIF_TYPE_PID,
    [0b0100] = ENIF_TYPE_PID,
    [0b1000] = ENIF_TYPE_PID,
    [0b1100] = ENIF_TYPE_PID,
    [0b0001] = ENIF_TYPE_PORT,
    [0b0101] = ENIF_TYPE_PORT,
    [0b1001] = ENIF_TYPE_PORT,
    [0b1101] = ENIF_TYPE_PORT,
    [0b0010] = ENIF_TYPE_ATOM,
    [0b0110] = ENIF_TYPE_INVALID,  // catch
    [0b1010] = ENIF_TYPE_INVALID,  // not used
    [0b1110] = ENIF_TYPE_NIL,
    [0b0011] = ENIF_TYPE_INTEGER,
    [0b0111] = ENIF_TYPE_INTEGER,
    [0b1011] = ENIF_TYPE_INTEGER,
    [0b1111] = ENIF_TYPE_INTEGER
};

static const enif_type_t boxed_type[16] =
{
    [0] = ENIF_TYPE_TUPLE,
    [1] = ENIF_TYPE_INVALID,
    [2] = ENIF_TYPE_INTEGER,
    [3] = ENIF_TYPE_INTEGER,
    [4] = ENIF_TYPE_REF,
    [5] = ENIF_TYPE_FUN,
    [6] = ENIF_TYPE_FLOAT,
    [7] = ENIF_TYPE_FUN,
    [8] = ENIF_TYPE_BINARY,
    [9] = ENIF_TYPE_BINARY,
    [10] = ENIF_TYPE_BINARY,
    [11] = ENIF_TYPE_INVALID,
    [12] = ENIF_TYPE_PID,
    [13] = ENIF_TYPE_PORT,
    [14] = ENIF_TYPE_REF,
    [15] = ENIF_TYPE_MAP
};

enif_type_t enif_get_type(ERL_NIF_TERM term, int number)
{
    switch(term & 0x3) {
    case TAG_PRIMARY_HEADER:
	return ENIF_TYPE_INVALID;
    case TAG_PRIMARY_LIST:
	return ENIF_TYPE_LIST;
    case TAG_PRIMARY_BOXED:
	return boxed_type[(*GET_BOXED(term) >> 2) & 0xf];
    case TAG_PRIMARY_IMMED1:
	return immed_type[(term >> 2) & 0xf];
    }
    return ENIF_TYPE_INVALID;
}

static int iolist_size(ERL_NIF_TERM term, size_t* sizep)
{
    size_t n = 0;

    while (IS_LIST(term)) {
    	ERL_NIF_TERM* ptr = GET_LIST(term);
	if (IS_SMALL(ptr[0])) {
	    if ((ptr[0] >> TAG_IMMED1_SIZE) > 255)
		return 0;
	    n++;
	}
	else if (IS_LIST(ptr[0])) {
	    size_t m;
	    if (!iolist_size(ptr[0],&m))
		return 0;
	    n += m;
	}
	else if (IS_BINARY(ptr[0])) {
	    size_t m;
	    if (!binary_byte_size(ptr[0], &m))
		return 0;
	    n += m;
	}
	else
	    return 0;
	term = ptr[1];
    }
    if (IS_BINARY(term)) {
	size_t m;
	if (!binary_byte_size(term, &m))
	    *sizep = n + m;
	return 1;
    }
    else if (term == MAKE_NIL) {
	*sizep = n;
	return 1;
    }
    return 0;
}

int enif_iolist_size(ErlNifEnv* env, ERL_NIF_TERM term, size_t* sizep)
{
    return iolist_size(term, sizep);
}

int enif_inline_reverse_list(ErlNifEnv* env, ERL_NIF_TERM term,
			     ERL_NIF_TERM tail, ERL_NIF_TERM *list)
{
    while(IS_LIST(term)) {
	ERL_NIF_TERM* ptr = GET_LIST(term);
	ERL_NIF_TERM  tmp = term;
	term = ptr[1];
	ptr[1] = tail;
	tail = tmp;
    }
    *list = tail;
    return 1;
}

// each tuple/list

int enif_each_element(ErlNifEnv* env,
		      void (*fun)(ErlNifEnv* env,int i,ERL_NIF_TERM value,void* arg),
		      ERL_NIF_TERM term,
		      void* arg)
{
    if (IS_LIST(term)) {
	int i = 0;
	while(IS_LIST(term)) {
	    ERL_NIF_TERM* ptr = GET_LIST(term);
	    i++;
	    fun(env, i, ptr[0], arg);
	    term = ptr[1];
	}
	return i;
    }
    else if (IS_TUPLE(term)) {
	ERL_NIF_TERM* ptr = GET_BOXED(term);
	int n = GET_ARITYVAL(ptr[0]);
	int i;
	for (i = 1; i <= n; i++)
	    fun(env, i, ptr[i], arg);
	return n;
    }
    return 0;
}
