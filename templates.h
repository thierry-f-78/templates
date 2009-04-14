/*
 * Copyright (c) 2009 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

/** @file */

#ifndef __TEMPLATES_H__
#define __TEMPLATES_H__

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

enum exec_type {
	X_NULL = 0,
	X_COLLEC,

	X_PRINT,

	X_ADD,
	X_SUB,
	X_MUL,
	X_DIV,
	X_MOD,

	X_EQUAL,
	X_STREQ,
	X_DIFF,
	X_LT,
	X_GT,
	X_LE,
	X_GE,
	X_AND,
	X_OR,

	X_ASSIGN,

	X_DISPLAY,
	X_FUNCTION,
	X_VAR,
	X_INTEGER,
	X_STRING,

	X_SWITCH,
	X_FOR,
	X_WHILE,
	X_IF,
	X_BREAK,
	X_CONT
};

enum exec_args_type {
	XT_STRING,
	XT_INTEGER,
	XT_PTR,
	XT_NULL
};

/* maximiun argument can be given at function */
#define MXARGS 20

/* execute template stak */
#define STACKSIZE 1024

/* max size for an error string */
#define ERROR_LEN 128

/*
 * definition from: linux-2.6.24/include/linux/kernel.h
 *
 * Simple doubly linked list implementation.
 * 
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */
struct tpl_list_head {
	struct tpl_list_head *next, *prev;
};

struct exec_args {
	union {
		int ent;
		char *str;
		struct exec_vars *var;
		struct exec_funcs *func;
		struct exec_node *n;
		void *ptr;
	} v;
	int len;
	enum exec_args_type type;
	char freeit;
};

typedef int (*exec_function)(void *easy, struct exec_args *args, int nargs,
                             struct exec_args *ret);
typedef ssize_t (*exec_write)(void *easy, const void *buf, size_t count);

struct exec_vars {
	struct tpl_list_head chain;
	char *name;
	int offset;
};

struct exec_funcs {	
	struct tpl_list_head chain;
	char *name;
	exec_function f;
};

struct exec_node {
	struct exec_node *p; /* parent */
	struct tpl_list_head c;  /* children */
	struct tpl_list_head b;  /* brother */

	enum exec_type type;
	struct exec_args v;
	int line;
};

struct exec {
	struct exec_node *program;
	int nbvars;
	struct tpl_list_head vars;
	struct tpl_list_head funcs;
	void *arg;
	exec_write w;
	char error[ERROR_LEN];
};

struct exec_run {
	struct exec_args *vars;
	void *arg;
	exec_write w;
	struct exec *e;
	struct exec_node *n;
	struct exec_args stack[STACKSIZE];
	int stack_ptr;
	int retry;
	char error[ERROR_LEN];
};

/**
 * New template
 */
struct exec *exec_new_template(void);

/**
 * clear template - free memory
 */
void exec_clear_template(struct exec *e);

/**
 * Parse file for template
 * @param e is template id
 * @param fd is FILE * pointer on open file in mode "r"
 * @return struct exec if ok, NULL, if memory error
 */
int exec_parse_file(struct exec *e, FILE *fd);

/**
 * Parse file for template
 * @param e is template id
 * @param file is filename tio parse
 * @return struct exec if ok, NULL, if memory error
 */
int exec_parse(struct exec *e, char *file);

/**
 * Build dot graph for debugging program.
 * You can build dot graph with this commanda line:
 * 'cat <file> | dot -Gsize="11.0,7.6" -Gpage="8.3,11.7" -Tps | ps2pdf - graph.pdf'
 * @param e is template id
 * @param file is output file
 * @param display_ptr is option to watch pointer
 * @param cut is the len of string displayed
 * @return 0 if ok, else -1
 */
int exec_display(struct exec *e, char *file, int display_ptr, int cut);

/**
 * Define function
 * @param e is template id
 * @param name is the neme of the function in template
 * @param f is function ptr
 * @return 0 if OK, -1 if an error is occurred, -2 if
 *         the function is already declared
 */
int exec_declare_func(struct exec *e, char *name, exec_function f);

/**
 * set default easy argument
 * @param e is template id
 * @param easy is easy arg
 */
static inline
void exec_set_easy(struct exec *e, void *easy) {
	e->arg = easy;
}

/**
 * set default write function
 * @param e is template id
 * @param w write function
 */
static inline
void exec_set_write(struct exec *e, exec_write w) {
	e->w = w;
}

/**
 * set easy argument
 * @param e is template id
 * @param easy is easy arg
 */
static inline
void exec_run_set_easy(struct exec_run *r, void *easy) {
	r->arg = easy;
}

/**
 * set write function
 * @param e is template id
 * @param w write function
 */
static inline
void exec_run_set_write(struct exec_run *r, exec_write w) {
	r->w = w;
}

/**
 * Initiate an execution session
 * @param e is template id
 * @return struct exec_run if OK, NULL if memory error
 */
struct exec_run *exec_new_run(struct exec *e);

/**
 * Clear an execution session
 * @param e is template id
 */
void exec_clear_run(struct exec_run *r);

/**
 * get variable descriptor
 * @param e is template id
 * @param var is var name
 * @return struct exec_vars if OK, NULL if var not found
 */
struct exec_vars *exec_get_var(struct exec *e, const char *str);

/* for the static inline variable effectation funcs */
#define reqvar (r->vars[v->offset])

/**
 * set variable integer value
 * @param r is run program id
 * @param v is var descriptor
 * @param val is value
 */
static inline
void exec_set_var_int(struct exec_run *r, struct exec_vars *v, int val) {
	if (v == NULL)
		return;
	if (reqvar.freeit == 1)
		free(reqvar.v.str);
	reqvar.v.ent = val;
	reqvar.type = XT_INTEGER;
	reqvar.freeit = 0;
}

/**
 * set variable string value
 * @param r is run program id
 * @param v is var descriptor
 * @param val is string
 * @param free is flag. when this flags is set, the ptr is freed
 */
static inline
void exec_set_var_str(struct exec_run *r, struct exec_vars *v,
                      char *val, int freeit) {
	if (v == NULL)
		return;
	if (reqvar.freeit == 1)
		free(reqvar.v.str);
	reqvar.v.str = val;
	reqvar.len = strlen(val);
	reqvar.type = XT_STRING;
	reqvar.freeit = freeit;
}

/**
 * set variable block value
 * @param r is run program id
 * @param v is var descriptor
 * @param val is block
 * @param len is len
 * @param free is flag. when this flags is set, the ptr is freed
 */
static inline
void exec_set_var_block(struct exec_run *r, struct exec_vars *v,
                      char *val, int len, int freeit) {
	if (v == NULL)
		return;
	if (reqvar.freeit == 1)
		free(reqvar.v.str);
	reqvar.v.str = val;
	reqvar.len = len;
	reqvar.type = XT_STRING;
	reqvar.freeit = freeit;
}

/**
 * execute template
 * @param r is run program id
 * @return 0: ended, 1: need write, -1: error
 *     the error message contained is in r->error;
 */
int exec_run_now(struct exec_run *r);

#endif
