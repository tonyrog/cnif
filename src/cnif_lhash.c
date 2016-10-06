//
// Linear hash table
//
#include <stdlib.h>
#include <memory.h>
#include "../include/cnif_lhash.h"

#define SZEXP   8
#define SEGSZ   (1 << SZEXP)
#define SZMASK  ((1 << SZEXP)-1)

#define SEG_LEN         256   /* When growing init segs */
#define SEG_INCREAMENT  128   /* Number of segments to grow */

#define BUCKET(lh, i) (lh)->seg[(i) >> SZEXP][(i) & SZMASK]

#define HASH(lh, hval, ix) \
  do { \
     if ((ix = ((hval) & (lh)->szm)) < (lh)->p) \
        ix = (hval) & (((lh)->szm << 1) | 1); \
  } while(0)

static lhash_bucket_t** alloc_seg()
{
    lhash_bucket_t** bp;
    int sz = sizeof(lhash_bucket_t*)*SEGSZ;

    bp = (lhash_bucket_t**) malloc(sz);
    memset(bp, 0, sz);
    return bp;
}

lhash_t* lhash_init(lhash_t* lh,char* name,int thres,const lhash_methods_t* fn)
{
    lh->fn = fn;
    lh->is_allocated = 0;
    lh->name = name;
    lh->thres = thres;
    lh->szm = SZMASK;
    lh->nslots = SEGSZ;
    lh->nactive = SEGSZ;
    lh->nitems = 0;
    lh->p = 0;
    lh->nsegs = 1;
    lh->seg = (lhash_bucket_t***) malloc(sizeof(lhash_bucket_t**));
    lh->seg[0] = alloc_seg();
    return lh;
}

static void grow(lhash_t* lh)
{
    lhash_bucket_t** bp;
    lhash_bucket_t** bps;
    lhash_bucket_t* b;
    int ix;
    int nszm = (lh->szm << 1) | 1;

    if (lh->nactive >= lh->nslots) {
	/* Time to get a new array */
	if ((lh->nactive & SZMASK) == 0) {
	    int six = lh->nactive >> SZEXP;
	    if (six == lh->nsegs) {
		int i, sz;

		if (lh->nsegs == 1)
		    sz = SEG_LEN;
		else
		    sz = lh->nsegs + SEG_INCREAMENT;
		lh->seg = (lhash_bucket_t***) realloc(lh->seg,
						   sizeof(lhash_bucket_t**)*sz);
		lh->nsegs = sz;
		for (i = six+1; i < sz; i++)
		    lh->seg[i] = 0;
	    }
	    lh->seg[six] = alloc_seg();
	    lh->nslots += SEGSZ;
	}
    }

    ix = lh->p;
    bp = &BUCKET(lh, ix);
    ix += (lh->szm+1);
    bps = &BUCKET(lh, ix);
    b = *bp;

    while (b != 0) {
	ix = b->hvalue & nszm;

	if (ix == lh->p)
	    bp = &b->next;          /* object stay */
	else {
	    *bp = b->next;  	    /* unlink */
	    b->next = *bps;         /* link */
	    *bps = b;
	}
	b = *bp;
    }

    lh->nactive++;
    if (lh->p == lh->szm) {
	lh->p = 0;
	lh->szm = nszm;
    }
    else
	lh->p++;
}

// Shrink the hash table and remove segments if they are empty
// but do not reallocate the segment index table !!!
static void shrink(lhash_t* lh)
{
    lhash_bucket_t** bp;

    if (lh->nactive == SEGSZ)
	return;

    lh->nactive--;
    if (lh->p == 0) {
	lh->szm >>= 1;
	lh->p = lh->szm;
    }
    else
	lh->p--;

    bp = &BUCKET(lh, lh->p);
    while(*bp != 0) bp = &(*bp)->next;

    *bp = BUCKET(lh, lh->nactive);
    BUCKET(lh, lh->nactive) = 0;

    if ((lh->nactive & SZMASK) == SZMASK) {
	int six = (lh->nactive >> SZEXP)+1;

	free(lh->seg[six]);
	lh->seg[six] = 0;
	lh->nslots -= SEGSZ;
    }
}

lhash_t* lhash_new(char* name, int thres, const lhash_methods_t* fn)
{
    lhash_t* tp;

    tp = (lhash_t*) malloc(sizeof(lhash_t));
    
    if (lhash_init(tp, name, thres, fn) == NULL) {
	free(tp);
	return NULL;
    }
    tp->is_allocated = 1;
    return tp;
}

void lhash_free(lhash_t* lh)
{
    lhash_bucket_t*** sp = lh->seg;
    int n = lh->nsegs;

    while(n--) {
	lhash_bucket_t** bp = *sp;
	if (bp != 0) {
	    int m = SEGSZ;

	    while(m--) {
		lhash_bucket_t* p = *bp++;

		while(p != 0) {
		    lhash_bucket_t* next = p->next;
		    (*lh->fn->free)((void*) p);
		    p = next;
		}
	    }
	    free(*sp);
	}
	sp++;
    }
    free(lh->seg);

    if (lh->is_allocated)
	free(lh);
}

void* lhash_put(lhash_t* lh, void* tmpl)
{
    lhash_value_t hval = lh->fn->hash(tmpl);
    int ix;
    lhash_bucket_t** bpp;
    lhash_bucket_t* b;

    HASH(lh, hval, ix);
    bpp = &BUCKET(lh, ix);

    b = *bpp;
    while(b != (lhash_bucket_t*) 0) {
	if ((b->hvalue == hval) && (lh->fn->cmp(tmpl, (void*) b) == 0))
	    return (void*) b;
	b = b->next;
    }
    b = (lhash_bucket_t*) lh->fn->alloc(tmpl);
    b->hvalue = hval;
    b->next = *bpp;
    *bpp = b;
    lh->nitems++;

    if ((lh->nitems / lh->nactive) >= lh->thres)
	grow(lh);
    return (void*) b;
}

void* lhash_get(lhash_t* lh, void* tmpl)
{
    lhash_value_t hval = lh->fn->hash(tmpl);
    int ix;
    lhash_bucket_t** bpp;
    lhash_bucket_t* b;

    HASH(lh, hval, ix);
    bpp = &BUCKET(lh, ix);

    b = *bpp;
    while(b != (lhash_bucket_t*) 0) {
	if ((b->hvalue == hval) && (lh->fn->cmp(tmpl, (void*) b) == 0))
	    return (void*) b;
	b = b->next;
    }
    return NULL;
}

// Erase an item
void* lhash_erase(lhash_t* lh, void* tmpl)
{
    lhash_value_t hval = lh->fn->hash(tmpl);
    int ix;
    lhash_bucket_t** bpp;
    lhash_bucket_t* b;

    HASH(lh, hval, ix);
    bpp = &BUCKET(lh, ix);

    b = *bpp;
    while(b != (lhash_bucket_t*) 0) {
	if ((b->hvalue == hval) && (lh->fn->cmp(tmpl, (void*) b) == 0)) {
	    *bpp = b->next;  /* unlink */
	    lh->fn->free((void*) b);
	    lh->nitems--;
	    if ((lh->nitems / lh->nactive) < lh->thres)
		shrink(lh);
	    return tmpl;
	}
	bpp = &b->next;
	b = b->next;
    }
    return NULL;    
}
