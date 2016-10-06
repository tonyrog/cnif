#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../include/cnif.h"
#include "../include/cnif_big.h"
#include "../include/cnif_io.h"
#include "../include/cnif_misc.h"

#define MAX_VAR_LEN    255
#define MAX_ATOM_LEN   255
#define MAX_STRING_LEN 1024
#define MAX_TUPLE_LEN  1024
#define MAX_NUM_LEN    1024
#define MAX_MAP_LEN    1024
#define DEF_BIN_SIZE   64
#define INC_BIN_SIZE   1024
#define ERROR 0

static char* string_dup(char* str)
{
    char *copy = enif_alloc(strlen(str)+1);
    strcpy(copy, str);
    return copy;
}

int enif_io_push(enif_io_t* p, void* iarg, char* ifile, int line,
		 void* oarg, char* ofile)
{
    p->sp++;
    p->state[p->sp].ifile = string_dup(ifile);
    p->state[p->sp].iarg = iarg;
    p->state[p->sp].ofile = string_dup(ofile);
    p->state[p->sp].oarg = oarg;
    p->state[p->sp].line = line;
    p->state[p->sp].error = 0;
    return 0;
}

int enif_io_pop(enif_io_t* p)
{
    p->meth->close(p, p->state[p->sp].iarg);
    enif_free(p->state[p->sp].ifile);
    p->meth->close(p, p->state[p->sp].oarg);
    enif_free(p->state[p->sp].ofile);
    p->sp--;
    return 0;
}

enif_io_t* enif_io_alloc(ErlNifEnv* env, enif_io_methods_t* meth, void* data)
{
    enif_io_t* p = enif_alloc(sizeof(enif_io_t));
    p->sp    = -1;
    p->env   = env;
    p->data  = data;
    p->meth  = meth;
    p->base  = 10;
    return p;
}

void enif_io_free(enif_io_t* p)
{
    while(p->sp >= 0)
	enif_io_pop(p);
    enif_free(p);
}

void enif_io_set_error(enif_io_t* p, char* err)
{
    p->state[p->sp].error = err;
}

char* enif_io_parser_error(enif_io_t* p)
{
    if (p->state[p->sp].error)
	return p->state[p->sp].error;
    return "ok";
}


//
// syntax:
//   [ (element '.')* ]
// element:
//      tuple
//    | list
//    | string 
//    | number
//    | atom
//    | binary
//    | map
//
// list: '[' elements ']'
// tuple: '{' elements '}'
// binary: '<''<' bytes '>''>'
// map:    '#''{' pairs '}'
//
// elements: empty
//       |   element
//       |   elements ',' element
//
// pairs: empty
//       |   pair
//       |   pairs ',' pair
// pair: element '=' '>' element
//
// bytes: empty
//       |   byte
//       |   bytes ',' byte
//

static int skip_line(enif_io_t* p)
{
    int c;
    while((c = enif_io_getc(p)) >= 0) {
	switch(c) {
	case '\n':
	    p->state[p->sp].line++;
	    return c;
	default:
	    continue;
	}
    }
    return c;
}

static int skip_blank(enif_io_t* p)
{
    int c;
    while((c = enif_io_getc(p)) >= 0) {
	switch(c) {
	case ' ':
	case '\t':
	case '\r':
	    break;
	case '\n':
	    p->state[p->sp].line++;
	    break;
	case '%':
	    skip_line(p);
	    break;
	default:
	    return c;
	}
    }
    return c;
}

//
// SEEN digit ['+'|'-'] [dd'#']d*
//            ['+'|'-'] d*['.' d*]
//
static ERL_NIF_TERM parse_number(enif_io_t* p, int sign, int c)
{
    char      buf[MAX_NUM_LEN];
    int       base = 10;
    int       base_set = 0;
    int       exp_ds  = -1;
    int64_t   num  = 0;
    int       ds = 0;
    int       i = 0;

    if (sign) {
	buf[i++] = (sign < 0) ? '-' : '+';
    }
    if (c != 0) {
	buf[i++] = c;
	num = (c - '0');
	ds++;
    }
    while((c = enif_io_getc(p)) >= 0) {
	int d;
	if (i >= (MAX_NUM_LEN-1)) {
	    enif_io_set_error(p, "number too big");
	    return ERROR;
	}
	buf[i++] = c;
	if (c == '#') {
	    if (!base_set && (num > 1) && (num < 37)) {
		base = num;
		base_set = 1;
		num = 0;
		ds  = 0;
		continue;
	    }
	    else {
		enif_io_set_error(p, "illegal integer");
		return ERROR;
	    }
	}
	if (c == '.') {
	    if (base_set) {
		enif_io_set_error(p, "illegal number");
		return ERROR;
	    }
	    if (((c = enif_io_getc(p)) != EOF) && isdigit(c)) {
		enif_io_ungetc(c, p);
		goto scan_float;
	    }
	    enif_io_ungetc(c, p);
	    enif_io_ungetc('.', p);
	    goto return_integer;
	}

	if ((c >= '0') && (c <= '9'))
	    d = c - '0';
	else if ((c >= 'a') && (c <= 'z'))
	    d = (c - 'a') + 10;
	else if ((c >= 'A') && (c <= 'Z'))
	    d = (c - 'A') + 10;
	else {
	    enif_io_ungetc(c, p);
	    goto return_integer;
	}
	if (d < base) {
	    num = num*base + d;
	    ds++;
	}
	else {
	    enif_io_ungetc(c, p);
	    goto return_integer;
	}
    }
    if (ds > 0)
	goto return_integer;
    enif_io_set_error(p, "illegal integer");
    return ERROR;

scan_float:
    while((c = enif_io_getc(p)) >= 0) {
	if (i >= (MAX_NUM_LEN-1)) {
	    enif_io_set_error(p, "number too big");
	    return 0;
	}
	if (((c == 'e') || (c == 'E')) && (exp_ds < 0)) {
	    buf[i++] = c;
	    exp_ds = 0;
	}
	else if ((c == '-') && (exp_ds == 0)) {
	    buf[i++] = '-';
	}
	else if ((c >= '0') && (c <= '9')) {
	    buf[i++] = c;
	    if (exp_ds >= 0)
		exp_ds++;
	}
	else {
	    enif_io_ungetc(c, p);
	    goto return_float;
	}
    }

return_float: {
	char* endptr = NULL;
	double fnum;

	buf[i] = '\0';
	// printf("FLOAT = [%s]\n", buf);
	fnum = strtod(buf, &endptr);
	if (*endptr != '\0') {
	    enif_io_set_error(p, "illegal float");
	    return ERROR;
	}
	return enif_make_double(p->env, fnum);
    }

return_integer:
    if (sign < 0) num = -num;
    return enif_make_int64(p->env, num);    
}


static ERL_NIF_TERM parse_element(enif_io_t* p);

static int parse_qchar(enif_io_t* p)
{
    int c;
    switch((c = enif_io_getc(p))) {
    case EOF: return EOF;
    case '\\':
	switch((c = enif_io_getc(p))) {
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 's': return ' ';
	case 'b': return '\b';
	case 'f': return '\f';
	default: 
	    if ((c >= '0') && (c <= '3')) {
		int n = c - '0';
		if ((c = enif_io_getc(p)) == EOF)
		    return n;
		if ((c >= '0') && (c <= '7')) {
		    n = n*8 + (c-'0');
		    if ((c = enif_io_getc(p)) == EOF)
			return n;
		    if ((c >= '0') && (c <= '7')) {
			n = n*8 + (c-'0');
			return n;
		    }
		}
		enif_io_ungetc(c, p);
		return n;
	    }
	    return c;
	}
	break;
    default:
	return c;
    }
}

// load 
static int parse_quoted_string_buf(enif_io_t* p, char* buf, 
				   int offs, int max_offs, int* res_offs)
{
    int c;

    while((c = parse_qchar(p)) >= 0) {
	if (c == '\"') {
	    *res_offs = offs;
	    return 1;
	}
	if (offs >= max_offs) {
	    enif_io_set_error(p, "string too long");
	    return 0;
	}
	buf[offs++] = c;
    }
    enif_io_set_error(p, "string not terminated");
    return 0;
}

// SEEN " parse until "
static ERL_NIF_TERM parse_quoted_string(enif_io_t* p)
{
    char buf[MAX_STRING_LEN];
    int i = 0;

    if (!parse_quoted_string_buf(p, buf, 0, MAX_STRING_LEN, &i)) 
	return ERROR;
    return enif_make_string_len(p->env, buf, i, ERL_NIF_LATIN1);
}

// ' (chars)* '
static ERL_NIF_TERM parse_quoted_atom(enif_io_t* p)
{
    char buf[MAX_ATOM_LEN];
    int i = 0;
    int c;
    
    while((c = parse_qchar(p)) >= 0) {
	if (c == '\'')
	    return enif_make_atom_len(p->env, buf, i);
	if (i >= MAX_ATOM_LEN) {
	    enif_io_set_error(p, "atom name too long");
	    return ERROR;
	}
	buf[i++] = c;
    }
    enif_io_set_error(p, "atom not terminated");
    return ERROR;
}


// SEEN { - parse '{' [element [(',' element)]*  '}'
static ERL_NIF_TERM parse_tuple(enif_io_t* p)
{
    ERL_NIF_TERM elems[MAX_TUPLE_LEN];
    int i = 0;
    int c;

    if ((c = skip_blank(p)) == '}')
	goto build;
    if (c == EOF) {
	enif_io_set_error(p, "missing '}'");
	return ERROR;
    }
    enif_io_ungetc(c, p);

    while(i < MAX_TUPLE_LEN) {
	if (!(elems[i] = parse_element(p)))
	    return ERROR;
	i++;
	if ((c = skip_blank(p)) == '}')
	    goto build;
	if (c != ',') {
	    enif_io_set_error(p, "missing '}'");
	    return ERROR;
	}
    }
    enif_io_set_error(p, "tuple too long");
    return ERROR;
build:
    return enif_make_tuple_from_array(p->env, elems, i);
}

// SEEN '['  parse '[' [ element ([(',' element)]* '|' element) ]  ']'
// return NIl | LIST
static ERL_NIF_TERM parse_list(enif_io_t* p)
{
    ERL_NIF_TERM acc;
    ERL_NIF_TERM tail = 0;
    int c;

    if ((c = skip_blank(p)) == ']')
	return enif_make_list0(p->env);
    if (c == EOF) {
	enif_io_set_error(p, "missing ']'");
	return ERROR;
    }
    enif_io_ungetc(c, p);

    acc = enif_make_list0(p->env);
    while(!tail) {
	ERL_NIF_TERM e;
	if (!(e = parse_element(p)))
	    return ERROR;
	acc = enif_make_list_cell(p->env, e, acc);
	if ((c = skip_blank(p)) == ']')
	    tail = enif_make_list0(p->env);
	else if (c == '|') {
	    if (!(tail = parse_element(p)))
		return ERROR;
	    if ((c = skip_blank(p)) != ']') {
		enif_io_set_error(p, "missing ']'");
		return ERROR;
	    }
	}
	else if (c != ',') {
	    enif_io_set_error(p, "missing ']'");
	    return ERROR;
	}
    }
    if (enif_inline_reverse_list(p->env, acc, tail, &acc)) {
	return acc;
    }
    return ERROR;
}

// SEEN '<'  <  elem [, elem]* >> 
// elem = byte | string
// return ERROR | BINARY
static ERL_NIF_TERM parse_binary(enif_io_t* p)
{
    ErlNifBinary bin;
    size_t size = DEF_BIN_SIZE;
    int i = 0;
    int c;

    if (enif_io_getc(p) != '<') {
	enif_io_set_error(p, "missing '<<'");
	return ERROR;
    }

    if (!enif_alloc_binary(size, &bin)) {
	enif_io_set_error(p, "allocation error");
	return ERROR;
    }

    if ((c = skip_blank(p)) == '>') {
	if ((c = enif_io_getc(p)) != '>') {
	    enif_io_set_error(p, "missing '>>'");
	    goto error;
	}
	goto build;
    }

    while(1) {
	if (c == '"') {
	    if ((size - i) < MAX_STRING_LEN) {
		size += MAX_STRING_LEN;
		if (!enif_realloc_binary(&bin, size)) {
		    enif_io_set_error(p, "re-allocation error");
		    goto error;
		}
	    }
	    if (!parse_quoted_string_buf(p,(char*)bin.data,
					 i,i+MAX_STRING_LEN,&i))
		goto error;
	}
	else {
	    ERL_NIF_TERM e;
	    if (c == '-')
		e = parse_number(p, -1, 0);
	    else if (isdigit(c)) 
		e = parse_number(p, 1, c);
	    else {
		enif_io_set_error(p, "syntax error");
		goto error;
	    }
	    if (!enif_get_int(p->env, e, &c) || (c < 0) || (c > 255)) {
		enif_io_set_error(p, "syntax error");
		goto error;
	    }
	    if (i >= size) {
		size += INC_BIN_SIZE;
		if (!enif_realloc_binary(&bin, size)) {
		    enif_io_set_error(p, "re-allocation error");
		    goto error;
		}
	    }
	    bin.data[i++] = c;
	}
	if ((c = skip_blank(p)) == '>') {
	    if ((c = enif_io_getc(p)) != '>') {
		enif_io_set_error(p, "missing '>>'");
		return ERROR;
	    }
	    break;
	}
	if (c != ',') {
	    enif_io_set_error(p, "missing '>>'");
	    goto error;
	}
	c = skip_blank(p);
    }
build:
    if (!enif_realloc_binary(&bin, i)) {
	enif_io_set_error(p, "re-allocation error");
	goto error;
    }
    return enif_make_binary(p->env, &bin);
error:
    enif_release_binary(&bin);
    return ERROR;
}

// SEEN '#' '{' pair [',' pair]* '}'
// pair = element '=' '>' element
// return ERROR | MAP
static ERL_NIF_TERM parse_map(enif_io_t* p)
{
    ERL_NIF_TERM key[MAX_MAP_LEN];
    ERL_NIF_TERM value[MAX_MAP_LEN];
    int c;
    int i = 0;

    if (skip_blank(p) != '{') {
	enif_io_set_error(p, "missing '{'");
	return ERROR;
    }
    if ((c = skip_blank(p)) == '}')
	goto build;

    enif_io_ungetc(c, p);

    while(i < MAX_MAP_LEN) {
	if (!(key[i] = parse_element(p)))
	    return ERROR;
	if ((skip_blank(p) != '=') ||
	    (skip_blank(p) != '>')) {
	    enif_io_set_error(p, "missing '=>'");
	    return ERROR;
	}
	if (!(value[i] = parse_element(p)))
	    return ERROR;
	i++;
	if ((c = skip_blank(p)) == '}')
	    goto build;
	if (c != ',') {
	    enif_io_set_error(p, "missing '}'");
	    return ERROR;
	}
    }
    enif_io_set_error(p, "map too long");
    return ERROR;
build:
    return enif_make_map_from_arrays(p->env, key, value, i);
}

// [a-z]([a-zA-Z0-9_])*
static ERL_NIF_TERM parse_atom(enif_io_t* p, int c)
{
    char buf[MAX_ATOM_LEN];
    int i = 0;

    buf[i++] = c;

    while((c = enif_io_getc(p)) >= 0) {
	if (i >= MAX_ATOM_LEN) {
	    enif_io_set_error(p, "atom name too long");
	    return ERROR;
	}
	if (isalnum(c) || (c == '_') || (c == '@'))
	    buf[i++] = c;
	else 
	    break;
    }
    if (c != EOF)
	enif_io_ungetc(c, p);
    return enif_make_atom_len(p->env, buf, i);
}

// [_A-Z]([a-zA-Z0-9_])*
static ERL_NIF_TERM parse_var(enif_io_t* p, int c)
{
    char buf[MAX_VAR_LEN];
    int i = 0;

    buf[i++] = c;
    while((c = enif_io_getc(p)) >= 0) {
	if (i >= MAX_VAR_LEN) {
	    enif_io_set_error(p, "variable name too long");
	    return ERROR;
	}
	if (isdigit(c) || isalpha(c) || (c == '_'))
	    buf[i++] = c;
	else 
	    break;
    }
    if (c != EOF)
	enif_io_ungetc(c, p);
    return ERROR;
    // ???
    // return enif_make_variable(p->env, buf, i);
}

// element:
static ERL_NIF_TERM parse_element(enif_io_t* p)
{
    int c;

    switch((c = skip_blank(p))) {
    case EOF: return 0;
    case '{': return parse_tuple(p);
    case '[': return parse_list(p);
    case '<': return parse_binary(p);
    case '#': return parse_map(p);
    case '"': return parse_quoted_string(p);
    case '\'': return parse_quoted_atom(p);
    case '-': return parse_number(p,-1,0);
    case '+': return parse_number(p,1,0);
    default:
	if (isdigit(c))
	    return parse_number(p,0,c);
	else if ((c=='_') || isupper(c))
	    return parse_var(p, c);
	else if (islower(c))
	    return parse_atom(p, c);
	return ERROR;
    }
}

// 
//  (element '.')*
//
int enif_io_scan_forms(enif_io_t* p)
{
    int r = 0;
    while(r >= 0) {
	ERL_NIF_TERM e;
	if (!(e = parse_element(p))) {
	    if (!p->state[p->sp].error) {
		if (p->sp == 0)
		    return 1;
		enif_io_pop(p);
		continue;
	    }
	    break;
	}
	if (skip_blank(p) != '.') {
	    enif_io_set_error(p, "missing '.'");
	    break;
	}
	if (p->callback)
	    r = p->callback(p, e);
    }
    return 0;
}

void enif_io_write_integer(enif_io_t* iop, ERL_NIF_TERM term)
{
    int64_t xi;
    ErlNifBignum bi;

    if (enif_get_int64(iop->env, term, &xi)) {
	switch(iop->base) {
	case 8:
	    if (xi < 0)
		enif_io_format(iop,"-8#%llo", (uint64_t) -xi);
	    else
		enif_io_format(iop,"8#%llo", (uint64_t) xi);
	    break;
	case 16: 
	    if (xi < 0) 
		enif_io_format(iop,"-16#%llx", (uint64_t) -xi);
	    else
		enif_io_format(iop,"16#%llx", (uint64_t) xi);
	    break;
	default: 
	    enif_io_format(iop,"%lld", xi); break;
	}
    }
    else if (enif_inspect_big(iop->env, term, &bi)) {
	int i;
	if (bi.sign)
	    enif_io_format(iop,"-");
	enif_io_format(iop,"16#[");
	for (i = 0; i < bi.size; i++) {
	    enif_io_format(iop,"%016lx", bi.digits[i]);
	}
	enif_io_format(iop,"]");
    }
}

void enif_io_write_float(enif_io_t* iop, ERL_NIF_TERM term)
{
    double xf;

    if (enif_get_double(iop->env, term, &xf))
	enif_io_format(iop,"%g", xf);
}


void enif_io_write_number(enif_io_t* iop, ERL_NIF_TERM term)
{
    int xi;

    if (enif_is_integer(iop->env, term))
	enif_io_write_integer(iop, term);
    else if (enif_is_float(iop->env, term))
	enif_io_write_float(iop, term);
}

static int atom_need_quotes(char* s)
{
    if (islower(*s)) {
	s++;
	while(isalnum(*s) || (*s == '_') || (*s == '@'))
	    s++;
	if (*s == '\0')
	    return 0;
    }
    return 1;
}

void enif_io_write_atom(enif_io_t* iop, ERL_NIF_TERM term)
{
    char buf[256];
    if (enif_get_atom(iop->env, term, buf, sizeof(buf), ERL_NIF_LATIN1)) {
	if (atom_need_quotes(buf))
	    enif_io_format(iop,"'%s'", buf);
	else
	    enif_io_format(iop,"%s", buf);
    }
}

void enif_io_write_binary(enif_io_t* iop, ERL_NIF_TERM term)
{
    ErlNifBinary bin;

    if (enif_inspect_binary(iop->env, term, &bin)) {
	enif_io_format(iop,"<<");
	if (bin.size > 0) {
	    int i;
	    enif_io_format(iop,"%d", bin.data[0]);
	    for (i = 1; i < bin.size; i++) {
		enif_io_format(iop,",%d", bin.data[i]);
	    }
	}
	enif_io_format(iop,">>");
    }
}

void enif_io_write_list(enif_io_t* iop, ERL_NIF_TERM term)
{
    char buf[MAX_STRING_LEN];

    if (enif_get_string(iop->env, term, buf, sizeof(buf), ERL_NIF_LATIN1)) {
	enif_io_format(iop,"\"%s\"", buf);
    }
    else {
	enif_io_format(iop,"[");
	while(enif_is_list(iop->env, term)) {
	    ERL_NIF_TERM head, tail;
	    enif_get_list_cell(iop->env, term, &head, &tail);
	    enif_io_write(iop, head);
	    term = tail;
	    if (enif_is_list(iop->env, term))
		enif_io_format(iop,",");
	}
	if (!enif_is_empty_list(iop->env, term)) {
	    enif_io_format(iop,"|");
	    enif_io_write(iop, term);
	}
	enif_io_format(iop,"]");
    }
}

void enif_io_write_tuple(enif_io_t* iop, ERL_NIF_TERM term)
{
    const ERL_NIF_TERM* array;
    int arity;

    if (enif_get_tuple(iop->env, term, &arity, &array)) {
	int i;

	enif_io_format(iop,"{");
	for (i = 0; i < arity; i++) {
	    enif_io_write(iop, array[i]);
	    if (i < arity-1)
		enif_io_format(iop,",");
	}
	enif_io_format(iop,"}");
    }
}

void enif_io_write_map(enif_io_t* iop, ERL_NIF_TERM term)
{
    ErlNifMapIteratorEntry entry;
    ErlNifMapIterator iter;

    if (enif_map_iterator_create(iop->env,term,&iter,ERL_NIF_MAP_ITERATOR_FIRST)) {
	int first = 1;
	enif_io_format(iop,"#{");
	while(enif_map_iterator_next(iop->env, &iter)) {
	    ERL_NIF_TERM key, value;
	    enif_map_iterator_get_pair(iop->env, &iter, &key, &value);
	    if (!first)
		enif_io_format(iop,",");
	    else
		first = 0;
	    enif_io_write(iop, key);
	    enif_io_format(iop," => ");
	    enif_io_write(iop, value);
	}
	enif_io_format(iop,"}");
    }
}

void enif_io_write(enif_io_t* iop, ERL_NIF_TERM term)
{
    switch(enif_get_type(term, 0)) {
    case ENIF_TYPE_INTEGER: enif_io_write_integer(iop, term); break;
    case ENIF_TYPE_FLOAT: enif_io_write_float(iop, term); break;
    case ENIF_TYPE_ATOM: enif_io_write_atom(iop, term); break;
    case ENIF_TYPE_TUPLE: enif_io_write_tuple(iop, term); break;
    case ENIF_TYPE_MAP: enif_io_write_map(iop, term); break;
    case ENIF_TYPE_NIL: enif_io_write_list(iop, term); break;
    case ENIF_TYPE_LIST: enif_io_write_list(iop, term); break;
    case ENIF_TYPE_BINARY: enif_io_write_binary(iop, term); break;
    default: enif_io_format(iop, "????");
    }
}
