#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "templates.h"

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
	[X_STREQ]    = "X_STREQ (===)",
	[X_DIFF]     = "X_DIFF (!=)",
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
	[X_SWITCH]   = "X_SWITCH",
	[X_FOR]      = "X_FOR",
	[X_WHILE]    = "X_WHILE",
	[X_IF]       = "X_IF",
	[X_BREAK]    = "X_BREAK",
	[X_CONT]     = "X_CONT"
};

char *replace_n(struct exec *e, char *str) {
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

	dest = malloc(len + 1);
	if (dest == NULL) {
		snprintf(e->error, ERROR_LEN, "malloc(%d): %s\n", len, strerror(errno));
		return NULL;
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

		case '{':
			*dest = '\\'; dest++;
			*dest = '{';  dest++;
			break;

		case '}':
			*dest = '\\'; dest++;
			*dest = '}';  dest++;
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

	*dest = '\0';

	return ret;
}

int exec_display_recurse(struct exec *e, struct exec_node *n, int first, FILE *fd, int display_ptr) {
	struct exec_node *p;
	char *c;
	int ret;

	if (first == 1)
		fprintf(fd, "\tnode [ fillcolor=\"yellow\" ]\n");

	switch (n->type) {

	case X_NULL:
	case X_COLLEC:
	case X_ADD:
	case X_SUB:
	case X_MUL:
	case X_DIV:
	case X_MOD:
	case X_EQUAL:
	case X_DIFF:
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
	case X_CONT:
	case X_BREAK:
	case X_SWITCH:
		if (display_ptr == 1)
			fprintf(fd, "\t\"%p\" [ label=\"{{{%p|p=%p}|{c=%p|next=%p|prev=%p}|"
			        "{b=%p|next=%p|prev=%p}}|%s}\" ]\n",
			        n, n, n->p,
			        &n->c, n->c.next, n->c.prev,
			        &n->b, n->b.next, n->b.prev,
			        exec_cmd2str[n->type]);
		else
			fprintf(fd, "\t\"%p\" [ label=\"%s\" ]\n",
			        n, exec_cmd2str[n->type]);
		break;

	case X_FUNCTION:
		if (display_ptr == 1)
			fprintf(fd, "\t\"%p\" [ label=\"{{{%p|p=%p}|{c=%p|next=%p|prev=%p}|"
			        "{b=%p|next=%p|prev=%p}}|%s|%s(%p)}\" ]\n",
			        n, n, n->p,
			        &n->c, n->c.next, n->c.prev,
			        &n->b, n->b.next, n->b.prev,
			        exec_cmd2str[n->type],
			        ((struct exec_funcs *)n->v.v.ptr)->name,
			        ((struct exec_funcs *)n->v.v.ptr)->f);
		else
			fprintf(fd, "\t\"%p\" [ label=\"{%s|%s(%p)}\" ]\n",
			        n, exec_cmd2str[n->type], ((struct exec_funcs *)n->v.v.ptr)->name,
			        ((struct exec_funcs *)n->v.v.ptr)->f);
		break;

	case X_VAR:
		if (display_ptr == 1)
			fprintf(fd, "\t\"%p\" [ label=\"{{{%p|p=%p}|{c=%p|next=%p|prev=%p}|"
			        "{b=%p|next=%p|prev=%p}}|%s|%s (%d)}\" ]\n",
			        n, n, n->p,
			        &n->c, n->c.next, n->c.prev,
			        &n->b, n->b.next, n->b.prev,
			        exec_cmd2str[n->type], n->v.v.var->name, n->v.v.var->offset);
		else
			fprintf(fd, "\t\"%p\" [ label=\"{%s|%s (%d)}\" ]\n",
			        n, exec_cmd2str[n->type], n->v.v.var->name, n->v.v.var->offset);
		break;

	case X_INTEGER:
		if (display_ptr == 1)
			fprintf(fd, "\t\"%p\" [ label=\"{{{%p|p=%p}|{c=%p|next=%p|prev=%p}|"
			        "{b=%p|next=%p|prev=%p}}|%s|%d}\" ]\n",
			        n, n, n->p,
			        &n->c, n->c.next, n->c.prev,
			        &n->b, n->b.next, n->b.prev,
			        exec_cmd2str[n->type], n->v.v.ent);
		else
			fprintf(fd, "\t\"%p\" [ label=\"{%s|%d}\" ]\n",
			       n, exec_cmd2str[n->type], n->v.v.ent);
		break;

	case X_PRINT:
	case X_STRING:
		c = replace_n(e, n->v.v.str);
		if (c == NULL)
			return -1;
		if (display_ptr == 1)
			fprintf(fd, "\t\"%p\" [ label=\"{{{%p|p=%p}|{c=%p|next=%p|prev=%p}|"
			       "{b=%p|next=%p|prev=%p}}|%s|\\\"%s\\\"}\" ]\n",
			       n, n, n->p,
			       &n->c, n->c.next, n->c.prev,
			       &n->b, n->b.next, n->b.prev,
			       exec_cmd2str[n->type], c);
		else
			fprintf(fd, "\t\"%p\" [ label=\"{%s|\\\"%s\\\"}\" ]\n",
			        n, exec_cmd2str[n->type], c);
		break;
	
	default:
		if (display_ptr == 1)
			fprintf(fd, "\t\"%p\" [ label=\"{{{%p|p=%p}|{c=%p|next=%p|prev=%p}|"
			        "{b=%p|next=%p|prev=%p}}|type=%d|%p}\" ]\n",
			        n, n, n->p,
			        &n->c, n->c.next, n->c.prev,
			        &n->b, n->b.next, n->b.prev,
			        n->type, n->v.v.ptr);
		else
			fprintf(fd, "\t\"%p\" [ label=\"{Error: Unknown node (code %d)|%p}\" ]\n",
			        n, n->type, n->v.v.ptr);
		break;

	}

	if (first == 1)
		fprintf(fd, "\tnode [ fillcolor=\"white\" ]\n");

	list_for_each_entry(p, &n->c, b) {
		fprintf(fd, "\t\"%p\" -> \"%p\"\n", n, p);
		ret = exec_display_recurse(e, p, 0, fd, display_ptr);
		if (ret != 0)
			return -1;
	}

	return 0;
}

int exec_display(struct exec *e, char *file, int display_ptr) {
	struct exec_vars *v;
	FILE *fd;
	int ret;

	fd = fopen(file, "w");
	if (fd == NULL) {
		snprintf(e->error, ERROR_LEN, "fopen(\"%s\", \"w\"): %s", file, strerror(errno));
		return -1;
	}

	fprintf(fd,
		"digraph finite_state_machine {\n"
		"\torientation=landscape\n"
		"\tnode [ fontname=\"Helvetica\" ]\n"
		"\tnode [ shape=\"record\" ]\n"
		"\tnode [ style=\"filled\" ]\n"
		"\tnode [ color=\"black\" ]\n"
		"\tnode [ fontcolor=\"black\" ]\n"
		"\tnode [ fillcolor=\"white\" ]\n"
	);

	/* display var */
	fprintf(fd, "\t\"vars\" [ label=\"{Varlist");
	list_for_each_entry(v, &e->vars, chain)
		fprintf(fd, "|{%s|%d}", v->name, v->offset);
	fprintf(fd, "}\" fillcolor=\"lightblue\" ]\n");

	ret = exec_display_recurse(e, e->program, 1, fd, display_ptr);

	fprintf(fd, "\t\"vars\" -> \"%p\"\n", e->program);
	fprintf(fd, "}\n");
	fclose(fd);

	if (ret != 0)
		return -1;
	return 0;
}

