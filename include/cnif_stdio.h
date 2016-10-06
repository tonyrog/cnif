#ifndef __CNIF_STDIO_H__
#define __CNIF_STDIO_H__

#include "cnif_io.h"

ERL_NIF_API_FUNC_DECL(enif_io_t*, enif_stdio_alloc, (ErlNifEnv* env, void* data));

#endif
