#ifndef __EXEC_H__
#define __EXEC_H__

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

typedef void *(*exec_function)(void *easy, void *args[], int nargs);

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
	union {
		int integer;
		char *string;
		struct exec_vars *var;
		void *ptr;
	} v;
};

struct exec {
	struct exec_node *program;
	int nbvars;
	struct list_head vars;
	struct list_head funcs;
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
 */
void exec_display(struct exec *e);

/**
 * Define function
 * @param e is template id
 * @param name is the neme of the function in template
 * @param f is function ptr
 */
void exec_declare_func(struct exec *e, char *name, exec_function f);

/* private */
struct exec_node *exec_new(enum exec_type type, void *value);
char *exec_blockdup(char *str);
char *exec_strdup(char *str);
void *exec_var(char *str);
void *exec_func(char *str);

#endif
