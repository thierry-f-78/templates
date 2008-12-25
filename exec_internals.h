#ifndef __EXEC_INTERNALS_H__
#define __EXEC_INTERNALS_H__

#include "exec.h"

#define YYSTYPE struct exec_node *

struct exec_node *exec_new(enum exec_type type, void *value, int line);
char *exec_blockdup(char *str);
char *exec_strdup(char *str);
void *exec_var(char *str);
void *exec_func(char *str);
void exec_set_parents(struct exec_node *n);

#endif
