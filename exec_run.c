#include "exec.h"

//#define TRACEDEBUG

#ifdef TRACEDEBUG
#	define DEBUG(fmt, args...) \
	        fprintf(stderr, "\e[33m" fmt "\e[0m\n", ##args);
#else
#	define DEBUG(fmt, args...)
#endif

#define exec_NODE(r, n) \
	node2func[n->type](r, n)

static void *exec_X_NULL(struct exec_run *r, struct exec_node *n);
static void *exec_X_COLLEC(struct exec_run *r, struct exec_node *n);
static void *exec_X_PRINT(struct exec_run *r, struct exec_node *n);
static void *exec_X_ADD(struct exec_run *r, struct exec_node *n);
static void *exec_X_SUB(struct exec_run *r, struct exec_node *n);
static void *exec_X_MUL(struct exec_run *r, struct exec_node *n);
static void *exec_X_DIV(struct exec_run *r, struct exec_node *n);
static void *exec_X_MOD(struct exec_run *r, struct exec_node *n);
static void *exec_X_EQUAL(struct exec_run *r, struct exec_node *n);
static void *exec_X_DIFF(struct exec_run *r, struct exec_node *n);
static void *exec_X_LT(struct exec_run *r, struct exec_node *n);
static void *exec_X_GT(struct exec_run *r, struct exec_node *n);
static void *exec_X_LE(struct exec_run *r, struct exec_node *n);
static void *exec_X_GE(struct exec_run *r, struct exec_node *n);
static void *exec_X_AND(struct exec_run *r, struct exec_node *n);
static void *exec_X_OR(struct exec_run *r, struct exec_node *n);
static void *exec_X_ASSIGN(struct exec_run *r, struct exec_node *n);
static void *exec_X_DISPLAY(struct exec_run *r, struct exec_node *n);
static void *exec_X_FUNCTION(struct exec_run *r, struct exec_node *n);
static void *exec_X_VAR(struct exec_run *r, struct exec_node *n);
static void *exec_X_INTEGER(struct exec_run *r, struct exec_node *n);
static void *exec_X_STRING(struct exec_run *r, struct exec_node *n);
static void *exec_X_SWITCH(struct exec_run *r, struct exec_node *n);
static void *exec_X_FOR(struct exec_run *r, struct exec_node *n);
static void *exec_X_WHILE(struct exec_run *r, struct exec_node *n);
static void *exec_X_IF(struct exec_run *r, struct exec_node *n);
static void *exec_X_BREAK(struct exec_run *r, struct exec_node *n);
static void *exec_X_CONT(struct exec_run *r, struct exec_node *n);

typedef void *(*exec_exec_func)(struct exec_run *r, struct exec_node *n);

static const exec_exec_func node2func[] = {
	[X_NULL] = exec_X_NULL,
	[X_COLLEC] = exec_X_COLLEC,
	[X_PRINT] = exec_X_PRINT,
	[X_ADD] = exec_X_ADD,
	[X_SUB] = exec_X_SUB,
	[X_MUL] = exec_X_MUL,
	[X_DIV] = exec_X_DIV,
	[X_MOD] = exec_X_MOD,
	[X_EQUAL] = exec_X_EQUAL,
	[X_DIFF] = exec_X_DIFF,
	[X_LT] = exec_X_LT,
	[X_GT] = exec_X_GT,
	[X_LE] = exec_X_LE,
	[X_GE] = exec_X_GE,
	[X_AND] = exec_X_AND,
	[X_OR] = exec_X_OR,
	[X_ASSIGN] = exec_X_ASSIGN,
	[X_DISPLAY] = exec_X_DISPLAY,
	[X_FUNCTION] = exec_X_FUNCTION,
	[X_VAR] = exec_X_VAR,
	[X_INTEGER] = exec_X_INTEGER,
	[X_STRING] = exec_X_STRING,
	[X_SWITCH] = exec_X_SWITCH,
	[X_FOR] = exec_X_FOR,
	[X_WHILE] = exec_X_WHILE,
	[X_IF] = exec_X_IF,
	[X_BREAK] = exec_X_BREAK,
	[X_CONT] = exec_X_CONT
};

static void *exec_X_NULL(struct exec_run *r, struct exec_node *n) {
	return NULL;
}

/**********************************************************************
* 
* X_DISPLAY
*
**********************************************************************/
static void *exec_X_DISPLAY(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	char *s;

	a = container_of(n->c.next, struct exec_node, b);
	s = exec_NODE(r, a);

	DEBUG("%4d: display(%s);", n->line, s);
	r->w(r->arg, s, strlen(s));
	return NULL;
}

/**********************************************************************
* 
* X_PRINT
*
**********************************************************************/
static void *exec_X_PRINT(struct exec_run *r, struct exec_node *n) {
	DEBUG("%4d: print(%s);", n->line, n->v.string);
	r->w(r->arg, n->v.string, strlen(n->v.string));
	return NULL;
}

/**********************************************************************
* 
* X_COLLEC
*
**********************************************************************/
static void *exec_X_COLLEC(struct exec_run *r, struct exec_node *n) {
	struct exec_node *p;
	void *ret;

	list_for_each_entry(p, &n->c, b) {
		if (p->type == X_BREAK)
			return (void *)-1;
		if (p->type == X_CONT)
			return (void *)-2;
		ret = exec_NODE(r, p);
		if (p->type == X_IF && ret != NULL)
			return ret;
	}
	return NULL;
}

/**********************************************************************
* 
* X_ADD
*
**********************************************************************/
static void *exec_X_ADD(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c + (long)d);
}

/**********************************************************************
* 
* X_SUB
*
**********************************************************************/
static void *exec_X_SUB(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c - (long)d);
}

/**********************************************************************
* 
* X_MUL
*
**********************************************************************/
static void *exec_X_MUL(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c * (long)d);
}

/**********************************************************************
* 
* X_DIV
*
**********************************************************************/
static void *exec_X_DIV(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c / (long)d);
}

/**********************************************************************
* 
* X_MOD
*
**********************************************************************/
static void *exec_X_MOD(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c % (long)d);
}

/**********************************************************************
* 
* X_EQUAL
*
**********************************************************************/
static void *exec_X_EQUAL(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c == (long)d);
}

/**********************************************************************
* 
* X_DIFF
*
**********************************************************************/
static void *exec_X_DIFF(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c != (long)d);
}

/**********************************************************************
* 
* X_LT
*
**********************************************************************/
static void *exec_X_LT(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c < (long)d);
}

/**********************************************************************
* 
* X_GT
*
**********************************************************************/
static void *exec_X_GT(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c > (long)d);
}

/**********************************************************************
* 
* X_LE
*
**********************************************************************/
static void *exec_X_LE(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c <= (long)d);
}

/**********************************************************************
* 
* X_GE
*
**********************************************************************/
static void *exec_X_GE(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c >= (long)d);
}

/**********************************************************************
* 
* X_AND
*
**********************************************************************/
static void *exec_X_AND(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c && (long)d);
}

/**********************************************************************
* 
* X_OR
*
**********************************************************************/
static void *exec_X_OR(struct exec_run *r, struct exec_node *n) {
	struct exec_node *a;
	struct exec_node *b;
	void *c;
	void *d;

	a = container_of(n->c.next, struct exec_node, b);
	b = container_of(a->b.next, struct exec_node, b);

	c = exec_NODE(r, a);
	d = exec_NODE(r, b);

	return (void *)((long)c || (long)d);
}

/**********************************************************************
* 
* X_ASSIGN
*
**********************************************************************/
static void *exec_X_ASSIGN(struct exec_run *r, struct exec_node *n) {
	struct exec_node *lv;
	struct exec_node *rv;

	lv = container_of(n->c.next, struct exec_node, b);
	rv = container_of(lv->b.next, struct exec_node, b);

	r->vars[lv->v.var->offset].ptr = exec_NODE(r, rv);
	DEBUG("%4d: %s = %p;", n->line, lv->v.var->name, r->vars[lv->v.var->offset].ptr);

	return r->vars[lv->v.var->offset].ptr;
}

/**********************************************************************
* 
* X_FUNCTION
*
**********************************************************************/
static void *exec_X_FUNCTION(struct exec_run *r, struct exec_node *n) {
	union exec_args args[EXEC_MAX_ARGS];
	struct exec_node *p;
	int nargs = 0;
	void *ret;

#ifdef TRACEDEBUG
	char buf[1024];
	int pbuf = 0;

	buf[0] = '\0';
#endif

	/* build args array */
	list_for_each_entry(p, &n->c, b) {
		args[nargs].ptr = exec_NODE(r, p);
#ifdef TRACEDEBUG
		pbuf += snprintf(buf+pbuf, 1024-pbuf, "%p, ", args[nargs].ptr);
#endif
		nargs++;
	}

	ret = n->v.func->f(r->arg, args, nargs);

#ifdef TRACEDEBUG
	*(buf+pbuf-2) = 0;
	DEBUG("%4d: %s(%s) = %p;", n->line, n->v.func->name, buf, ret);
#endif

	return ret;
}

static void *exec_X_SWITCH(struct exec_run *r, struct exec_node *n) {
	return NULL;
}

/**********************************************************************
* 
* X_FOR
*
**********************************************************************/
static void *exec_X_FOR(struct exec_run *r, struct exec_node *n) {
	struct exec_node *init;
	struct exec_node *cond;
	struct exec_node *next;
	struct exec_node *exec;
	void *ret;

	init = container_of(n->c.next, struct exec_node, b);
	cond = container_of(init->b.next, struct exec_node, b);
	next = container_of(cond->b.next, struct exec_node, b);
	exec = container_of(next->b.next, struct exec_node, b);

	/* init */
	exec_NODE(r, init);

	while (1) {
		
		/* cond */
		if (exec_NODE(r, cond) == 0)
			break;

		/* exec */
		ret = exec_NODE(r, exec);
		if ((long)ret == -1)
			break;
		/* implicit
		if ((long)ret == -2)
			continue;
		*/

		/* next */
		exec_NODE(r, next);
	}

	return NULL;
}

/**********************************************************************
* 
* X_WHILE
*
**********************************************************************/
static void *exec_X_WHILE(struct exec_run *r, struct exec_node *n) {
	struct exec_node *cond;
	struct exec_node *exec;
	void *ret;

	cond = container_of(n->c.next, struct exec_node, b);
	exec = container_of(cond->b.next, struct exec_node, b);

	while (1) {

		/* test condition */
		if (exec_NODE(r, cond) == 0)
			break;

		/* exec code */
		ret = exec_NODE(r, exec);
		if ((long)ret == -1)
			break;
		/* implicit
		if ((long)ret == -2)
			continue;
		*/
	}

	return NULL;
}

/**********************************************************************
* 
* X_IF
*
**********************************************************************/
static void *exec_X_IF(struct exec_run *r, struct exec_node *n) {
	struct exec_node *cond;
	struct exec_node *exec;
	struct exec_node *exec_else;

	cond = container_of(n->c.next, struct exec_node, b);
	exec = container_of(cond->b.next, struct exec_node, b);
	exec_else = container_of(exec->b.next, struct exec_node, b);

	if (exec_NODE(r, cond) != 0)
		return exec_NODE(r, exec);
	else if (&exec_else->b != &n->c)
		return exec_NODE(r, exec_else);
	return NULL;
}

/**********************************************************************
* 
* X_BREAK
*
**********************************************************************/
static void *exec_X_BREAK(struct exec_run *r, struct exec_node *n) {
	return NULL;
}
static void *exec_X_CONT(struct exec_run *r, struct exec_node *n) {
	return NULL;
}

/* end point */

/**********************************************************************
* 
* X_VAR
*
**********************************************************************/
static void *exec_X_VAR(struct exec_run *r, struct exec_node *n) {
	return r->vars[n->v.var->offset].ptr;
}

/**********************************************************************
* 
* X_INTEGER
*
**********************************************************************/
static void *exec_X_INTEGER(struct exec_run *r, struct exec_node *n) {
	return n->v.ptr;
}

/**********************************************************************
* 
* X_STRING
*
**********************************************************************/
static void *exec_X_STRING(struct exec_run *r, struct exec_node *n) {
	return n->v.ptr;
}

void exec_run_now(struct exec_run *r) {
	exec_NODE(r, r->n);
}

