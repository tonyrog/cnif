//
//  C library for representing erlang terms using NIF api
//
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <limits.h>

#include "../include/cnif.h"
#include "../include/cnif_big.h"
#include "../include/cnif_lhash.h"
#include "../include/cnif_term.h"
#include "../include/cnif_sort.h"
#include "../include/cnif_misc.h"

#define DBG(...) printf(__VA_ARGS__)

#define DEFAULT_FRAGMENT_SIZE  1024

typedef struct _fragment_t {
    size_t size;
    struct _fragment_t* prev;
    ERL_NIF_TERM data[];
} fragment_t;

typedef struct _cnif_heap_mark_t
{
    fragment_t* frag;
    ERL_NIF_TERM* top;
} cnif_heap_mark_t;    

struct enif_environment_t
{
    fragment_t* first;
    fragment_t* last;
    ERL_NIF_TERM* top;    // into last moving backwards
};

static lhash_value_t atom_hash(void* a);
static int atom_cmp(void* a, void* b);
static void* atom_alloc(void* a);
static void atom_free(void* a);

lhash_t* global_atoms = NULL;

static const lhash_methods_t atom_funcs =
{
    atom_hash,
    atom_cmp,
    atom_alloc,
    atom_free
};

ERL_NIF_TERM* cnif_heap_alloc(ErlNifEnv* env, size_t n)
{
    if ((env->top == NULL) || ((env->top - env->last->data) < n)) {
	size_t sz = (n < DEFAULT_FRAGMENT_SIZE) ? DEFAULT_FRAGMENT_SIZE : (n << 1);
	size_t memsz = sizeof(fragment_t) + sizeof(ERL_NIF_TERM)*sz;
	fragment_t* fp = malloc(memsz);

	DBG("new framgent %p size %zu allocated\n", fp, sz);
	fp->size = sz;
	fp->prev = env->last;
	env->last = fp;
	env->top = &env->last->data[sz];
    }
    env->top -= n;
    // DBG("heap alloc %zu, top = %p\n", n, env->top);
    return env->top;
}

int cnif_heap_begin(ErlNifEnv* env, void** mark)
{
    cnif_heap_mark_t* mp = enif_alloc(sizeof(cnif_heap_mark_t));
    mp->frag = env->last;
    mp->top  = env->top;
    *mark = mp;
    return 1;
}

int cnif_heap_rewind(ErlNifEnv* env, void** mark)
{
    cnif_heap_mark_t* mp = (cnif_heap_mark_t*) mark;
    fragment_t* fp = env->last;

    while(fp && (fp != mp->frag)) {
	fragment_t* fpp = fp->prev;
	enif_free(fp);
	fp = fpp;
    }
    if ((env->last = fp) == NULL)
	env->first = NULL;
    env->top = mp->top;
    enif_free(mp);
    return 1;
}    

int cnif_heap_commit(ErlNifEnv* env, void** mark)
{
    cnif_heap_mark_t* mp = (cnif_heap_mark_t*) mark;
    enif_free(mp);
    return 1;
}


void* enif_alloc(size_t size)
{
    return malloc(size);
}

void* enif_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

void enif_free(void* ptr)
{
    free(ptr);
}

///////////////////////////////////////////////////////////////////////////////
// ENV
///////////////////////////////////////////////////////////////////////////////

ErlNifEnv* enif_alloc_env(void)
{
    ErlNifEnv* env = enif_alloc(sizeof(struct enif_environment_t));
    if (env)
	memset(env, 0, sizeof(struct enif_environment_t));
    if (!global_atoms) {
	global_atoms = lhash_new("atoms", 3, &atom_funcs);
    }
    return env;
}

void enif_clear_env(ErlNifEnv* env)
{
    fragment_t* p = env->last;
    while(p) {
	fragment_t* pn = p->prev;
	enif_free(p);
	p = pn;
    }
    env->first = NULL;
    env->last = NULL;
    env->top = NULL;
}

void enif_free_env(ErlNifEnv* env)
{
    enif_clear_env(env);
    free(env);
}

///////////////////////////////////////////////////////////////////////////////
// ENIF
///////////////////////////////////////////////////////////////////////////////

ERL_NIF_TERM enif_make_badarg(ErlNifEnv* env)
{
    return INVALID_TERM;
}

///////////////////////////////////////////////////////////////////////////////
// INTEGER
///////////////////////////////////////////////////////////////////////////////

ERL_NIF_TERM enif_make_uint64(ErlNifEnv* env, uint64_t u)
{
#if WORDSIZE == 32
    if (u < (UINT64_C(1) << 27)) {
	ERL_NIF_TERM cell = u & UINT32_C(0x0fffffff);
	return (cell << TAG_IMMED1_SIZE) | TAG_IMMED1_SMALL;
    }
    else {
	ERL_NIF_TERM* ptr;
	int ari = (u <= UINT64(0xffffffff)) ? 1 : 2;
	ptr = cnif_heap_alloc(env, 1+ari);
	ptr[0] = MAKE_POS_BIGVAL(ari);
	if (arg == 2)
	    ptr[2] = (u >> 32);
	ptr[1] = u;
	return MAKE_BIGNUM(ptr);
    }
#elif WORDSIZE == 64
    if (u < (UINT64_C(1) << 59)) {
	ERL_NIF_TERM cell = u & UINT64_C(0x0fffffffffffffff);
	return (cell << TAG_IMMED1_SIZE) | TAG_IMMED1_SMALL;
    }
    else {
	ERL_NIF_TERM* ptr;
	int ari = 1;
	ptr = cnif_heap_alloc(env, 1+ari);
	ptr[0] = MAKE_POS_BIGVAL(ari);
	ptr[1] = u;
	return MAKE_BIGNUM(ptr);
    }
#endif
    return 0;
}

ERL_NIF_TERM enif_make_ulong(ErlNifEnv* env, unsigned long u)
{
    return enif_make_uint64(env, (uint64_t) u);
}

ERL_NIF_TERM enif_make_uint(ErlNifEnv* env, unsigned u)
{
    return enif_make_uint64(env, (uint64_t) u);
}

ERL_NIF_TERM enif_make_int64(ErlNifEnv* env, int64_t i)
{
#if WORDSIZE == 32
    if ((i >= -(UINT64_C(1) << 27)) || (i < (UINT64_C(1) << 27))) {
	ERL_NIF_TERM  cell = i & UINT32_C(0x0fffffff);
	return (cell << TAG_IMMED1_SIZE) | TAG_IMMED1_SMALL;
    }
    else {
	ERL_NIF_TERM* ptr;
	int sign = (i < 0) ? 1 : 0;
	uint64_t u = (i < 0) ? -i : i;
	int ari = (u <= UINT64(0xffffffff)) ? 1 : 2;
	ptr = cnif_heap_alloc(env, 1+ari);
	ptr[0] = sign ? MAKE_NEG_BIGVAL(ari) : MAKE_POS_BIGVAL(ari);
	if (arg == 2)
	    ptr[2] = (u >> 32);
	ptr[1] = u;
	return MAKE_BIGNUM(ptr);
    }
#elif WORDSIZE == 64
    if ((i >= -(UINT64_C(1) << 59)) && (i < (UINT64_C(1) << 59))) {
	ERL_NIF_TERM  cell = i & UINT64_C(0x0fffffffffffffff);
	return (cell << TAG_IMMED1_SIZE) | TAG_IMMED1_SMALL;
    }
    else {
	ERL_NIF_TERM* ptr;
	int sign = (i < 0) ? 1 : 0;
	uint64_t u = (i < 0) ? -i : i;
	int ari = 1;
	ptr = cnif_heap_alloc(env, 1+ari);
	ptr[0] = sign ? MAKE_NEG_BIGVAL(ari) : MAKE_POS_BIGVAL(ari);
	ptr[1] = u;
	return MAKE_BIGNUM(ptr);
    }
#endif
}

ERL_NIF_TERM enif_make_int(ErlNifEnv* env, int i)
{
    return enif_make_int64(env, (int64_t) i);
}

ERL_NIF_TERM enif_make_long(ErlNifEnv* env, long i)
{
    return enif_make_int64(env, (int64_t) i);
}

int enif_get_uint64(ErlNifEnv* env, ERL_NIF_TERM term, ErlNifUInt64* up)
{
    if (IS_SMALL(term)) {
	if (term >> (WORDSIZE-1)) // negative
	    return 0;
	*up = (term >> TAG_IMMED1_SIZE);
	return 1;
    }
    else if (IS_BIGNUM(term)) {
	ERL_NIF_TERM* ptr = GET_PTR(term);
	int ari;
	ErlNifUInt64 u;
	if (IS_NEG_BIGVAL(ptr[0]))
	    return 0;
	ari = GET_ARITYVAL(ptr[0]);
#if WORDSIZE == 32
	if (ari > 2) return 0;
	if (ari == 1)
	    u = ptr[1];
	else {
	    u = ptr[2];
	    u = (u << 32) | ptr[1];
	}
#elif WORDSIZE == 64
	if (ari > 1) return 0;
	u = ptr[1];
#endif
	*up = u;
	return 1;
    }
    return 0;
}

static int get_int64(ERL_NIF_TERM term, ErlNifSInt64* ip)
{
    if (IS_SMALL(term)) {
	*ip = ((ERL_NIF_INT) term >> TAG_IMMED1_SIZE);
	return 1;
    }
    else if (IS_BIGNUM(term)) {
	ERL_NIF_TERM* ptr = GET_PTR(term);
	int ari = GET_ARITYVAL(ptr[0]);
	ErlNifUInt64 u;
#if WORDSIZE == 32
	if (ari > 2) return 0;
	if (ari == 1)
	    u = ptr[1];
	else {
	    u = ptr[2];
	    u = (u << 32) | ptr[1];
	}
#elif WORDSIZE == 64
	if (ari > 1) return 0;
	u = ptr[1];
#endif
	if (IS_NEG_BIGVAL(ptr[0])) {
	    if ((u >> 63) && ((u << 1)!=0)) return 0;
	    *ip = -u;
	}
	else {
	    if (u >> 63) return 0;
	    *ip = u;
	}
	return 1;
    }
    return 0;
}

int enif_get_int64(ErlNifEnv* env, ERL_NIF_TERM term, ErlNifSInt64* ip)
{
    return get_int64(term, ip);
}

int enif_get_int(ErlNifEnv* env, ERL_NIF_TERM term, int* ip)
{
    ErlNifSInt64 i;

    if (enif_get_int64(env, term, &i)) {
	if (i > INT_MAX) return 0;
	if (i < INT_MIN) return 0;
	*ip = i;
	return 1;
    }
    return 0;
}

int enif_get_long(ErlNifEnv* env, ERL_NIF_TERM term, long* ip)
{
    ErlNifSInt64 i;

    if (enif_get_int64(env, term, &i)) {
	if (i > LONG_MAX) return 0;
	if (i < LONG_MIN) return 0;
	*ip = i;
	return 1;
    }
    return 0;
}

int enif_get_uint(ErlNifEnv* env, ERL_NIF_TERM term, unsigned* ip)
{
    ErlNifUInt64 u;
    if (enif_get_uint64(env, term, &u)) {
	if (u > UINT_MAX) return 0;
	*ip = u;
	return 1;
    }
    return 0;
}

int enif_get_ulong(ErlNifEnv* env, ERL_NIF_TERM term, unsigned long* ip)
{
    ErlNifUInt64 u;
    if (enif_get_uint64(env, term, &u)) {
	if (u > ULONG_MAX) return 0;
	*ip = u;
	return 1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// FLOAT
///////////////////////////////////////////////////////////////////////////////


static int get_double(ERL_NIF_TERM term, double* dp)
{
    if (IS_FLOAT(term)) {
	ERL_NIF_TERM* ptr = GET_FLOAT(term);
	int arity = sizeof(double)/sizeof(ERL_NIF_TERM);
	if (GET_ARITYVAL(ptr[0]) == arity) {
	    *dp = *((double*) (ptr+1));
	    return 1;
	}
    }
    return 0;
}

int enif_get_double(ErlNifEnv* env, ERL_NIF_TERM term, double* dp)
{
    return get_double(term, dp);
}

ERL_NIF_TERM enif_make_double(ErlNifEnv* env, double d)
{
    ERL_NIF_TERM cell;
    int arity = sizeof(double)/sizeof(ERL_NIF_TERM);
    ERL_NIF_TERM* ptr = cnif_heap_alloc(env, 1+arity);

    ptr[0] = MAKE_FLOATVAL(arity);
    *((double*)(ptr+1)) = d;
    return MAKE_FLOAT(ptr);
}

int enif_is_number(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return (IS_SMALL(term) || IS_BIGNUM(term) || IS_FLOAT(term));
}

////////////////////////////////////////////////////////////////////////////////
// ATOM()
////////////////////////////////////////////////////////////////////////////////

static lhash_value_t inline hash_u8(u_int8_t value, lhash_value_t h)
{
    lhash_value_t g;
    h = (h << 4) + value;
    if ((g = h & 0xf0000000)) {
	h ^= (g >> 24);
	h ^= g;
    }
    return h;
}

static lhash_value_t hash_u32(u_int32_t value, lhash_value_t h)
{
    h = hash_u8((value >> 24) & 0xff, h);
    h = hash_u8((value >> 16) & 0xff, h);
    h = hash_u8((value >> 8)  & 0xff, h);
    return hash_u8(value & 0xff, h);
}

static lhash_value_t memhash(void* ptr, size_t len, lhash_value_t h)
{
    uint8_t* uptr = (uint8_t*) ptr;
    while(len--)
	h = hash_u8(*uptr++, h);
    return h;
}


static lhash_value_t atom_hash(void* a)
{
    u_int8_t* ptr  = (uint8_t*) ((atom_t*)a)->name;
    size_t len =  ((atom_t*)a)->len;
    lhash_value_t h = 0, g;

    while(len--) {
        h = (h << 4) + *ptr++;
        if ((g = h & 0xf0000000)) {
            h ^= (g >> 24);
            h ^= g;
        }
    }
    return h;
}

static int atom_cmp(void* a, void* b)
{
    size_t len_a = ((atom_t*)a)->len;
    size_t len_b = ((atom_t*)b)->len;
    if (len_a  == len_b) {
	return memcmp(((atom_t*)a)->name,
		      ((atom_t*)b)->name,
		      len_a);
    }
    else if (len_a > len_b)
	return 1;
    else
	return -1;
}

static void* atom_alloc(void* a)
{
    atom_t* aptr = (atom_t*) a;
    atom_t* bptr;
    void*   vptr;
    if (posix_memalign(&vptr, (1 << TAG_IMMED2_SIZE), sizeof(atom_t)+aptr->len+1))
	return NULL;
    bptr = vptr;
    bptr->len = aptr->len;
    memcpy(bptr->data, aptr->name, aptr->len);
    bptr->name = bptr->data;
    bptr->name[aptr->len] = '\0';  // simplify printing
    return bptr;
}

static void atom_free(void* a)
{
    free(a);
}

int enif_make_existing_atom_len(ErlNifEnv* env, const char* name, size_t len,
				ERL_NIF_TERM* atom, ErlNifCharEncoding code)
{
    atom_t templ;
    void* aptr;

    templ.len = len;
    templ.name = (char*) name;
    if (!(aptr = lhash_get(global_atoms, &templ)))
	return 0;
    *atom = MAKE_ATOM(aptr);
    return 1;
}

ERL_NIF_TERM enif_make_atom_len(ErlNifEnv* env, const char* name, size_t len)
{
    atom_t templ;
    void* aptr;

    templ.len = len;
    templ.name = (char*) name;
    aptr = lhash_put(global_atoms, &templ);
    return MAKE_ATOM(aptr);
}

int enif_make_existing_atom(ErlNifEnv* env, const char* name, ERL_NIF_TERM* atom, ErlNifCharEncoding code)
{
    return enif_make_existing_atom_len(env, name, strlen(name), atom, code);
}

ERL_NIF_TERM enif_make_atom(ErlNifEnv* env, const char* name)
{
    return enif_make_atom_len(env, name, strlen(name));
}

int enif_is_atom(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return IS_ATOM(term);
}

int enif_get_atom(ErlNifEnv* env, ERL_NIF_TERM atom, char* buf, unsigned len, ErlNifCharEncoding code)
{
    atom_t templ;
    atom_t* aptr;

    if (!IS_ATOM(atom))
	return 0;
    aptr = GET_ATOM(atom);
    if (aptr->len > len-1)
	return 0;
    memcpy(buf, aptr->name, aptr->len);
    buf[aptr->len] = '\0';
    return aptr->len+1;
}

int enif_get_atom_length(ErlNifEnv* env, ERL_NIF_TERM atom, unsigned* len, ErlNifCharEncoding code)
{
    atom_t* aptr;
    if (!IS_ATOM(atom))
	return 0;
    aptr = GET_ATOM(atom);
    *len = aptr->len;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// BINARY
///////////////////////////////////////////////////////////////////////////////

int enif_is_binary(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return IS_BINARY(term);
}

static int get_binary(ERL_NIF_TERM term, ErlNifBinary* bin)
{
    if (IS_BINARY(term)) {
	ERL_NIF_TERM* ptr = GET_BINARY(term);
	ERL_NIF_TERM  offs = 0;
	ERL_NIF_TERM  size = 0;
	uint8_t*      data = NULL;
	int           is_sub = 0;

	if (IS_SUB_BIN(ptr[0])) {
	    sub_binary_t* sbp = (sub_binary_t*) ptr;
	    is_sub = 1;
	    size = sbp->size;
	    term = sbp->orig;
	    offs = sbp->offs;
	    ptr = GET_BINARY(sbp->orig);
	}
	if (IS_HEAP_BIN(ptr[0])) {
	    heap_binary_t* hbp = (heap_binary_t*) ptr;
	    bin->size = is_sub ? size : hbp->size;
	    bin->data = hbp->data + offs;
	    bin->bin_term = term;
	}
	else { // REFC_BIN
	    refc_binary_t* rbp = (refc_binary_t*) ptr;
	    bin->size = is_sub ? size : rbp->size;
	    bin->data = rbp->bytes + offs;
	    bin->bin_term = term;
	}
	return 1;
    }
    return 0;
}

int enif_inspect_binary(ErlNifEnv* env, ERL_NIF_TERM term, ErlNifBinary* bin)
{
    return get_binary(term, bin);
}

int enif_alloc_binary(size_t size, ErlNifBinary* bin)
{
    bin->data = enif_alloc(size);
    bin->size = size;
    bin->bin_term = 0;
    bin->ref_bin = NULL;
    return 1;
}

int enif_realloc_binary(ErlNifBinary* bin, size_t size)
{
    if (bin->bin_term == 0) {
	uint8_t* ptr = enif_realloc(bin->data, size);
	if (ptr) {
	    bin->data = ptr;
	    bin->size = size;
	    bin->ref_bin = NULL;
	    return 1;
	}
    }
    else {
	uint8_t* ptr = enif_alloc(size);
	if (ptr) {
	    if (bin->size < size) {
		memcpy(ptr, bin->data, bin->size);
		memset(ptr+bin->size, 0, size-bin->size);
	    }
	    else {
		memcpy(ptr, bin->data, size);
	    }
	    bin->bin_term = 0;
	    bin->data = ptr;
	    bin->size = size;
	    bin->ref_bin = NULL;
	    return 1;
	}
    }
    return 0;
}

void enif_release_binary(ErlNifBinary* bin)
{
    if (bin->bin_term == 0) {
	enif_free(bin->data);
	bin->data = NULL;
	bin->size = 0;
    }
}

// create heap binary
uint8_t* enif_make_new_binary(ErlNifEnv* env,size_t size,ERL_NIF_TERM* termp)
{
    size_t hsize = NWORDS(size+sizeof(heap_binary_t));
    heap_binary_t* hbp = (heap_binary_t*) cnif_heap_alloc(env,hsize);
    hbp->header = MAKE_HEAP_BINVAL(hsize-1);
    hbp->size = size;
    *termp = MAKE_BINARY(hbp);
    return (unsigned char*) hbp->data;
}

ERL_NIF_TERM enif_make_binary(ErlNifEnv* env, ErlNifBinary* bin)
{
    ERL_NIF_TERM term;
    uint8_t* ptr;

    if ((term = bin->bin_term) == 0) {
	if ((ptr = enif_make_new_binary(env, bin->size, &term)) == NULL)
	    return INVALID_TERM;
	memcpy(ptr, bin->data, bin->size);
	enif_release_binary(bin);
	bin->data = ptr;
	bin->size = bin->size;
	bin->bin_term = term;
    }
    return term;
}


static int iolist_to_buf(ERL_NIF_TERM term,uint8_t* buf,int i,unsigned len)
{
    while (IS_LIST(term)) {
    	ERL_NIF_TERM* ptr = GET_LIST(term);
	if (IS_SMALL(ptr[0])) {
	    ERL_NIF_UINT val = (ptr[0] >> TAG_IMMED1_SIZE);
	    if ((ptr[0] >> TAG_IMMED1_SIZE) > 255)
		return -1;
	    if (val > 255)
		return -1;
	    if (i >= len)
		return -1;
	    buf[i++] = val;
	}
	else if (IS_LIST(ptr[0])) {
	    int n;
	    if ((n = iolist_to_buf(ptr[0], buf, i, len)) < 0)
		return -1;
	    i += n;
	}
	else if (IS_BINARY(ptr[0])) {
	    ErlNifBinary bin;
	    if (!get_binary(ptr[0], &bin))
		return -1;
	    if (bin.size <= (len - i)) {
		memcpy(buf+i, bin.data, bin.size);
		i += bin.size;
	    }
	    else
		return -1;
	}
	else
	    return -1;
	term = ptr[1];
    }
    if (IS_BINARY(term)) {
	ErlNifBinary bin;
	if (!get_binary(term, &bin))
	    return -1;
	if (bin.size <= (len - i)) {
	    memcpy(buf, bin.data, bin.size);
	    i += bin.size;
	    return i+bin.size;
	}
	return -1;
    }
    else if (term == MAKE_NIL)
	return i;
    return -1;
}

int enif_inspect_iolist_as_binary(ErlNifEnv* env, ERL_NIF_TERM term, ErlNifBinary* bin)
{
    size_t len;
    if (!enif_iolist_size(env, term, &len))
	return 0;
    if (!enif_alloc_binary(len, bin))
	return 0;
    if (iolist_to_buf(term, bin->data, 0, bin->size) < 0) {
	enif_release_binary(bin);
	return 0;
    }
    return 1;
}

ERL_NIF_TERM enif_make_sub_binary(ErlNifEnv* env, ERL_NIF_TERM term, size_t pos, size_t size)
{
    if (IS_BINARY(term)) {
	ERL_NIF_TERM* ptr = GET_BINARY(term);
	ERL_NIF_TERM  offs = 0;
	ERL_NIF_TERM  size = 0;
	size_t        n;
	sub_binary_t* sbp;
	int           is_sub = 0;

	if (IS_SUB_BIN(ptr[0])) {
	    sbp = (sub_binary_t*) ptr;
	    is_sub = 1;
	    size = sbp->size;
	    term = sbp->orig;
	    offs = sbp->offs;
	    ptr = GET_BINARY(sbp->orig);
	}
	offs += pos;
	if (IS_HEAP_BIN(ptr[0])) {
	    heap_binary_t* hbp = (heap_binary_t*) ptr;
	    if (hbp->size < size+offs)
		return INVALID_TERM;
	    n = NWORDS(sizeof(sub_binary_t));
	    sbp = (sub_binary_t*) cnif_heap_alloc(env,n);
	    sbp->header = MAKE_SUB_BINVAL(n-1);
	    sbp->size = size;
	    sbp->offs = offs+pos;
	    sbp->bitsize = 0;
	    sbp->bitoffs = 0;
	    sbp->is_writable = 0;
	    sbp->orig = term;
	}
	else {
	    refc_binary_t* rbp = (refc_binary_t*) ptr;
	    if (rbp->size < size+offs)
		return 0;
	    n = NWORDS(sizeof(refc_binary_t));
	    sbp = (sub_binary_t*) cnif_heap_alloc(env,n);
	    sbp->header = MAKE_SUB_BINVAL(n-1);
	    sbp->size = size;
	    sbp->offs = offs+pos;
	    sbp->bitsize = 0;
	    sbp->bitoffs = 0;
	    sbp->is_writable = 0;
	    sbp->orig = term;
	}
	return MAKE_BINARY(sbp);
    }
    return INVALID_TERM;
}


////////////////////////////////////////////////////////////////////////////////
// LIST()
////////////////////////////////////////////////////////////////////////////////

ERL_NIF_TERM enif_make_list_from_array(ErlNifEnv* env, const ERL_NIF_TERM arr[], unsigned cnt)
{
    ERL_NIF_TERM cell;

    if (cnt == 0) {
	cell = MAKE_NIL;
    }
    else {
	ERL_NIF_TERM* ptr = cnif_heap_alloc(env, 2*cnt);
	int i, j;
	for (i = 0, j = 0; i < (int)cnt; i++, j += 2) {
	    ptr[j] =  arr[i];
	    ptr[j+1] = MAKE_LIST(&ptr[j+2]);
	}
	ptr[j-1] = MAKE_NIL;
	cell = MAKE_LIST(ptr);
    }
    return cell;
}

ERL_NIF_TERM enif_make_list(ErlNifEnv* env, unsigned cnt, ...)
{
    va_list ap;
    ERL_NIF_TERM cell;

    if (cnt == 0) {
	cell = MAKE_NIL;
    }
    else {
	ERL_NIF_TERM* ptr = cnif_heap_alloc(env, 2*cnt);
	int i,j;
	va_start(ap, cnt);
	for (i = 0, j = 0; i < (int) cnt; i++, j += 2) {
	    ptr[j] = va_arg(ap, ERL_NIF_TERM);
	    ptr[j+1] = MAKE_LIST(&ptr[j+2]);
	}
	ptr[j-1] = MAKE_NIL;
	va_end(ap);
	cell = MAKE_LIST(ptr);
    }
    return cell;
}

ERL_NIF_TERM enif_make_list_cell(ErlNifEnv* env, ERL_NIF_TERM car, ERL_NIF_TERM cdr)
{
    ERL_NIF_TERM* ptr = cnif_heap_alloc(env, 2);
    ERL_NIF_TERM cell;
    ptr[0] = car;
    ptr[1] = cdr;
    cell = MAKE_LIST(ptr);
    return cell;
}

int enif_is_empty_list(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return (term == MAKE_NIL);
}

int enif_is_list(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return IS_LIST(term);
}

int enif_get_list_cell(ErlNifEnv* env, ERL_NIF_TERM term, ERL_NIF_TERM* head, ERL_NIF_TERM* tail)
{
    if (IS_LIST(term)) {
	ERL_NIF_TERM* ptr = GET_LIST(term);
	*head = ptr[0];
	*tail = ptr[1];
	return 1;
    }
    return 0;
}

ERL_NIF_TERM enif_make_string_len(ErlNifEnv* env, const char* string, size_t len, ErlNifCharEncoding code)
{
    ERL_NIF_TERM cell;
    if (len == 0) {
	cell = MAKE_NIL;
    }
    else {
	ERL_NIF_TERM* ptr = cnif_heap_alloc(env, 2*len);
	int i, j;
	for (i = 0, j = 0; i < (int)len; i++, j += 2) {
	    ptr[j] =  MAKE_SMALL(string[i]);
	    ptr[j+1] = MAKE_LIST(&ptr[j+2]);
	}
	ptr[j-1] = MAKE_NIL;
	cell = MAKE_LIST(ptr);
    }
    return cell;
}

int enif_get_string(ErlNifEnv* env, ERL_NIF_TERM list, char* buf, unsigned len, ErlNifCharEncoding code)
{
    int i = 0;

    while(IS_LIST(list)) {
	ERL_NIF_TERM* ptr = GET_LIST(list);
	if (IS_SMALL(ptr[0])) {
	    ERL_NIF_UINT val = (ptr[0] >> TAG_IMMED1_SIZE);
	    if (val > 255)
		return 0;
	    if (i >= len)
		return 0;
	    buf[i++] = val;
	}
	list = ptr[1];
    }
    if (list != MAKE_NIL)
	return 0;
    buf[i] = '\0';
    return i;
}

ERL_NIF_TERM enif_make_string(ErlNifEnv* env, const char* string, ErlNifCharEncoding code)
{
    return enif_make_string_len(env, string, strlen(string), code);
}

int enif_get_list_length(ErlNifEnv* env, ERL_NIF_TERM term, unsigned* len)
{
    unsigned n = 0;

    while(IS_LIST(term)) {
	ERL_NIF_TERM* ptr = GET_LIST(term);
	n++;
	term = ptr[1];
    }
    *len = n;  // always store len even when not pure
    if (term != MAKE_NIL)
	return INVALID_TERM;
    return 1;
}

int enif_make_reverse_list(ErlNifEnv* env, ERL_NIF_TERM term, ERL_NIF_TERM *list)
{
    unsigned cnt;
    if (!enif_get_list_length(env, term, &cnt))
	return 0;
    if (cnt == 0) {
	*list = MAKE_NIL;
	return 1;
    }
    else {
	ERL_NIF_TERM* ptr = cnif_heap_alloc(env, 2*cnt);
	ERL_NIF_TERM* lptr = GET_LIST(term);
	int i, j;

	for (i = 0,j = 2*cnt-2; i < (int)cnt; i++, j -= 2) {
	    ptr[j] = lptr[0];
	    ptr[j+1] = MAKE_LIST(&ptr[j+2]);
	    lptr = GET_LIST(lptr[1]);
	}
	ptr[2*cnt-1] = MAKE_NIL;
	*list = MAKE_LIST(ptr);
	return 1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// TUPLE()
////////////////////////////////////////////////////////////////////////////////

int enif_is_tuple(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return IS_TUPLE(term);
}

// insert_value({}, 0, {1,2,3}, 3, 0) -> {0,1,2,3}

static void insert_value(ERL_NIF_TERM* dst, int i, ERL_NIF_TERM* src,
			 size_t n, ERL_NIF_TERM value)
{
    int j;
    for (j = 0; j < i; j++)
	dst[j] = src[j];
    dst[i] = value;
    for (j = i+1; j <= n; j++)
	dst[j] = src[j-1];
}

static ERL_NIF_TERM insert_element(ErlNifEnv* env, int i, ERL_NIF_TERM tpl,
				   ERL_NIF_TERM value)
{
    if (IS_TUPLE(tpl)) {
	ERL_NIF_TERM* src = GET_BOXED(tpl);
	size_t n = GET_ARITYVAL(src[0]);
	ERL_NIF_TERM* dst = cnif_heap_alloc(env, 1+n+1);
	dst[0] = MAKE_ARITYVAL(n+1);
	insert_value(dst+1, i-1, src+1, n, value);
	return MAKE_TUPLE(dst);
    }
    return INVALID_TERM;
}

static void delete_value(ERL_NIF_TERM* dst, int i, ERL_NIF_TERM* src, size_t n)
{
    int j;
    for (j = 0; j < i; j++)
	dst[j] = src[j];
    for (j = i+1; j < (int)n; j++)
	dst[j-1] = src[j];
}

static ERL_NIF_TERM delete_element(ErlNifEnv* env, int i, ERL_NIF_TERM tpl)
{
    if (IS_TUPLE(tpl)) {
	ERL_NIF_TERM* src = GET_BOXED(tpl);
	size_t n = GET_ARITYVAL(src[0]);
	ERL_NIF_TERM* dst = cnif_heap_alloc(env, 1+n-1);
	int j;
	dst[0] = MAKE_ARITYVAL(n-1);
	delete_value(dst+1, i-1, src+1, n);
	return MAKE_TUPLE(dst);
    }
    return INVALID_TERM;
}

ERL_NIF_TERM enif_make_tuple(ErlNifEnv* env, unsigned cnt, ...)
{
    va_list ap;
    ERL_NIF_TERM* ptr = cnif_heap_alloc(env, 1+cnt);
    int i;

    ptr[0] = MAKE_ARITYVAL(cnt);
    va_start(ap, cnt);
    for (i = 0; i < (int) cnt; i++)
	ptr[i+1] = va_arg(ap, ERL_NIF_TERM);
    va_end(ap);
    return MAKE_TUPLE(ptr);
}

ERL_NIF_TERM enif_make_tuple_from_array(ErlNifEnv* env, const ERL_NIF_TERM arr[], unsigned cnt)
{
    ERL_NIF_TERM* ptr = cnif_heap_alloc(env, 1+cnt);
    int i;

    ptr[0] = MAKE_ARITYVAL(cnt);
    for (i = 0; i < (int) cnt; i++)
	ptr[i+1] = arr[i];
    return MAKE_TUPLE(ptr);
}

int enif_get_tuple(ErlNifEnv* env, ERL_NIF_TERM tpl, int* arity, const ERL_NIF_TERM** array)
{
    if (IS_TUPLE(tpl)) {
	ERL_NIF_TERM* ptr = GET_BOXED(tpl);
	*arity = GET_ARITYVAL(ptr[0]);
	*array = ptr+1;
	return 1;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// MAP()
////////////////////////////////////////////////////////////////////////////////

int enif_is_map (ErlNifEnv* env, ERL_NIF_TERM term)
{
    return IS_MAP(term);
}

int enif_get_map_size (ErlNifEnv* env, ERL_NIF_TERM term, size_t *size)
{
    if (IS_MAP(term)) {
	flatmap_t* mp = (flatmap_t*) GET_MAP(term);
	*size = mp->size;
	return 1;
    }
    return 0;
}

ERL_NIF_TERM enif_make_new_map(ErlNifEnv* env)
{
    size_t n = NWORDS(sizeof(flatmap_t));
    flatmap_t* mp = (flatmap_t*) cnif_heap_alloc(env,n);
    mp->header = MAKE_MAPVAL(n-1);
    mp->size = 0;
    mp->keys = enif_make_tuple0(env);
    return MAKE_MAP(mp);
}

static int key_index(int low, int high, int* index,
		     ERL_NIF_TERM key, const ERL_NIF_TERM* keys)
{
    int mid = 0;
    while(low <= high) {
	int r;
	ERL_NIF_TERM key1;
	mid = (high+low)/2;
	key1 = keys[mid-1];

	if ((r = enif_compare(key, key1)) < 0)
	    high = mid-1;
	else if (r > 0)
	    low = mid+1;
	else {
	    *index = mid;
	    return 1;
	}
    } while(low <= high);
    *index = mid;
    return 0;
}

int enif_get_map_value(ErlNifEnv* env, ERL_NIF_TERM map, ERL_NIF_TERM key, ERL_NIF_TERM* value)
{
    if (IS_MAP(map)) {
	flatmap_t* mp = (flatmap_t*) GET_MAP(map);
	const ERL_NIF_TERM* keys;
	int arity;
	int i;
	enif_get_tuple(env, mp->keys, &arity, &keys);
	if (!key_index(1, arity, &i, key, keys))
	    return 0;
	*value = mp->value[i-1];
	return 1;
    }
    return 0;
}

int enif_make_map_update(ErlNifEnv* env, ERL_NIF_TERM map_in, ERL_NIF_TERM key, ERL_NIF_TERM value, ERL_NIF_TERM* map_out)
{
    if (IS_MAP(map_in)) {
	flatmap_t* mp_in = (flatmap_t*) GET_MAP(map_in);
	flatmap_t* mp_out;
	const ERL_NIF_TERM* keys;
	size_t n = NWORDS(sizeof(flatmap_t));
	int arity;
	int cnt = mp_in->size;
	int i;
	enif_get_tuple(env, mp_in->keys, &arity, &keys);
	if (!key_index(1, arity, &i, key, keys))
	    return 0;
	mp_out =  (flatmap_t*) cnif_heap_alloc(env, n+cnt);
	*mp_out = *mp_in;
	memcpy(mp_out->value, mp_in->value, cnt*sizeof(ERL_NIF_TERM));
	mp_out->value[i-1] = value;  // new value
	*map_out = MAKE_MAP(mp_out);
	return 1;
    }
    return 0;
}

int enif_make_map_put(ErlNifEnv* env, ERL_NIF_TERM map_in, ERL_NIF_TERM key, ERL_NIF_TERM value, ERL_NIF_TERM* map_out)
{
    if (IS_MAP(map_in)) {
	flatmap_t* mp_in = (flatmap_t*) GET_MAP(map_in);
	flatmap_t* mp_out;
	const ERL_NIF_TERM* keys;
	size_t n = NWORDS(sizeof(flatmap_t));
	int arity;
	int cnt = mp_in->size;
	int i;
	enif_get_tuple(env, mp_in->keys, &arity, &keys);
	if (!key_index(1, arity, &i, key, keys)) {
	    mp_out =  (flatmap_t*) cnif_heap_alloc(env, n+cnt+1);
	    mp_out->header = MAKE_MAPVAL(n+cnt);
	    mp_out->size = cnt+1;
	    mp_out->keys = insert_element(env, i, mp_in->keys, key);
	    insert_value(mp_out->value,i-1,mp_in->value,cnt,value);
	    *map_out = MAKE_MAP(mp_out);
	    return 1;
	}
	else {
	    mp_out =  (flatmap_t*) cnif_heap_alloc(env, n+cnt);
	    *mp_out = *mp_in;
	    memcpy(mp_out->value,mp_in->value,cnt*sizeof(ERL_NIF_TERM));
	    mp_out->value[i-1] = value;  // new value
	    *map_out = MAKE_MAP(mp_out);
	}
	return 1;
    }
    return 0;
}

int enif_make_map_remove (ErlNifEnv* env, ERL_NIF_TERM map_in, ERL_NIF_TERM key, ERL_NIF_TERM* map_out)
{
    if (IS_MAP(map_in)) {
	flatmap_t* mp_in = (flatmap_t*) GET_MAP(map_in);
	flatmap_t* mp_out;
	const ERL_NIF_TERM* keys;
	size_t n = NWORDS(sizeof(flatmap_t));
	int arity;
	int cnt = mp_in->size;
	int i, j;
	enif_get_tuple(env, mp_in->keys, &arity, &keys);
	if (!key_index(1, arity, &i, key, keys))
	    return 0;
	mp_out =  (flatmap_t*) cnif_heap_alloc(env, n+cnt);
	mp_out->header = MAKE_MAPVAL(n+cnt-1-1);
	mp_out->size = cnt-1;
	mp_out->keys = delete_element(env, i, mp_in->keys);
	delete_value(mp_out->value, i-1, mp_in->value, cnt);
	*map_out = MAKE_MAP(mp_out);
	return 1;
    }
    return 0;
}

int enif_map_iterator_create (ErlNifEnv *env, ERL_NIF_TERM map, ErlNifMapIterator *iter, ErlNifMapIteratorEntry entry)
{
    if (IS_MAP(map)) {
	flatmap_t* mp = (flatmap_t*) GET_MAP(map);
	if (entry == ERL_NIF_MAP_ITERATOR_FIRST) {
	    iter->map  = map;
	    iter->size = mp->size;
	    iter->idx  = 0;
	    return 1;
	}
	else if (entry == ERL_NIF_MAP_ITERATOR_LAST) {
	    iter->map  = map;
	    iter->size = mp->size;
	    iter->idx  = mp->size+1;
	    return 1;
	}
    }
    return 0;
}

void enif_map_iterator_destroy (ErlNifEnv *env, ErlNifMapIterator *iter)
{
    iter->map  = 0;
    iter->size = 0;
    iter->idx  = 0;
}

int enif_map_iterator_is_head (ErlNifEnv *env, ErlNifMapIterator *iter)
{
    if (iter->idx < 1)
	return 1;
    return 0;
}

int enif_map_iterator_is_tail(ErlNifEnv *env, ErlNifMapIterator *iter)
{
    if (iter->idx > iter->size)
	return 1;
    return 0;
}

int enif_map_iterator_next(ErlNifEnv *env, ErlNifMapIterator *iter)
{
    if (iter->idx < iter->size) {
	iter->idx++;
	return 1;
    }
    if (iter->idx == iter->size)
	iter->idx++;
    return 0;
}

int enif_map_iterator_prev(ErlNifEnv *env, ErlNifMapIterator *iter)
{
    if (iter->idx > 1) {
	iter->idx--;
	return 1;
    }
    if (iter->idx == 1)
	iter->idx--;
    return 0;
}

int enif_map_iterator_get_pair(ErlNifEnv *env, ErlNifMapIterator *iter, ERL_NIF_TERM *key, ERL_NIF_TERM *value)
{
    flatmap_t* mp = (flatmap_t*) GET_MAP(iter->map);
    const ERL_NIF_TERM* keys;
    int arity;
    ERL_NIF_UINT i;

    enif_get_tuple(env, mp->keys, &arity, &keys);
    if (((i = iter->idx) >= 1) && (i <= mp->size)) {
	*key = keys[i-1];
	*value = mp->value[i-1];
	return 1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// UTIL
///////////////////////////////////////////////////////////////////////////////


// FIXME: bignums
static int compare_integer(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs)
{
    int64_t li, ri;
    get_int64(lhs, &li);
    get_int64(rhs, &ri);
    return li - ri;
}

static int compare_float(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs)
{
    double lf, rf;
    get_double(lhs, &lf);
    get_double(rhs, &rf);
    if (lf < rf) return -1;
    else if (lf > rf) return 1;
    return 0;
}

// FIXME: bignums
static int compare_integer_float(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs, int exact)
{
    int64_t li;
    double rf;
    get_int64(lhs, &li);
    get_double(rhs, &rf);
    if (li < rf) return -1;
    else if (li > rf) return 1;
    if (exact) return -1;
    return 0;
}

static int compare_atom(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs)
{
    atom_t* lptr;
    atom_t* rptr;
    size_t  n;
    size_t  i;
    int r;
    lptr = GET_ATOM(lhs);
    rptr = GET_ATOM(rhs);
    n = (lptr->len < rptr->len) ? lptr->len : rptr->len;
    for (i = 0; i < n; i++) {
	if ((r = (lptr->name[i] - rptr->name[i])) != 0)
	    return r;
    }
    return lptr->len - rptr->len;
}

static int compare_binary(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs)
{
    ErlNifBinary lb, rb;
    size_t n;
    size_t i;
    int r;

    get_binary(lhs, &lb);
    get_binary(rhs, &rb);

    n = (lb.size < rb.size) ? lb.size : rb.size;
    for (i = 0; i < n; i++) {
	if ((r = (lb.data[i] - rb.data[i])) != 0)
	    return r;
    }
    return lb.size - rb.size;
}

static int compare(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs, int exact)
{
    if (lhs == rhs)
	return 0;
    else {
	enif_type_t lt = enif_get_type(lhs, 0);
	enif_type_t rt = enif_get_type(rhs, 0);

	if ((lt <= ENIF_TYPE_NUMBER) && (rt <= ENIF_TYPE_NUMBER)) {
	    if (lt == ENIF_TYPE_INTEGER) {
		if (rt == ENIF_TYPE_INTEGER)
		    return compare_integer(lhs, rhs);
		else
		    return compare_integer_float(lhs, rhs, exact);
	    }
	    else {
		if (rt == ENIF_TYPE_FLOAT)
		    return compare_float(lhs, rhs);
		else
		    return -compare_integer_float(rhs, lhs, exact);
	    }
	}
	if (lt != rt)
	    return lt - rt;
	switch(lt) {
	case ENIF_TYPE_ATOM:
	    return compare_atom(lhs, rhs);
	case ENIF_TYPE_TUPLE: {
	    ERL_NIF_TERM* lptr = GET_TUPLE(lhs);
	    ERL_NIF_TERM* rptr = GET_TUPLE(rhs);
	    int lari = GET_ARITYVAL(lptr[0]);
	    int rari = GET_ARITYVAL(rptr[0]);
	    int i;
	    if (lari != rari)
		return lari - rari;
	    for (i = 1; i <= lari; i++) {
		int r;
		if ((r = compare(lptr[i], rptr[i], exact)) != 0)
		    return r;
	    }
	    return 0;
	}
	case ENIF_TYPE_MAP:
	    return 1;
	case ENIF_TYPE_NIL:
	    return 0;
	case ENIF_TYPE_LIST: {
	    ERL_NIF_TERM* lptr = GET_LIST(lhs);
	    ERL_NIF_TERM* rptr = GET_LIST(rhs);
	    int r;
	    
	    while((r = compare(lptr[0], rptr[0], exact)) == 0) {
		if ((lptr[1] == MAKE_NIL) && (rptr[1] == MAKE_NIL))
		    return 0;
		if (IS_LIST(lptr[1]) && IS_LIST(rptr[1])) {
		    lptr = GET_LIST(lptr[1]);
		    rptr = GET_LIST(rptr[1]);
		}
		else {
		    if (lptr[1] == MAKE_NIL)
			return -1;
		    if (rptr[1] == MAKE_NIL)
			return 1;
		    return compare(lptr[1], rptr[1], exact);
		}
	    }
	    return r;
	}
	case ENIF_TYPE_BINARY:
	    return compare_binary(lhs, rhs);
	default:
	    return 1;
	}
    }
}

int enif_compare(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs)
{
    return compare(lhs, rhs, 0);  // compare == 
}

int enif_is_identical(ERL_NIF_TERM lhs, ERL_NIF_TERM rhs)
{
    return compare(lhs, rhs, 1) == 0;  // compare =:= 
}
