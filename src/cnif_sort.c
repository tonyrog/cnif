//
// Sorting functions
//

#include "../include/cnif.h"
#include "../include/cnif_sort.h"

int cnif_is_sorted(const ERL_NIF_TERM* arr, size_t n)
{
    int i;
    for (i = 0; i < (int)n-1; i++) {
	if (enif_compare(arr[i],arr[i+1]) > 0)
	    return 0;
    }
    return 1;
}

int cnif_is_usorted(const ERL_NIF_TERM* arr, size_t n)
{
    int i;
    for (i = 0; i < (int)n-1; i++) {
	if (enif_compare(arr[i],arr[i+1]) >= 0)
	    return 0;
    }
    return 1;
}

static inline void swap(ERL_NIF_TERM* arr, int i, int j)
{
    ERL_NIF_TERM tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
}

// inline quick sort
void cnif_inline_quick_sort_aux(ERL_NIF_TERM* src1,ERL_NIF_TERM* src2,
				int left,int right)
{
    int i=left, j=right;
    ERL_NIF_TERM tmp;
    ERL_NIF_TERM pivot = src1[(left+right)/2];

    // partition
    while (i <= j) {
	while(enif_compare(src1[i],pivot) < 0)
	    i++;
	while(enif_compare(src1[j],pivot) > 0)
	    j--;
	if (i <= j) {
	    swap(src1, i, j);
	    if (src2) swap(src2, i, j);
	    i++;
	    j--;
	}
    };
    if (left < j)
	cnif_inline_quick_sort_aux(src1,src2,left,j);
    if (i < right)
	cnif_inline_quick_sort_aux(src1,src2,i,right);
}

// remove duplicates from src1 and corresponding element in src2
static int uremove_aux(ERL_NIF_TERM* src1,ERL_NIF_TERM* src2,int left,int right)
{
    int i=left, j=right;
    for (j = left+1; j <= right; j++) {
	if (enif_compare(src1[i],src1[j]) < 0) {
	    i++;
	    if (i < j+1) {
		src1[i] = src1[j];
		if (src2)
		    src2[i] = src2[j];
	    }
	}
    }
    return i+1;
}

int cnif_inline_quick_usort_aux(ERL_NIF_TERM* src1,ERL_NIF_TERM* src2,
				int left,int right)
{
    cnif_inline_quick_sort_aux(src1, src2, left, right);
    return uremove_aux(src1, src2, left, right);
}


// first partition quick sort and move to destination
void cnif_quick_sort_aux(ERL_NIF_TERM* src1,ERL_NIF_TERM* src2,
			 ERL_NIF_TERM* dst1,ERL_NIF_TERM* dst2,
			 int left, int right)
{
    if (src1 == dst1) dst1 = NULL;
    if (src2 == dst2) dst2 = NULL;
    if (src1 && dst1) {
	int i=left, j=right;
	ERL_NIF_TERM tmp;
	ERL_NIF_TERM pivot = src1[(left+right)/2];
	if (src2 && dst2) {
	    // partition & copy
	    while (i <= j) {
		while(enif_compare(src1[i],pivot) < 0) {
		    dst1[i] = src1[i];
		    dst2[i] = src2[i];
		    i++;
		}
		while(enif_compare(src1[j],pivot) > 0) {
		    dst1[j] = src1[j];
		    dst2[j] = src2[j];
		    j--;
		}
		if (i <= j) {
		    dst1[i] = src1[j];
		    dst1[j] = src1[i];
		    dst2[i] = src2[j];
		    dst2[j] = src2[i];
		    i++;
		    j--;
		}
	    }
	}
	else {
	    // partition & copy
	    while (i <= j) {
		while(enif_compare(src1[i],pivot) < 0) {
		    dst1[i] = src1[i];
		    i++;
		}
		while(enif_compare(src1[j],pivot) > 0) {
		    dst1[j] = src1[j];
		    j--;
		}
		if (i <= j) {
		    dst1[i] = src1[j];
		    dst1[j] = src1[i];
		    i++;
		    j--;
		}
	    }
	}
	if (left < j)
	    cnif_inline_quick_sort_aux(dst1,dst2,left,j);
	if (i < right)
	    cnif_inline_quick_sort_aux(dst1,dst2,i,right);
    }
    else if (src1)
	cnif_inline_quick_sort_aux(src1,src2,left,right);
}

void cnif_quick_sort(ERL_NIF_TERM* src1,ERL_NIF_TERM* dst1, int left, int right)
{
    cnif_quick_sort_aux(src1, NULL, dst1, NULL, left, right);
}

int cnif_quick_usort_aux(ERL_NIF_TERM* src1,ERL_NIF_TERM* src2,
			 ERL_NIF_TERM* dst1,ERL_NIF_TERM* dst2,
			 int left, int right)
{
    cnif_quick_sort_aux(src1, src2, dst1, dst2, left, right);
    if (dst1 && (src1 != dst1))
	return uremove_aux(dst1, dst2, left, right);
    else
	return uremove_aux(src1, src2, left, right);
}

int cnif_quick_usort(ERL_NIF_TERM* src1,ERL_NIF_TERM* dst1,int left,int right)
{
    return cnif_quick_usort_aux(src1, NULL, dst1, NULL, left, right);
}
