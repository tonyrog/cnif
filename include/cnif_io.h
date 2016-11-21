// Erlang term i/o
#ifndef __CNIF_IO_H__
#define __CNIF_IO_H__

#include <stdio.h>
#include <stdarg.h>

#include "cnif.h"

struct _enif_io_t;

typedef int (*enif_io_putc_t)(struct _enif_io_t* p, int c, void* arg);
typedef int (*enif_io_getc_t)(struct _enif_io_t* p, void* arg);
typedef int (*enif_io_ungetc_t)(struct _enif_io_t* p, int c, void* arg);
typedef int (*enif_io_close_t)(struct _enif_io_t* p, void* arg);
typedef int (*enif_io_format_t)(struct _enif_io_t* p, void* arg, char* fmt, va_list ap);
typedef int (*enif_io_callback_t)(struct _enif_io_t* p, ERL_NIF_TERM term);

typedef struct {
    enif_io_putc_t     putc;
    enif_io_format_t   format;
    enif_io_getc_t     getc;
    enif_io_ungetc_t   ungetc;
    enif_io_close_t    close;
} enif_io_methods_t;

typedef struct _enif_io_state_t
{
    int line;
    char* ifile;
    void* iarg;   // argument to getc/ungetc/close
    char* ofile;
    char* oarg;   // argument to putc/format/close
    char* error;
} enif_io_state_t;

#define ENIF_IO_MAX_DEPTH 10

typedef struct _enif_io_t {
    int sp;
    int base;                     // base for integer formating
    enif_io_state_t state[ENIF_IO_MAX_DEPTH];
    ErlNifEnv*     env;           // term data context
    void*          data;          // user data
    enif_io_callback_t callback;  // term callback
    enif_io_methods_t* meth;      // io methods
} enif_io_t;


static UNUSED inline char* enif_io_ifile(enif_io_t* p) 
{
    if (p->sp >= 0)
        return p->state[p->sp].ifile;
    return 0;
}

static UNUSED inline char* enif_io_ofile(enif_io_t* p) 
{
    if (p->sp >= 0)
        return p->state[p->sp].ofile;
    return 0;
}

static UNUSED inline int enif_io_line(enif_io_t* p) 
{
    if (p->sp >= 0)
        return p->state[p->sp].line;
    return 0;
}

static UNUSED inline char* enif_io_error(enif_io_t* p) 
{
    if (p->sp >= 0)
        return p->state[p->sp].error;
    return "";
}

static inline int enif_io_getc(enif_io_t* p)
{
    return p->meth->getc(p, p->state[p->sp].iarg);
}

static inline int enif_io_ungetc(int c, enif_io_t* p)
{
    return p->meth->ungetc(p, c, p->state[p->sp].iarg);
}

static inline int enif_io_putc(enif_io_t* p, int c)
{
    return p->meth->putc(p, c, p->state[p->sp].oarg);
}

static inline int enif_io_format(enif_io_t* p, char* fmt, ...)
{
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = p->meth->format(p, p->state[p->sp].oarg, fmt, ap);
    va_end(ap);
    return r;
}

static inline void enif_io_set_callback(enif_io_t* p, enif_io_callback_t cb)
{
    p->callback = cb;
}

ERL_NIF_API_FUNC_DECL(enif_io_t*, enif_io_alloc, (ErlNifEnv* env, enif_io_methods_t* meth, void* data));
ERL_NIF_API_FUNC_DECL(void, enif_io_free, (enif_io_t*));
ERL_NIF_API_FUNC_DECL(int, enif_io_push, (enif_io_t*,void* iarg,char* ifile,int line,void* oarg,char* ofile));
ERL_NIF_API_FUNC_DECL(int, enif_io_pop, (enif_io_t*));
ERL_NIF_API_FUNC_DECL(int, enif_io_scan_forms, (enif_io_t*));
ERL_NIF_API_FUNC_DECL(void, enif_io_set_error, (enif_io_t*, char* err));

ERL_NIF_API_FUNC_DECL(void,enif_io_write_integer,(enif_io_t*,ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(void,enif_io_write_number,(enif_io_t*,ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(void,enif_io_write_atom,(enif_io_t*,ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(void,enif_io_write_list,(enif_io_t*,ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(void,enif_io_write_tuple,(enif_io_t*,ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(void,enif_io_write_map,(enif_io_t*,ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(void,enif_io_write_binary,(enif_io_t*,ERL_NIF_TERM term));
ERL_NIF_API_FUNC_DECL(void,enif_io_write,(enif_io_t*,ERL_NIF_TERM term));

#endif
