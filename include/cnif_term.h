#ifndef __CNIF_TERM_H__
#define __CNIF_TERM_H__

#include <stdlib.h>
#include <stdint.h>

#include "cnif.h"
#include "cnif_lhash.h"


#define TAG_PRIMARY_SIZE        2
#define TAG_PRIMARY_HEADER      0x0
#define TAG_PRIMARY_LIST        0x1
#define TAG_PRIMARY_BOXED       0x2
#define TAG_PRIMARY_IMMED1      0x3

#define TAG_IMMED1_SIZE         4
#define TAG_IMMED1_MASK         0xF
#define TAG_IMMED1_PID          ((0x0 << TAG_PRIMARY_SIZE) | TAG_PRIMARY_IMMED1)
#define TAG_IMMED1_PORT         ((0x1 << TAG_PRIMARY_SIZE) | TAG_PRIMARY_IMMED1)
#define TAG_IMMED1_IMMED2       ((0x2 << TAG_PRIMARY_SIZE) | TAG_PRIMARY_IMMED1)
#define TAG_IMMED1_SMALL        ((0x3 << TAG_PRIMARY_SIZE) | TAG_PRIMARY_IMMED1)

#define TAG_IMMED2_SIZE         6
#define TAG_IMMED2_MASK         0x3F
#define TAG_IMMED2_ATOM         ((0x0 << TAG_IMMED1_SIZE) | TAG_IMMED1_IMMED2)
#define TAG_IMMED2_CATCH        ((0x1 << TAG_IMMED1_SIZE) | TAG_IMMED1_IMMED2)
#define TAG_IMMED2_NIL          ((0x3 << TAG_IMMED1_SIZE) | TAG_IMMED1_IMMED2)

// _TAG_PRIMARY_HEADER
#define ARITYVAL_SUBTAG         (0x0 << TAG_PRIMARY_SIZE)
#define BIN_MATCHSTATE_SUBTAG   (0x1 << TAG_PRIMARY_SIZE) 
#define POS_BIG_SUBTAG          (0x2 << TAG_PRIMARY_SIZE)
#define NEG_BIG_SUBTAG          (0x3 << TAG_PRIMARY_SIZE)
#define _BIG_SIGN_BIT           (0x1 << TAG_PRIMARY_SIZE)
#define REF_SUBTAG              (0x4 << TAG_PRIMARY_SIZE)
#define FUN_SUBTAG              (0x5 << TAG_PRIMARY_SIZE)
#define FLOAT_SUBTAG            (0x6 << TAG_PRIMARY_SIZE)
#define EXPORT_SUBTAG           (0x7 << TAG_PRIMARY_SIZE)
#define _BINARY_XXX_MASK        (0x3 << TAG_PRIMARY_SIZE)
#define REFC_BINARY_SUBTAG      (0x8 << TAG_PRIMARY_SIZE)
#define HEAP_BINARY_SUBTAG      (0x9 << TAG_PRIMARY_SIZE)
#define SUB_BINARY_SUBTAG       (0xA << TAG_PRIMARY_SIZE)
//  _BINARY_XXX_MASK depends on 0xB being unused
#define EXTERNAL_PID_SUBTAG     (0xC << TAG_PRIMARY_SIZE)
#define EXTERNAL_PORT_SUBTAG    (0xD << TAG_PRIMARY_SIZE)
#define EXTERNAL_REF_SUBTAG     (0xE << TAG_PRIMARY_SIZE)
#define MAP_SUBTAG		(0xF << TAG_PRIMARY_SIZE)

#define _TAG_HEADER_MASK        0x3F
#define _HEADER_SUBTAG_MASK     0x3C
#define _HEADER_ARITY_OFFS      6

#define TAG_HEADER_ARITYVAL	(TAG_PRIMARY_HEADER | ARITYVAL_SUBTAG)
#define TAG_HEADER_FUN	        (TAG_PRIMARY_HEADER | FUN_SUBTAG)
#define TAG_HEADER_POS_BIG	(TAG_PRIMARY_HEADER | POS_BIG_SUBTAG)
#define TAG_HEADER_NEG_BIG	(TAG_PRIMARY_HEADER | NEG_BIG_SUBTAG)
#define TAG_HEADER_FLOAT	(TAG_PRIMARY_HEADER | FLOAT_SUBTAG)
#define TAG_HEADER_EXPORT	(TAG_PRIMARY_HEADER | EXPORT_SUBTAG)
#define TAG_HEADER_REF         (TAG_PRIMARY_HEADER | REF_SUBTAG)
#define TAG_HEADER_REFC_BIN	(TAG_PRIMARY_HEADER | REFC_BINARY_SUBTAG)
#define TAG_HEADER_HEAP_BIN	(TAG_PRIMARY_HEADER | HEAP_BINARY_SUBTAG)
#define TAG_HEADER_SUB_BIN	(TAG_PRIMARY_HEADER | SUB_BINARY_SUBTAG)
#define TAG_HEADER_EXTERNAL_PID  (TAG_PRIMARY_HEADER | EXTERNAL_PID_SUBTAG)
#define TAG_HEADER_EXTERNAL_PORT (TAG_PRIMARY_HEADER | EXTERNAL_PORT_SUBTAG)
#define TAG_HEADER_EXTERNAL_REF  (TAG_PRIMARY_HEADER | EXTERNAL_REF_SUBTAG)
#define TAG_HEADER_BIN_MATCHSTATE (TAG_PRIMARY_HEADER | BIN_MATCHSTATE_SUBTAG)
#define TAG_HEADER_MAP	          (TAG_PRIMARY_HEADER|MAP_SUBTAG)

#define MASK_WORD(W) ((1 << (W))-1)

#define GET_PTR(term)   ((ERL_NIF_TERM*)((term) & ~MASK_WORD(TAG_PRIMARY_SIZE)))
#define MAKE_BOXED(ptr) (((ERL_NIF_TERM)(ptr)) | TAG_PRIMARY_BOXED)
#define MAKE_LIST(ptr)  (((ERL_NIF_TERM)(ptr)) | TAG_PRIMARY_LIST)
#define GET_LIST(term)  GET_PTR(term)
#define MAKE_NIL        ((((ERL_NIF_TERM) ~0) << TAG_IMMED2_SIZE) | TAG_IMMED2_NIL)
#define IS_LIST(term)   (((term) & MASK_WORD(TAG_PRIMARY_SIZE)) == TAG_PRIMARY_LIST)
#define IS_BOXED(term)    (((term) & MASK_WORD(TAG_PRIMARY_SIZE)) == TAG_PRIMARY_BOXED)
#define GET_BOXED(term)   GET_PTR(term)

#define MAKE_TUPLE(ptr)   MAKE_BOXED(ptr)
#define GET_TUPLE(term)   GET_BOXED(term)
#define MAKE_ARITYVAL(ari) (((ari)<<_HEADER_ARITY_OFFS) | TAG_HEADER_ARITYVAL)

#define IS_ARITYVAL(term) (((term) & _TAG_HEADER_MASK) == TAG_HEADER_ARITYVAL)
#define IS_TUPLE(term)    (IS_BOXED(term) && IS_ARITYVAL(*GET_BOXED(term)))

#define IS_FLOATVAL(term) (((term) & _TAG_HEADER_MASK) == TAG_HEADER_FLOAT)
#define IS_FLOAT(term)    (IS_BOXED(term) && IS_FLOATVAL(*GET_BOXED(term)))
#define MAKE_FLOAT(ptr)   MAKE_BOXED(ptr)
#define GET_FLOAT(term)   GET_BOXED(term)
#define MAKE_FLOATVAL(ari) (((ari)<<_HEADER_ARITY_OFFS) | TAG_HEADER_FLOAT)
#define GET_ARITYVAL(term) ((term) >> _HEADER_ARITY_OFFS)
#define IS_ATOM(term)     (((term) & TAG_IMMED2_MASK) == TAG_IMMED2_ATOM)
#define MAKE_ATOM(ptr)    (((ERL_NIF_TERM)(ptr)) | TAG_IMMED2_ATOM)
#define GET_ATOM(term)    ((atom_t*)((term) & ~MASK_WORD(TAG_IMMED2_SIZE)))
#define IS_SMALL(term)    (((term) & TAG_IMMED1_MASK) == TAG_IMMED1_SMALL)
#define MAKE_SMALL(val)   (((val) << TAG_IMMED1_SIZE) | TAG_IMMED1_SMALL)

#define MAKE_BIGNUM(ptr)   MAKE_BOXED(ptr)
#define MAKE_POS_BIGVAL(ari) (((ari)<<_HEADER_ARITY_OFFS) | TAG_HEADER_POS_BIG)
#define MAKE_NEG_BIGVAL(ari) (((ari)<<_HEADER_ARITY_OFFS) | TAG_HEADER_NEG_BIG)
#define IS_POS_BIGVAL(term) (((term) & _TAG_HEADER_MASK) == TAG_HEADER_POS_BIG)
#define IS_NEG_BIGVAL(term) (((term) & _TAG_HEADER_MASK) == TAG_HEADER_NEG_BIG)
#define IS_BIGVAL(term)   (((term) & (_TAG_HEADER_MASK-_BIG_SIGN_BIT)) == TAG_HEADER_POS_BIG)
#define IS_BIGNUM(term)   (IS_BOXED(term) && (IS_BIGVAL(*GET_BOXED(term))))

#define IS_REFC_BIN(term) (((term)&_TAG_HEADER_MASK)==TAG_HEADER_REFC_BIN)
#define IS_HEAP_BIN(term) (((term)&_TAG_HEADER_MASK)==TAG_HEADER_HEAP_BIN)
#define IS_SUB_BIN(term)  (((term)&_TAG_HEADER_MASK)==TAG_HEADER_SUB_BIN)
#define IS_BINVAL(term)    (IS_HEAP_BIN(term)||IS_REFC_BIN(term)||IS_SUB_BIN(term))
#define IS_BINARY(term)    (IS_BOXED(term) && IS_BINVAL(*GET_BOXED(term)))
#define GET_BINARY(term)  GET_BOXED(term)
#define MAKE_REFC_BINVAL(sz) (((sz)<<_HEADER_ARITY_OFFS)|TAG_HEADER_REFC_BIN)
#define MAKE_HEAP_BINVAL(sz) (((sz)<<_HEADER_ARITY_OFFS)|TAG_HEADER_HEAP_BIN)
#define MAKE_SUB_BINVAL(sz) (((sz)<<_HEADER_ARITY_OFFS)|TAG_HEADER_SUB_BIN)
#define MAKE_BINARY(ptr)   MAKE_BOXED(ptr)

// fixme: add more map types?
#define MAKE_MAPVAL(ari) (((ari)<<_HEADER_ARITY_OFFS) | TAG_HEADER_MAP)
#define IS_MAPVAL(term) (((term) & _TAG_HEADER_MASK) == TAG_HEADER_MAP)
#define IS_MAP(term)    (IS_BOXED(term) && IS_MAPVAL(*GET_BOXED(term)))
#define GET_MAP(term)   GET_BOXED(term)
#define MAKE_MAP(ptr)   MAKE_BOXED(ptr)

// the only 0 around is the arity value for {} so it will not occure
// as a return value. That goes for other HEADER tags as well
// We may use HEADER + various code to code for exceptions etc
#define INVALID_TERM  0

#define WORDSIZE __WORDSIZE   // from stdint better alternative?
#define NWORDS(bytes) (((bytes)+sizeof(ERL_NIF_TERM)-1)/sizeof(ERL_NIF_TERM))

// Global heap object
typedef struct _binary_t
{
    unsigned long flags;
    unsigned long refc;
    unsigned long orig_size;
    uint8_t orig_bytes[1];
} binary_t;

// Global atom object
typedef struct _atom_t 
{
    lhash_bucket_t bucket;
    size_t len;
    char*  name;
    char   data[];
} atom_t;

// on heap objects

typedef struct _refc_binary_t {
    ERL_NIF_TERM header;    // boxed header value
    ERL_NIF_UINT size;      // Size in bytes
    ERL_NIF_UINT next;      // link
    binary_t*    val;       // global data pointer
    uint8_t*     bytes;     // actual data bytes
    ERL_NIF_UINT flags;     // flag word
} refc_binary_t;

typedef struct _sub_binary_t {
    ERL_NIF_TERM header;    // boxed header value
    ERL_NIF_UINT size;      // Size in bytes
    ERL_NIF_UINT offs;      // Offset into original binary
    uint8_t bitsize; 
    uint8_t bitoffs; 
    uint8_t is_writable;    // The underlying binary is writable
    ERL_NIF_TERM orig;      // Original binary (REFC or HEAP binary).
} sub_binary_t;

typedef struct _heap_binary_t {
    ERL_NIF_TERM header;    // boxed header value
    ERL_NIF_UINT size;      // Size in bytes
    uint8_t      data[];    // binary data
} heap_binary_t;

typedef struct _flatmap_t {
    ERL_NIF_TERM header;  // +type?
    ERL_NIF_UINT size;    // number of key/value pairs
    ERL_NIF_TERM keys;    // tuple
    ERL_NIF_TERM value[]; // value[0..size-1]
} flatmap_t;

#endif

