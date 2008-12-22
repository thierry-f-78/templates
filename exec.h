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

struct exec_vars {
	struct list_head chain;
	char *name;
	int offset;
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
};

static inline void exec_new_template(void) {
	exec_template = malloc(sizeof(*exec_template));
	if (exec_template == NULL) {
		ERRS("malloc(%d): %s", sizeof(*exec_template), strerror(errno));
		exit(1);
	}
	exec_template->nbvars = 0;
	INIT_LIST_HEAD(&exec_template->vars);
}
static inline void exec_set_template(struct exec_node *n) {
	exec_template->program = n;
}
static inline struct exec *exec_get_template(void) {
	return exec_template;
}
struct exec_node *exec_new(enum exec_type type, void *value);
char *exec_blockdup(char *str);
char *exec_strdup(char *str);
void *exec_var(char *str);
void *exec_func(char *str);
void exec_display(struct exec_node *n);

#endif
