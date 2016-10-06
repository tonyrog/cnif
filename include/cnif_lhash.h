//
// Linear hash table
//
#ifndef __LHASH_H__
#define __LHASH_H__

#include <stdint.h>

typedef uint32_t lhash_value_t;

typedef int (*hcmp_fun_t)(void*, void*);
typedef lhash_value_t (*h_fun_t)(void*);
typedef void* (*halloc_fun_t)(void*);
typedef void (*hfree_fun_t)(void*);

typedef struct
{
    h_fun_t      hash;
    hcmp_fun_t   cmp;
    halloc_fun_t alloc;
    hfree_fun_t  free;
} lhash_methods_t;

typedef struct _lhash_bucket_t {
    struct _lhash_bucket_t* next;
    lhash_value_t hvalue;
} lhash_bucket_t;

typedef struct _lhash_table_t {
    const lhash_methods_t* fn;
    int is_allocated;
    char* name;

    int thres;        /* Medium bucket chain len, for grow */
    int szm;          /* current size mask */
    int nactive;      /* Number of "active" slots */
    int nslots;       /* Total number of slots */
    int nitems;       /* Total number of items */
    int p;            /* Split position */
    int nsegs;        /* Number of segments */
    lhash_bucket_t*** seg;
} lhash_t;

extern lhash_t* lhash_new(char*, int, const lhash_methods_t* fn);
extern lhash_t* lhash_init(lhash_t*, char*, int, const lhash_methods_t* fn);
extern void   lhash_free(lhash_t*);

extern void* lhash_get(lhash_t*, void*);
extern void* lhash_put(lhash_t*, void*);
extern void* lhash_erase(lhash_t*, void*);

#endif
