//
//  C library for representing erlang terms using NIF api
//
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>

#include "../include/cnif.h"
#include "../include/cnif_io.h"
#include "../include/cnif_stdio.h"
#include "../include/cnif_misc.h"

#define DBG(...) printf(__VA_ARGS__)

static int term_callback(enif_io_t* p, ERL_NIF_TERM term)
{
    printf("TERM = [");
    enif_io_write(p, term);
    printf("]\n");
    return 1;
}

#define ARRAY_SIZE 10
#define MAP_SIZE   10

void print_element(ErlNifEnv* env, int i, ERL_NIF_TERM value, void* arg)
{
    enif_io_t* iop = (enif_io_t*) arg;
    enif_io_write(iop, value);
    enif_io_putc(iop, '\n');
}

int main(int argc, char** argv) 
{
    ErlNifEnv* env = enif_alloc_env();
    ERL_NIF_TERM arr[ARRAY_SIZE];
    ERL_NIF_TERM key[MAP_SIZE];
    ERL_NIF_TERM value[MAP_SIZE];
    ERL_NIF_TERM t,v,w;
    unsigned i;
    enif_io_t* iop = enif_stdio_alloc(env, NULL);
    FILE* fin = stdin;
    char* ifile = "*stdin*";

    if (argc > 1) {
	ifile = argv[1];
	if ((fin = fopen(ifile, "r")) == NULL) {
	    fprintf(stderr, "unable to open %s\n", ifile);
	    exit(1);
	}
    }
    enif_io_push(iop, fin, ifile, 1, stdout, "*stdout*");

    for (i = 0; i < ARRAY_SIZE; i++)
	arr[i] = enif_make_uint(env, i+1);

    t = enif_make_list_from_array(env, arr, ARRAY_SIZE);
    t = enif_make_list_cell(env, enif_make_uint(env, 0), t);
    enif_io_write(iop, t); printf("\n");

    enif_each_element(env, print_element, t, iop);

    enif_make_reverse_list(env, t, &t);
    enif_io_write(iop, t); printf("\n");    

    t = enif_make_tuple_from_array(env, arr, ARRAY_SIZE);
    enif_io_write(iop, t); printf("\n");

    arr[0] = enif_make_string(env, "hello", ERL_NIF_LATIN1);
    arr[1] = enif_make_string(env, "world", ERL_NIF_LATIN1);
    t = enif_make_tuple_from_array(env, arr, 2);
    enif_io_write(iop, t); printf("\n");
    

    arr[0] = enif_make_atom(env, "false");
    arr[1] = enif_make_atom(env, "true");
    arr[2] = enif_make_atom(env, "hello");
    arr[3] = enif_make_atom(env, "world");
    arr[4] = enif_make_double(env, 3.14);
    arr[5] = enif_make_uint64(env, UINT64_C(1));
    arr[6] = enif_make_uint64(env, UINT64_C(0xffffffffffffffff));
    arr[7] = enif_make_int64(env,   INT64_C(0x7fffffffffffffff));
    arr[8] = enif_make_uint64(env, UINT64_C(0x1234567812345678));
    arr[9] = enif_make_int64(env,   INT64_C(0x7fffffffffffffff));

    memcpy(enif_make_new_binary(env, 10, &arr[10]),
	   "01234567789", 10);

    t = enif_make_tuple_from_array(env, arr, 11);
    enif_io_write(iop, t); printf("\n");

    enif_each_element(env, print_element, t, iop);

    key[0] = enif_make_atom(env, "a");
    key[1] = enif_make_atom(env, "b");
    key[2] = enif_make_atom(env, "b");
    key[3] = enif_make_atom(env, "c");
    key[4] = enif_make_atom(env, "c");
    key[5] = enif_make_atom(env, "c");
    key[6] = enif_make_atom(env, "d");

    value[0] = enif_make_int(env, 1);
    value[1] = enif_make_int(env, 2);
    value[2] = enif_make_int(env, 3);
    value[3] = enif_make_int(env, 4);
    value[4] = enif_make_int(env, 5);
    value[5] = enif_make_int(env, 6);
    value[6] = enif_make_int(env, 7);
    t = enif_make_map_from_arrays(env, key, value, 7);
    enif_io_write(iop, t); printf("\n");

    if (enif_make_map_update(env, t, enif_make_atom(env,"a"),
			     enif_make_int(env, 1000),&w)) {
	printf("updated a =");
	enif_io_write(iop, w); printf("\n");

	if (enif_make_map_remove(env, w, enif_make_atom(env,"a"),&w)) {
	    printf("remove a =");
	    enif_io_write(iop, w); printf("\n");
	}
	else
	    fprintf(stdout, "failed to remove 'a' from map\n");
    }
    else
	fprintf(stdout, "failed to update map\n");

    if (enif_get_map_value(env, t, enif_make_atom(env, "b"), &v)) {
	fprintf(stdout, "get(b) = ");
	enif_io_write(iop, v); printf("\n");	
    }
    else {
	fprintf(stdout, "get(b) = ERROR\n");
    }

    t = enif_make_map_from_arrays(env, key, value, 0);
    enif_io_write(iop, t); printf("\n");

    key[0] = enif_make_atom(env, "x");
    value[0] = enif_make_int(env, 0);
    t = enif_make_map_from_arrays(env, key, value, 1);
    enif_io_write(iop, t); printf("\n");

    // create io-list print and make a binary
    {
	ErlNifBinary b;
	ERL_NIF_TERM tb;

	arr[0] = enif_make_string(env, "hello", ERL_NIF_LATIN1);
	memcpy(enif_make_new_binary(env, 10, &arr[1]), "01234567789", 10);
	arr[2] = enif_make_string(env, "world", ERL_NIF_LATIN1);
	t = enif_make_list_from_array(env, arr, 3);
	enif_io_write(iop, t); printf("\n");

	if (enif_inspect_iolist_as_binary(env, t, &b)) {
	    printf("iolist size = %zu\n", b.size);
	    tb = enif_make_binary(env, &b);
	    enif_io_write(iop, tb); printf("\n");	    
	}
    }

    // create term and copy-recurive/flat/struct
    {
	ERL_NIF_TERM t_copy;
	ERL_NIF_TERM t_flat;
	ERL_NIF_TERM t_struct;
	
	arr[0] = enif_make_string(env, "hello", ERL_NIF_LATIN1);
	memcpy(enif_make_new_binary(env, 10, &arr[1]), "01234567789", 10);
	arr[2] = enif_make_string(env, "world", ERL_NIF_LATIN1);
	arr[3] = arr[2];
	arr[4] = enif_make_tuple_from_array(env, arr, 3);
	arr[5] = arr[1];
	arr[6] = arr[0];
	arr[7] = arr[4];
	t = enif_make_list_from_array(env, arr, 8);
	enif_io_write(iop, t); printf("\n");

	printf("term size = %lu\n", enif_flat_size(t));
	t_copy = enif_make_copy(env, t);
	t_flat = enif_make_flat_copy(env, t);
	t_struct = enif_make_struct_copy(env, t);

	printf("size of t_copy = %lu\n", enif_flat_size(t_copy));
	enif_io_write(iop, t_copy); printf("\n");
	printf("size of t_flat = %lu\n", enif_flat_size(t_flat));
	enif_io_write(iop, t_flat); printf("\n");
	printf("size of t_struct = %lu\n", enif_flat_size(t_struct));
	enif_io_write(iop, t_struct); printf("\n");

	printf("original t again\n");
	enif_io_write(iop, t); printf("\n");
    }

    // Test stream a erlang consult file
    if (argc > 1) {
	enif_io_set_callback(iop, term_callback);

	if (!enif_io_scan_forms(iop)) {
	    fprintf(stderr, "%s:%d: error %s\n", 
		    argv[1], enif_io_line(iop), enif_io_error(iop));
	}
    }

    enif_io_pop(iop);

    enif_clear_env(env);

    enif_free_env(env);

    exit(0);
}
