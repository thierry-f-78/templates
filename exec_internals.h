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

struct exec_node *_exec_new(struct exec *e, enum exec_type type, void *value,
                            enum exec_args_type valtype, int len, int line);

static inline
struct exec_node *exec_new(struct exec *e, enum exec_type type, int line) {
	return _exec_new(e, type, NULL, XT_NULL, 0, line);
}

static inline
struct exec_node *exec_new_str(struct exec *e, enum exec_type type,
                               char *str, int len, int line) {
	return _exec_new(e, type, str, XT_STRING, len, line);
}

static inline
struct exec_node *exec_new_ent(struct exec *e, enum exec_type type,
                               int ent, int line) {
	return _exec_new(e, type, (void *)ent, XT_INTEGER, 0, line);
}

static inline
struct exec_node *exec_new_ptr(struct exec *e, enum exec_type type,
                               void *ptr, int line) {
	return _exec_new(e, type, ptr, XT_PTR, 0, line);
}

char *exec_blockdup(struct exec *e, char *str, int len);
struct exec_vars *exec_var(struct exec *e, char *str);
struct exec_funcs *exec_func(struct exec *e, char *str);
void exec_set_parents(struct exec_node *n);

#endif
