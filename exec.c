#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "exec.h"

const char *exec_cmd2str[] = {
	[X_NULL]     = "X_NULL",
	[X_COLLEC]   = "X_COLLEC",
	[X_PRINT]    = "X_PRINT",
	[X_ADD]      = "X_ADD (+)",
	[X_SUB]      = "X_SUB (-)",
	[X_MUL]      = "X_MUL (*)",
	[X_DIV]      = "X_DIV (/)",
	[X_MOD]      = "X_MOD (%)",
	[X_EQUAL]    = "X_EQUAL (==)",
	[X_LT]       = "X_LT (\\<)",
	[X_GT]       = "X_GT (\\>)",
	[X_LE]       = "X_LE (\\<=)",
	[X_GE]       = "X_GE (\\>=)",
	[X_AND]      = "X_AND (&&)",
	[X_OR]       = "X_OR (||)",
	[X_ASSIGN]   = "X_ASSIGN (=)",
	[X_DISPLAY]  = "X_DISPLAY",
	[X_FUNCTION] = "X_FUNCTION",
	[X_VAR]      = "X_VAR",
	[X_INTEGER]  = "X_INTEGER",
	[X_STRING]   = "X_STRING",
	[X_FOR]      = "X_FOR",
	[X_WHILE]    = "X_WHILE",
	[X_IF]       = "X_IF"
};

struct exec *exec_template;

struct exec_node *exec_new(enum exec_type type, void *value) {
	struct exec_node *n;

	n = malloc(sizeof *n);
	if (n == NULL) {
		ERRS("malloc(%d): %s\n", sizeof *n, strerror(errno));
		exit(1);
	}

	n->type = type;
	n->v.ptr = value;
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

void *exec_func(char *str) {
	return exec_new(X_FUNCTION, NULL);
}

char *replace_n(char *str) {
	char *p;
	char *dest;
	char *ret;
	int len = 0;

	p = str;
	while(*p != 0) {
		switch (*p) {

		case '\n':
		case '\r':
		case '\t':
			len += 3;
			break;

		case '>':
		case '<':
		case '"':
			len += 2;
			break;

		default:
			len++;
			break;

		}
		p++;
	}

	dest = malloc(len);
	if (dest == NULL) {
		ERRS("malloc(%d): %s\n", len, strerror(errno));
		exit(1);
	}
	ret = dest;

	p = str;
	while(*p != 0) {
		switch (*p) {

		case '\n':
			*dest = '\\'; dest++;
			*dest = '\\'; dest++;
			*dest = 'n';  dest++;
			break;

		case '\r':
			*dest = '\\'; dest++;
			*dest = '\\'; dest++;
			*dest = 'r';  dest++;
			break;

		case '<':
			*dest = '\\'; dest++;
			*dest = '<';  dest++;
			break;

		case '>':
			*dest = '\\'; dest++;
			*dest = '>';  dest++;
			break;

		case '\t':
			*dest = '\\'; dest++;
			*dest = '\\'; dest++;
			*dest = 't';  dest++;
			break;

		case '"':
			*dest = '\\'; dest++;
			*dest = '"';  dest++;
			break;

		default:
			*dest = *p;  dest++;
			break;

		}
		p++;
	}

	return ret;
}

void exec_display_recurse(struct exec_node *n, int first) {
	struct exec_node *p;
	char *c;
	int display_ptr = 0;

	if (first == 1)
		printf("\tnode [ fillcolor=\"yellow\" ]\n");

	switch (n->type) {

	case X_NULL:
	case X_COLLEC:
	case X_ADD:
	case X_SUB:
	case X_MUL:
	case X_DIV:
	case X_MOD:
	case X_EQUAL:
	case X_LT:
	case X_GT:
	case X_LE:
	case X_GE:
	case X_AND:
	case X_OR:
	case X_ASSIGN:
	case X_FOR:
	case X_WHILE:
	case X_DISPLAY:
	case X_IF:
		if (display_ptr == 1)
			printf("\t\"%p\" [ label=\"{{%p|{c=%p|next=%p|prev=%p}|"
			       "{b=%p|next=%p|prev=%p}}|%s}\" ]\n",
			       n, n,
			       &n->c, n->c.next, n->c.prev,
			       &n->b, n->b.next, n->b.prev,
			       exec_cmd2str[n->type]);
		else
			printf("\t\"%p\" [ label=\"%s\" ]\n",
			       n, exec_cmd2str[n->type]);
		break;

	case X_FUNCTION:
		if (display_ptr == 1)
			printf("\t\"%p\" [ label=\"{{%p|{c=%p|next=%p|prev=%p}|"
			       "{b=%p|next=%p|prev=%p}}|%s|%p}\" ]\n",
			       n, n,
			       &n->c, n->c.next, n->c.prev,
			       &n->b, n->b.next, n->b.prev,
			       exec_cmd2str[n->type], n->v.ptr);
		else
			printf("\t\"%p\" [ label=\"{%s|%p}\" ]\n",
			       n, exec_cmd2str[n->type], n->v.ptr);
		break;

	case X_VAR:
		if (display_ptr == 1)
			printf("\t\"%p\" [ label=\"{{%p|{c=%p|next=%p|prev=%p}|"
			       "{b=%p|next=%p|prev=%p}}|%s|%s (%d)}\" ]\n",
			       n, n,
			       &n->c, n->c.next, n->c.prev,
			       &n->b, n->b.next, n->b.prev,
			       exec_cmd2str[n->type], n->v.var->name, n->v.var->offset);
		else
			printf("\t\"%p\" [ label=\"{%s|%s (%d)}\" ]\n",
			       n, exec_cmd2str[n->type], n->v.var->name, n->v.var->offset);
		break;

	case X_INTEGER:
		if (display_ptr == 1)
			printf("\t\"%p\" [ label=\"{{%p|{c=%p|next=%p|prev=%p}|"
			       "{b=%p|next=%p|prev=%p}}|%s|%d}\" ]\n",
			       n, n,
			       &n->c, n->c.next, n->c.prev,
			       &n->b, n->b.next, n->b.prev,
			       exec_cmd2str[n->type], n->v.integer);
		else
			printf("\t\"%p\" [ label=\"{%s|%d}\" ]\n",
			       n, exec_cmd2str[n->type], n->v.integer);
		break;

	case X_PRINT:
	case X_STRING:
		c = replace_n(n->v.string);
		if (display_ptr == 1)
			printf("\t\"%p\" [ label=\"{{%p|{c=%p|next=%p|prev=%p}|"
			       "{b=%p|next=%p|prev=%p}}|%s|\\\"%s\\\"}\" ]\n",
			       n, n,
			       &n->c, n->c.next, n->c.prev,
			       &n->b, n->b.next, n->b.prev,
			       exec_cmd2str[n->type], c);
		else
			printf("\t\"%p\" [ label=\"{%s|\\\"%s\\\"}\" ]\n",
			       n, exec_cmd2str[n->type], c);
		break;
	
	default:
		if (display_ptr == 1)
			printf("\t\"%p\" [ label=\"{{%p|{c=%p|next=%p|prev=%p}|"
			       "{b=%p|next=%p|prev=%p}}|type=%d|%p}\" ]\n",
			       n, n,
			       &n->c, n->c.next, n->c.prev,
			       &n->b, n->b.next, n->b.prev,
			       n->type, n->v.ptr);
		else
			printf("\t\"%p\" [ label=\"{Error: Unknown node (code %d)|%p}\" ]\n",
			       n, n->type, n->v.ptr);
		break;

	}

	if (first == 1)
		printf("\tnode [ fillcolor=\"white\" ]\n");

	list_for_each_entry(p, &n->c, b) {
		printf("\t\"%p\" -> \"%p\"\n", n, p);
		exec_display_recurse(p, 0);
	}
}

void exec_display(struct exec_node *n) {
	printf(
		"digraph finite_state_machine {\n"
		"\tnode [ fontname=\"Helvetica\" ]\n"
		"\tnode [ shape=\"record\" ]\n"
		"\tnode [ style=\"filled\" ]\n"
		"\tnode [ color=\"black\" ]\n"
		"\tnode [ fontcolor=\"black\" ]\n"
		"\tnode [ fillcolor=\"white\" ]\n"

	);

	exec_display_recurse(n, 1);

	printf("}\n");
}
