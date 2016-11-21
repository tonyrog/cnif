//
// Copy functions
//
#include <stdio.h>
#include "../include/cnif_term.h"

//
// RECURSIVE SIZE OF TERM
//
static ERL_NIF_UINT size_boxed(ERL_NIF_TERM src_term);
static ERL_NIF_UINT size_list(ERL_NIF_TERM src_term);

static inline ERL_NIF_UINT flat_size(ERL_NIF_TERM src_term)
{
    switch(src_term & 3) {
    case TAG_PRIMARY_HEADER: return 0;
    case TAG_PRIMARY_LIST:   return size_list(src_term);
    case TAG_PRIMARY_BOXED:  return size_boxed(src_term);
    case TAG_PRIMARY_IMMED1: return 0;
    }
    return 0;
}

static ERL_NIF_UINT size_boxed(ERL_NIF_TERM src_term)
{
    ERL_NIF_TERM* srcp  = GET_BOXED(src_term);
    ERL_NIF_UINT  arity = GET_ARITYVAL(*srcp);
    ERL_NIF_UINT  i;
    ERL_NIF_UINT  sz = arity+1;
	
    switch(srcp[0] & _TAG_HEADER_MASK) {
    case TAG_HEADER_POS_BIG:
    case TAG_HEADER_NEG_BIG:
    case TAG_HEADER_FLOAT:
    case TAG_HEADER_HEAP_BIN:
    case TAG_HEADER_REFC_BIN:
    case TAG_HEADER_EXTERNAL_PID:
    case TAG_HEADER_EXTERNAL_PORT:
    case TAG_HEADER_EXTERNAL_REF:
    case TAG_HEADER_BIN_MATCHSTATE:
	break;
    case TAG_HEADER_ARITYVAL:
	for (i = 1; i <= arity; i++)
	    sz += flat_size(srcp[i]);
	break;
    case TAG_HEADER_SUB_BIN: {
	sub_binary_t* src_sbp = (sub_binary_t*) srcp;
	sz += flat_size(src_sbp->orig);
	break;
    }
    case TAG_HEADER_MAP: {
	flatmap_t* src_mp = (flatmap_t*) srcp;
	ERL_NIF_UINT size = src_mp->size;
	sz += flat_size(src_mp->keys);
	for (i = 0; i < size; i++)
	    sz += flat_size(src_mp->value[i]);
	break;
    }
    default:
	break;
    }
    return sz;
}

static ERL_NIF_UINT size_list(ERL_NIF_TERM src_term)
{
    ERL_NIF_TERM* srcp;
    ERL_NIF_UINT sz = 0;  // the cons cell

    goto do_list;
again:
    switch(src_term & 3) {
    case TAG_PRIMARY_HEADER:
	return 0;
    case TAG_PRIMARY_LIST:
    do_list:
	srcp = GET_LIST(src_term);
	sz += 2;
	sz += flat_size(srcp[0]);
	src_term = srcp[1];
	goto again;
    case TAG_PRIMARY_BOXED:
	sz += size_boxed(src_term);
	break;
    case TAG_PRIMARY_IMMED1:
	break;
    }
    return sz;
}

ERL_NIF_UINT enif_flat_size(ERL_NIF_TERM src_term)
{
    return flat_size(src_term);
}

// 
// RECURSIVE COPY
//

ERL_NIF_TERM enif_make_copy(ErlNifEnv* dst_env, ERL_NIF_TERM src_term);
static ERL_NIF_TERM copy_boxed(ErlNifEnv* dst_env, ERL_NIF_TERM src_term);
static ERL_NIF_TERM copy_list(ErlNifEnv* dst_env, ERL_NIF_TERM src_term);

static inline ERL_NIF_TERM recursive_copy(ErlNifEnv* dst_env, 
					  ERL_NIF_TERM src_term)
{
    switch(src_term & 3) {
    case TAG_PRIMARY_HEADER: return 0;
    case TAG_PRIMARY_LIST:   return copy_list(dst_env, src_term);
    case TAG_PRIMARY_BOXED:  return copy_boxed(dst_env, src_term);
    case TAG_PRIMARY_IMMED1: return src_term;
    }
    return 0;
}

static ERL_NIF_TERM copy_boxed_data(ErlNifEnv* dst_env, ERL_NIF_TERM* dstp,
				    ERL_NIF_TERM* srcp, ERL_NIF_UINT n)
{
    ERL_NIF_UINT i;
    for (i = 0; i <= n; i++) // include arity
	dstp[i] = srcp[i];
    return MAKE_BOXED(dstp);    
}

static ERL_NIF_TERM copy_boxed_tuple(ErlNifEnv* dst_env, ERL_NIF_TERM* dstp,
				     ERL_NIF_TERM* srcp, ERL_NIF_UINT n)
{
    ERL_NIF_UINT i;
    dstp[0] = srcp[0];  // arity
    for (i = 1; i <= n; i++)
	dstp[i] = recursive_copy(dst_env, srcp[i]);
    return MAKE_BOXED(dstp);    
}

static ERL_NIF_TERM copy_boxed_map(ErlNifEnv* dst_env, ERL_NIF_TERM* dstp,
				   ERL_NIF_TERM* srcp)
{
    flatmap_t* src_mp = (flatmap_t*) srcp;
    flatmap_t* dst_mp = (flatmap_t*) dstp;
    ERL_NIF_UINT size = src_mp->size;
    ERL_NIF_UINT  i;
	
    dst_mp->header = src_mp->header;
    dst_mp->size   = src_mp->size;
    dst_mp->keys = recursive_copy(dst_env, src_mp->keys);
    for (i = 0; i < size; i++)
	dst_mp->value[i] = recursive_copy(dst_env, src_mp->value[i]);
    return MAKE_MAP(dst_mp);    
}

static ERL_NIF_TERM copy_boxed_refc_bin(ErlNifEnv* dst_env, ERL_NIF_TERM* dstp,
					ERL_NIF_TERM* srcp, ERL_NIF_UINT n)
{
    refc_binary_t* src_rbp = (refc_binary_t*) srcp;
    refc_binary_t* dst_rbp = (refc_binary_t*) dstp;
    dst_rbp->header = src_rbp->header;
    dst_rbp->size   = src_rbp->size;
    dst_rbp->next   = 0;
    dst_rbp->val    = src_rbp->val;
    dst_rbp->bytes  = dst_rbp->bytes;
    dst_rbp->flags  = dst_rbp->flags;
    dst_rbp->val->refc++;
    return MAKE_BINARY(dstp);
}

static ERL_NIF_TERM copy_boxed_sub_bin(ErlNifEnv* dst_env, ERL_NIF_TERM* dstp,
				       ERL_NIF_TERM* srcp, ERL_NIF_UINT n)
{
    sub_binary_t* src_sbp = (sub_binary_t*) srcp;
    sub_binary_t* dst_sbp = (sub_binary_t*) dstp;
    dst_sbp->header = src_sbp->header;
    dst_sbp->size   = src_sbp->size;
    dst_sbp->offs   = src_sbp->offs;
    dst_sbp->bitsize   = src_sbp->bitsize;
    dst_sbp->bitoffs   = src_sbp->bitoffs;
    dst_sbp->is_writable = src_sbp->is_writable;
    dst_sbp->orig      = recursive_copy(dst_env, src_sbp->orig);
    return MAKE_BINARY(dstp);
}

static ERL_NIF_TERM copy_boxed(ErlNifEnv* dst_env, ERL_NIF_TERM src_term)
{
    ERL_NIF_TERM* srcp  = GET_BOXED(src_term);
    ERL_NIF_UINT  arity = GET_ARITYVAL(*srcp);
    ERL_NIF_TERM* dstp  = cnif_heap_alloc(dst_env, arity+1);

    switch(srcp[0] & _TAG_HEADER_MASK) {
    case TAG_HEADER_POS_BIG:
    case TAG_HEADER_NEG_BIG:
    case TAG_HEADER_FLOAT:
	return copy_boxed_data(dst_env, dstp, srcp, arity);
    case TAG_HEADER_ARITYVAL:
	return copy_boxed_tuple(dst_env, dstp, srcp, arity);
    case TAG_HEADER_EXPORT:
    case TAG_HEADER_FUN:
	return 0;

    case TAG_HEADER_HEAP_BIN:
	return copy_boxed_data(dst_env, dstp, srcp, arity);
    case TAG_HEADER_REFC_BIN:
	return copy_boxed_refc_bin(dst_env, dstp, srcp, arity);
    case TAG_HEADER_SUB_BIN:
	return copy_boxed_sub_bin(dst_env, dstp, srcp, arity);
    case TAG_HEADER_EXTERNAL_PID:
    case TAG_HEADER_EXTERNAL_PORT:
    case TAG_HEADER_EXTERNAL_REF:
    case TAG_HEADER_BIN_MATCHSTATE:
	return 0;
    case TAG_HEADER_MAP:
	return copy_boxed_map(dst_env, dstp, srcp);
    default:
	return 0;
    }
}

static ERL_NIF_TERM copy_list(ErlNifEnv* dst_env, ERL_NIF_TERM src_term)
{
    ERL_NIF_TERM* dstp0 = NULL;
    ERL_NIF_TERM* srcp;
    ERL_NIF_TERM* dstp;
    ERL_NIF_TERM* tailp = NULL;

    goto start;

again:
    switch(src_term & 3) {
    case TAG_PRIMARY_HEADER:
	return 0;
    case TAG_PRIMARY_LIST:
    start:
	srcp = GET_LIST(src_term);
	dstp = cnif_heap_alloc(dst_env, 2);
	if (tailp)
	    *tailp = MAKE_LIST(dstp);
	else
	    dstp0 = dstp;
	dstp[0] = recursive_copy(dst_env, srcp[0]);
	tailp = dstp+1;
	src_term = srcp[1];
	goto again;
    case TAG_PRIMARY_BOXED:
	*tailp = copy_boxed(dst_env, src_term);
	break;
    case TAG_PRIMARY_IMMED1:
	*tailp = src_term;
	break;
    }
    return MAKE_LIST(dstp0);
}

// copy src_term into dst_env and return the term copy
ERL_NIF_TERM enif_make_copy(ErlNifEnv* dst_env, ERL_NIF_TERM src_term)
{
    return recursive_copy(dst_env, src_term);
}

// 
// FLAT COPY - flat copy without a stack
//
static inline ERL_NIF_TERM flat_copy(ErlNifEnv* dst_env, 
				     ERL_NIF_TERM src_term)
{
    ERL_NIF_UINT size;
    ERL_NIF_TERM* srcp;
    ERL_NIF_TERM* from;
    ERL_NIF_TERM* to;
    ERL_NIF_TERM  dst_term;

    if ((size=flat_size(src_term)) == 0)
	return src_term;
    to = cnif_heap_alloc(dst_env, size);  // must be consecutive chunk
    from = to;

    switch(src_term & 3) {
    case TAG_PRIMARY_HEADER: // FIXME: fail
	return 0;
    case TAG_PRIMARY_LIST: {
	ERL_NIF_TERM* srcp = GET_LIST(src_term);
	to[0] = srcp[0];
	to[1] = srcp[1];
	dst_term = MAKE_LIST(to);
	to += 2;
	break;
    }
    case TAG_PRIMARY_BOXED: {
	ERL_NIF_TERM* srcp = GET_BOXED(src_term);
	ERL_NIF_UINT  arity = GET_ARITYVAL(*srcp);
	ERL_NIF_UINT  i;
	for (i = 0; i <= arity; i++)
	    to[i] = srcp[i];
	dst_term = MAKE_BOXED(to);
	to += (arity+1);
	break;
    }
    case TAG_PRIMARY_IMMED1: // FIXME: fail
	return 0;
    }

    while(from < to) {
	ERL_NIF_TERM src = *from;
	switch(src & 3) {
	case TAG_PRIMARY_HEADER: {
	    ERL_NIF_UINT arity = GET_ARITYVAL(src);
	    switch(src & _TAG_HEADER_MASK) {
	    case TAG_HEADER_POS_BIG:
	    case TAG_HEADER_NEG_BIG:
	    case TAG_HEADER_FLOAT:
	    case TAG_HEADER_HEAP_BIN:
	    case TAG_HEADER_EXTERNAL_PID:
	    case TAG_HEADER_EXTERNAL_PORT:
	    case TAG_HEADER_EXTERNAL_REF:
	    case TAG_HEADER_BIN_MATCHSTATE:
		from += (arity+1);
		break;
	    case TAG_HEADER_REFC_BIN: {
		refc_binary_t* rbp = (refc_binary_t*) from;
		rbp->val->refc++;
		// fixme: link it
		from += (arity+1);
		break;
	    }
	    case TAG_HEADER_ARITYVAL:
		from++;
		break;
	    case TAG_HEADER_SUB_BIN: {
		sub_binary_t* sbp = (sub_binary_t*) from;
		from = (ERL_NIF_TERM*) &sbp->orig;
	    }
	    case TAG_HEADER_MAP: {
		flatmap_t* mp = (flatmap_t*) from;
		from = (ERL_NIF_TERM*) &mp->keys;
	    }
	    default:
		// FIXME: fail
		return 0;
	    }
	    break;
	}

	case TAG_PRIMARY_LIST: {
	    ERL_NIF_TERM* srcp = GET_LIST(src);
	    to[0] = srcp[0];
	    to[1] = srcp[1];
	    *from++ = MAKE_LIST(to);
	    to += 2;
	    break;
	}
	case TAG_PRIMARY_BOXED: {
	    ERL_NIF_TERM* srcp  = GET_BOXED(src);
	    ERL_NIF_UINT  arity = GET_ARITYVAL(*srcp);
	    ERL_NIF_UINT  i;
	    for (i = 0; i <= arity; i++)
		to[i] = srcp[i];
	    *from++ = MAKE_BOXED(to);
	    to += (arity+1);
	    break;
	}
	case TAG_PRIMARY_IMMED1:
	    from++;
	}
    }
    return dst_term;
}


// copy src_term into dst_env and return the term copy
ERL_NIF_TERM enif_make_flat_copy(ErlNifEnv* dst_env, ERL_NIF_TERM src_term)
{
    return flat_copy(dst_env, src_term);
}

// 
// STRUCT COPY - struct copy without a stack
// and repair the original using a log
//
#define IN_RANGE(ptr,src0,src1) (((ptr) >= src0) && ((ptr) <= src1))

#define SET_POINTER(logp, srcp, value) do {		\
	if ((logp)) {				\
	    (logp)[0] = (ERL_NIF_TERM) (srcp);	\
	    (logp)[1] = (srcp)[0];		\
	    logp += 2;				\
	}					\
	(srcp)[0] = (value);			\
    } while(0)	


static inline ERL_NIF_TERM struct_copy(ErlNifEnv* dst_env,
				       ERL_NIF_TERM src_term)
{
    ERL_NIF_UINT size;
    ERL_NIF_TERM* srcp;
    ERL_NIF_TERM* from;
    ERL_NIF_TERM* to0;
    ERL_NIF_TERM* to;
    ERL_NIF_TERM* to_end;
    ERL_NIF_TERM  dst_term;
    ERL_NIF_TERM* logp0;
    ERL_NIF_TERM* logp;
    ERL_NIF_TERM* ptr;

    if ((size = flat_size(src_term)) == 0)
	return src_term;
    to0 = cnif_heap_alloc(dst_env, size);  // must be consecutive chunk
    to  = to0;
    from = to0;

    logp0 = (ERL_NIF_TERM*) enif_alloc(2*size*sizeof(ERL_NIF_TERM*));
    logp = logp0;

    switch(src_term & 3) {
    case TAG_PRIMARY_HEADER: // FIXME: fail
	return 0;
    case TAG_PRIMARY_LIST: {
	ERL_NIF_TERM* srcp = GET_LIST(src_term);
	to[0] = srcp[0];
	to[1] = srcp[1];
	dst_term = MAKE_LIST(to);
	SET_POINTER(logp, srcp, dst_term);
	to += 2;
	break;
    }
    case TAG_PRIMARY_BOXED: {
	ERL_NIF_TERM* srcp  = GET_BOXED(src_term);
	ERL_NIF_UINT  arity = GET_ARITYVAL(*srcp);
	ERL_NIF_UINT  i;
	for (i = 0; i <= arity; i++)
	    to[i] = srcp[i];
	dst_term = MAKE_BOXED(to);
	SET_POINTER(logp, srcp, dst_term);
	to += (arity+1);
	break;
    }
    case TAG_PRIMARY_IMMED1: // FIXME: fail
	return 0;
    }

    while(from < to) {
	ERL_NIF_TERM src = *from;
	switch(src & 3) {
	case TAG_PRIMARY_HEADER: {
	    ERL_NIF_UINT arity = GET_ARITYVAL(src);
	    switch(src & _TAG_HEADER_MASK) {
	    case TAG_HEADER_POS_BIG:
	    case TAG_HEADER_NEG_BIG:
	    case TAG_HEADER_FLOAT:
	    case TAG_HEADER_HEAP_BIN:
	    case TAG_HEADER_EXTERNAL_PID:
	    case TAG_HEADER_EXTERNAL_PORT:
	    case TAG_HEADER_EXTERNAL_REF:
	    case TAG_HEADER_BIN_MATCHSTATE:
		from += (arity+1);
		break;
	    case TAG_HEADER_REFC_BIN: {
		refc_binary_t* rbp = (refc_binary_t*) from;
		rbp->val->refc++;
		// fixme: link it
		from += (arity+1);
		break;
	    }
	    case TAG_HEADER_ARITYVAL:
		from++;
		break;
	    case TAG_HEADER_SUB_BIN: {
		sub_binary_t* sbp = (sub_binary_t*) from;
		from = (ERL_NIF_TERM*) &sbp->orig;
	    }
	    case TAG_HEADER_MAP: {
		flatmap_t* mp = (flatmap_t*) from;
		from = (ERL_NIF_TERM*) &mp->keys;
	    }
	    default:
		// FIXME: fail
		return 0;
	    }
	    break;
	}

	case TAG_PRIMARY_LIST: {
	    ERL_NIF_TERM* srcp = GET_LIST(src);
	    if (IS_LIST(srcp[0]) && IN_RANGE(GET_PTR(srcp[0]),to0,to)) {
		printf("reuse list = %p\n", (ERL_NIF_TERM*) srcp[0]);
		*from++ = srcp[0];
	    }
	    else {
		to[0] = srcp[0];
		to[1] = srcp[1];
		*from = MAKE_LIST(to);
		SET_POINTER(logp, srcp, *from);
		from++;
		to += 2;
	    }
	    break;
	}
	case TAG_PRIMARY_BOXED: {
	    ERL_NIF_TERM* srcp  = GET_BOXED(src);
	    if (IS_BOXED(srcp[0]) && IN_RANGE(GET_PTR(srcp[0]),to0,to)) {
		printf("reuse structure = %p\n", (ERL_NIF_TERM*) srcp[0]);
		*from++ = srcp[0];
	    }
	    else {
		ERL_NIF_UINT  arity = GET_ARITYVAL(*srcp);
		ERL_NIF_UINT  i;
		for (i = 0; i <= arity; i++)
		    to[i] = srcp[i];
		*from = MAKE_BOXED(to);
		SET_POINTER(logp, srcp, *from);
		from++;
		to += (arity+1);
	    }
	    break;
	}
	case TAG_PRIMARY_IMMED1: 
	    from++;
	}
    }
    printf("struct size = %ld\n", (to - to0));

    // restore pointers
    ptr = logp0;
    while(ptr < logp) {
	printf("restore pointer %p\n", (ERL_NIF_TERM*) ptr[0]);
	*((ERL_NIF_TERM*) ptr[0]) = ptr[1];
	ptr += 2;
    }
    enif_free(logp0);

    return dst_term;
}

//
//  Structure copy with repair
//
ERL_NIF_TERM enif_make_struct_copy(ErlNifEnv* dst_env, ERL_NIF_TERM src_term)
{
    return struct_copy(dst_env, src_term);
}
