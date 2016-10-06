#ifndef __CNIF_SORT_H__
#define __CNIF_SORT_H__

#include "cnif.h"

ERL_NIF_API_FUNC_DECL(int,cnif_is_sorted,(const ERL_NIF_TERM* arr, size_t n));
ERL_NIF_API_FUNC_DECL(int,cnif_is_usorted,(const ERL_NIF_TERM* arr, size_t n));
ERL_NIF_API_FUNC_DECL(void,cnif_inline_quick_sort_aux,(ERL_NIF_TERM* src1,
						       ERL_NIF_TERM* src2,
						       int left,int right));
ERL_NIF_API_FUNC_DECL(int,cnif_inline_quick_usort_aux,(ERL_NIF_TERM* src1,
						       ERL_NIF_TERM* src2,
						       int left,int right));
ERL_NIF_API_FUNC_DECL(void,cnif_quick_sort_aux,(ERL_NIF_TERM* src1,
						ERL_NIF_TERM* src2,
						ERL_NIF_TERM* dst1,
						ERL_NIF_TERM* dst2,
						int left, int right));
ERL_NIF_API_FUNC_DECL(void,cnif_quick_sort,(ERL_NIF_TERM* src1,
					    ERL_NIF_TERM* dst1,
					    int left, int right));
ERL_NIF_API_FUNC_DECL(int,cnif_quick_usort_aux,(ERL_NIF_TERM* src1,
						ERL_NIF_TERM* src2,
						ERL_NIF_TERM* dst1,
						ERL_NIF_TERM* dst2,
						int left, int right));

ERL_NIF_API_FUNC_DECL(int,cnif_quick_usort,(ERL_NIF_TERM* src1,
					    ERL_NIF_TERM* dst1,
					    int left,int right));

#endif
