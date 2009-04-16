/*
 * Copyright (c) 2009 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "templates.h"
#include "exec_internals.h"

struct exec *exec_new_template(void) {
	struct exec *e;

	e = malloc(sizeof(*e));
	if (e == NULL) {
		snprintf(e->error, ERROR_LEN, "malloc(%d): %s", sizeof(*e), strerror(errno));
		return NULL;
	}
	e->nbvars = 0;
	INIT_LIST_HEAD(&e->vars);
	INIT_LIST_HEAD(&e->funcs);
	return e;
}

static void _exec_clear_node(struct exec_node *node) {
	struct exec_node *n;
	struct exec_node *n_cur;

	/* list nodes */
	list_for_each_entry_safe(n, n_cur, &node->c, b) {
		_exec_clear_node(n);
		list_del(&n->b);
		free(n);
	}
}

void exec_clear_template(struct exec *e) {
	struct exec_vars *v;
	struct exec_vars *v_cur;
	struct exec_funcs *f;
	struct exec_funcs *f_cur;

	/* clear execution tree */
	if (e->program != NULL)
		_exec_clear_node(e->program);

	/* clear vars */
	list_for_each_entry_safe(v, v_cur, &e->vars, chain) {
		list_del(&v->chain);
		free(v->name);
	}

	/* clear functions */
	list_for_each_entry_safe(f, f_cur, &e->funcs, chain) {
		list_del(&f->chain);
		free(f->name);
	}

	/* clear */
	free(e);
}

struct exec_run *exec_new_run(struct exec *e) {
	struct exec_run *r;

	/* memory for struct */
	r = malloc(sizeof(*r));
	if (r == NULL) {
		return NULL;
	}

	/* memory for vars */
	r->vars = malloc(sizeof(*r->vars) * e->nbvars);
	if (r->vars == NULL) {
		free(r);
		return NULL;
	}

	/* set variable stack */
	memset(r->vars, '\0', sizeof(*r->vars) * e->nbvars);

	/* copy default args and init run */
	r->arg = e->arg;
	r->w = e->w;
	r->e = e;
	r->stack_ptr = 0;
	r->retry = 0;
	r->n = e->program;

	return r;
}

void exec_clear_run(struct exec_run *r) {
	int i;

	for (i=0; i<r->e->nbvars; i++)
		if (r->vars[i].freeit == 1)
			free(r->vars[i].v.ptr);

	free(r->vars);
	free(r);
}

struct exec_node *_exec_new(struct exec *e, enum exec_type type, void *value,
                            enum exec_args_type valtype, int len, int line) {
	struct exec_node *n;

#ifdef DEBUG
	fprintf(stderr, "[%s:%s:%d] type=%s(%d) value=%p, valtype=%s(%d), len=%d, line=%d\n",
	        __FILE__, __FUNCTION__, __LINE__,
	        exec_cmd2str[type], type, value, exec_type2str[valtype], valtype, len, line);
#endif

	n = malloc(sizeof *n);
	if (n == NULL) {
		snprintf(e->error, ERROR_LEN, "malloc(%d): %s\n", sizeof *n, strerror(errno));
		return NULL;
	}

	n->type = type;
	n->v.v.ptr = value;
	n->v.len = len;
	n->v.type = valtype;
	n->line = line;
	INIT_LIST_HEAD(&n->c);

	return n;
}

char *exec_blockdup(struct exec *e, char *str, int len) {
	char *out;

	out = malloc(len+1);
	if (out == NULL) {
		snprintf(e->error, ERROR_LEN, "malloc(%d): %s\n", len + 1, strerror(errno));
		return NULL;
	}

	memcpy(out, str, len);
	out[len] = '\0';

	return out;
}

struct exec_vars *exec_var(struct exec *e, char *str) {
	struct exec_vars *v;

	/* search var */
	list_for_each_entry(v, &e->vars, chain) {
		if (strcmp(v->name, str)==0)
			break;
	}
	if (&v->chain != &e->vars)
		return v;

	/* memory for var */
	v = malloc(sizeof(*v));
	if (v == NULL) {
		snprintf(e->error, ERROR_LEN, "malloc(%d): %s", sizeof(*v), strerror(errno));
		return NULL;
	}

	/* set var offset */
	v->offset = e->nbvars;
	e->nbvars++;

	/* copy var name */
	v->name = strdup(str);
	if (v->name == NULL) {
		snprintf(e->error, ERROR_LEN, "strdup(\"%s\"): %s", str, strerror(errno));
		return NULL;
	}

	/* chain */
	list_add_tail(&v->chain, &e->vars);

	return v;
}

struct exec_vars *exec_get_var(struct exec *e, const char *str) {
	struct exec_vars *v;

	/* search var */
	list_for_each_entry(v, &e->vars, chain) {
		if (strcmp(v->name, str)==0)
			break;
	}
	if (&v->chain != &e->vars)
		return v;

	return NULL;
}

int exec_declare_func(struct exec *e, char *name, exec_function fc) {
	struct exec_funcs *f;

	/* check if the function exists */
	list_for_each_entry(f, &e->funcs, chain) {
		if (strcmp(f->name, name)==0)
			break;
	}
	if (&f->chain != &e->funcs)
		return -2; /* already exists */
	
	/* memory for func */
	f = malloc(sizeof(*f));
	if (f == NULL) {
		snprintf(e->error, ERROR_LEN, "malloc(%d): %s", sizeof(*f), strerror(errno));
		return -1;
	}

	/* copy function ptr */
	f->f = fc;

	/* copy var name */
	f->name = strdup(name);
	if (f->name == NULL) {
		snprintf(e->error, ERROR_LEN, "strdup(\"%s\"): %s", name, strerror(errno));
		return -1;
	}

	/* chain */
	list_add_tail(&f->chain, &e->funcs);
	return 0;
}

struct exec_funcs *exec_func(struct exec *e, char *str) {
	struct exec_funcs *f;

	/* check if the function exists */
	list_for_each_entry(f, &e->funcs, chain) {
		if (strcmp(f->name, str)==0)
			break;
	}
	if (&f->chain != &e->funcs)
		return f;

	return NULL;
}

void exec_set_parents(struct exec_node *n) {
	struct exec_node *p;

	list_for_each_entry(p, &n->c, b) {
		p->p = n;
		exec_set_parents(p);
	}
}

