#ifndef __TEMPLATES_H__
#define __TEMPLATES_H__

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "list.h"

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

extern const char *exec_cmd2str[];
extern struct exec *exec_template;

#define ERRS(fmt, args...) \
	fprintf(stderr, "[%s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##args);

/* maximiun argument can be given at function */
#define MXARGS 20

/* execute template stak */
#define STACKSIZE 1024

union exec_args {
	int integer;
	char *string;
	struct exec_vars *var;
	struct exec_funcs *func;
	void *ptr;
};

typedef void *(*exec_function)(void *easy, union exec_args *args, int nargs);
typedef ssize_t (*exec_write)(void *easy, const void *buf, size_t count);

struct exec_vars {
	struct list_head chain;
	char *name;
	int offset;
};

struct exec_funcs {	
	struct list_head chain;
	char *name;
	exec_function f;
};

struct exec_node {
	struct exec_node *p; /* parent */
	struct list_head c;  /* children */
	struct list_head b;  /* brother */

	enum exec_type type;
	union exec_args v;
	int line;
};

struct exec {
	struct exec_node *program;
	int nbvars;
	struct list_head vars;
	struct list_head funcs;
	void *arg;
	exec_write w;
};

struct exec_run {
	union exec_args *vars;
	void *arg;
	exec_write w;
	struct exec *e;
	struct exec_node *n;
	union {
		int ent;
		struct exec_node *n;
		char *string;
		void *ptr;
	} stack[STACKSIZE];
	int stack_ptr;
	int retry;
};

/**
 * New template
 */
struct exec *exec_new_template(void);

/**
 * Parse file for template
 * @param e is template id
 * @param file is filename tio parse
 */
void exec_parse(struct exec *e, char *file);

/**
 * Build dot graph for debugging program
 * @param e is template id
 * @param file is output file
 * @param display_ptr is option to watch pointer
 */
void exec_display(struct exec *e, char *file, int display_ptr);

/**
 * Define function
 * @param e is template id
 * @param name is the neme of the function in template
 * @param f is function ptr
 */
void exec_declare_func(struct exec *e, char *name, exec_function f);

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
 */
struct exec_run *exec_new_run(struct exec *e);

/**
 * get variable descriptor
 * @param e is template id
 * @param var is var name
 */
struct exec_vars *exec_get_var(struct exec *e, char *str);

/**
 * set variable value
 * @param r is run program id
 * @param v is var descriptor
 * @param val is value
 */
static inline
void exec_set_var(struct exec_run *r, struct exec_vars *v, void *val) {
	((long *)r->vars)[v->offset] = (long)val;
}

/**
 * execute template
 * @param r is run program id
 * @return 0: ended, 1: need write
 */
int exec_run_now(struct exec_run *r);

#endif