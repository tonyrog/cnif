#include <stdarg.h>
#include "../include/cnif_io.h"

static int stdio_format(enif_io_t* p, void* arg, char* fmt, va_list ap)
{
    return vfprintf((FILE*) arg, fmt, ap);
}

static int stdio_getc(enif_io_t* p, void* arg)
{
    return fgetc((FILE*)arg);
}

static int stdio_ungetc(enif_io_t* p, int c, void* arg)
{
    return ungetc(c, (FILE*)arg);
}

static int stdio_putc(enif_io_t* p, int c, void* arg)
{
    return fputc(c, (FILE*)arg);
}

static int stdio_close(enif_io_t* p, void* arg)
{
    FILE* f = (FILE*) arg;
    if ((f == stdin) || (f == stdout) || (f == stderr))
	return 0;
    return fclose(f);
}

enif_io_methods_t stdio_meth = 
{
    .putc     = stdio_putc,
    .format   = stdio_format,
    .getc     = stdio_getc,
    .ungetc   = stdio_ungetc,
    .close    = stdio_close,
};

enif_io_t* enif_stdio_alloc(ErlNifEnv* env, void* data)
{
    return enif_io_alloc(env, &stdio_meth, data);
}
