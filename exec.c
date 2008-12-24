#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "exec.h"

struct exec *exec_template;

struct exec *exec_new_template(void) {
	struct exec *e;

	e = malloc(sizeof(*e));
	if (e == NULL) {
		ERRS("malloc(%d): %s", sizeof(*e), strerror(errno));
		exit(1);
	}
	e->nbvars = 0;
	INIT_LIST_HEAD(&e->vars);
	INIT_LIST_HEAD(&e->funcs);
	return e;
}

struct exec_run *exec_new_run(struct exec *e) {
	struct exec_run *r;

	/* memory for struct */
	r = malloc(sizeof(*r));
	if (r == NULL) {
		ERRS("malloc(%d): %s", sizeof(*r), strerror(errno));
		exit(1);
	}

	/* memory for vars */
	r->vars = malloc(sizeof(r->vars) * e->nbvars);
	if (r->vars == NULL) {
		ERRS("malloc(%d): %s", sizeof(r->vars), strerror(errno));
		exit(1);
	}

	/* copy default args and init run */
	r->arg = e->arg;
	r->w = e->w;
	r->e = e;
	r->stack_ptr = 0;
	r->retry = 0;
	r->n = e->program;

	return r;
}

struct exec_node *exec_new(enum exec_type type, void *value, int line) {
	struct exec_node *n;

	n = malloc(sizeof *n);
	if (n == NULL) {
		ERRS("malloc(%d): %s\n", sizeof *n, strerror(errno));
		exit(1);
	}

	n->type = type;
	n->v.ptr = value;
	n->line = line;
	INIT_LIST_HEAD(&n->c);

	return n;
}

char *exec_blockdup(char *str) {
	char *out;
	int len;

	str += 2; /* supprime %> */
	len = strlen(str) - 2; /* supprime <% */

	out = malloc(len + 1);
	if (out == NULL) {
		ERRS("malloc(%d): %s\n", len + 1, strerror(errno));
		exit(1);
	}

	memcpy(out, str, len);
	out[len] = '\0';

	return out;
}

char *exec_strdup(char *str) {
	char *out;
	int len;

	str += 1; /* supprime " */
	len = strlen(str) - 1; /* supprime " */

	out = malloc(len + 1);
	if (out == NULL) {
		ERRS("malloc(%d): %s\n", len + 1, strerror(errno));
		exit(1);
	}

	memcpy(out, str, len);
	out[len] = '\0';

	return out;
}

void *exec_var(char *str) {
	struct exec_vars *v;

	/* search var */
	list_for_each_entry(v, &exec_template->vars, chain) {
		if (strcmp(v->name, str)==0)
			break;
	}
	if (&v->chain != &exec_template->vars)
		return v;

	/* memory for var */
	v = malloc(sizeof(*v));
	if (v == NULL) {
		ERRS("malloc(%d): %s", sizeof(*v), strerror(errno));
		exit(1);
	}

	/* set var offset */
	v->offset = exec_template->nbvars;
	exec_template->nbvars++;

	/* copy var name */
	v->name = strdup(str);
	if (v->name == NULL) {
		ERRS("strdup(\"%s\"): %s", str, strerror(errno));
		exit(1);
	}

	/* chain */
	list_add_tail(&v->chain, &exec_template->vars);

	return v;
}

struct exec_vars *exec_get_var(struct exec *e, char *str) {
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

void exec_declare_func(struct exec *e, char *name, exec_function fc) {
	struct exec_funcs *f;

	/* check if the function exists */
	list_for_each_entry(f, &e->funcs, chain) {
		if (strcmp(f->name, name)==0)
			break;
	}
	if (&f->chain != &e->funcs)
		return; /* already exists */
	
	/* memory for func */
	f = malloc(sizeof(*f));
	if (f == NULL) {
		ERRS("malloc(%d): %s", sizeof(*f), strerror(errno));
		exit(1);
	}

	/* copy function ptr */
	f->f = fc;

	/* copy var name */
	f->name = strdup(name);
	if (f->name == NULL) {
		ERRS("strdup(\"%s\"): %s", name, strerror(errno));
		exit(1);
	}

	/* chain */
	list_add_tail(&f->chain, &e->funcs);
}

void *exec_func(char *str) {
	struct exec_funcs *f;

	/* check if the function exists */
	list_for_each_entry(f, &exec_template->funcs, chain) {
		if (strcmp(f->name, str)==0)
			break;
	}
	if (&f->chain != &exec_template->funcs)
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

