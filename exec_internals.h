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
	char *print;
	int printblock;
	int printsize;
};

extern const char *exec_cmd2str[];

struct exec_node *exec_new(struct exec *e, enum exec_type type, void *value, int line);
char *exec_blockdup(struct exec *e, char *str, int len);
char *exec_strdup(struct exec *e, char *str);
struct exec_vars *exec_var(struct exec *e, char *str);
struct exec_funcs *exec_func(struct exec *e, char *str);
void exec_set_parents(struct exec_node *n);

#endif
