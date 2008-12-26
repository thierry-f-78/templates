#ifndef __EXEC_INTERNALS_H__
#define __EXEC_INTERNALS_H__

#include "templates.h"

#define YYSTYPE struct exec_node *
#define STACK_SIZE 150

struct yyargs_t {
	struct list_head stack[STACK_SIZE];
	int stack_idx;
	struct exec *e;
	void *scanner;
};

struct exec_node *exec_new(enum exec_type type, void *value, int line);
char *exec_blockdup(char *str);
char *exec_strdup(char *str);
void *exec_var(char *str);
void *exec_func(char *str);
void exec_set_parents(struct exec_node *n);

#endif
