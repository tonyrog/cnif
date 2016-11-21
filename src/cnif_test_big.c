//
//  Test integer
//

#include "../include/cnif_big.h"
#include "../include/cnif_stdio.h"

int main(int argc, char** argv)
{
    ErlNifEnv* env = enif_alloc_env();
    enif_io_t* iop = enif_stdio_alloc(env, NULL);
    ErlNifBignum a;
    ErlNifBignum b;
    ErlNifBignum c;
    int i;
    FILE* fin = stdin;
    char* ifile = "*stdin*";

    enif_io_push(iop, fin, ifile, 1, stdout, "*stdout*");

    enif_alloc_number(env, &a, 1);
    enif_alloc_number(env, &b, 10);
    enif_alloc_number(env, &c, 11);

    a.digits[0] = 1;

    for (i = 0; i < b.size; i++)
	b.digits[i] = (ErlNifBigDigit)(-1);

    if (!cnif_big_add(&a, &b, &c)) {
	enif_io_format(iop, "add failed\n");
    }
    cnif_big_write(iop, &c);
    enif_io_putc(iop, '\n');

    if (!cnif_big_sub(&c, &a, &c)) {
	enif_io_format(iop, "sub failed\n");
    }
    cnif_big_write(iop, &c);
    enif_io_putc(iop, '\n');

    enif_io_pop(iop);
    enif_clear_env(env);
    enif_free_env(env);
    exit(0);
}
