/*
 * Copyright (c) 2009 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

#include "exec_internals.h"
#include "templates.h"

/* reserve words in stack */
static inline 
int exec_reserve_stack(struct exec_run *r, int size)
{
	r->stack_ptr += size;
	if (r->stack_ptr >= STACKSIZE) {
		snprintf(r->error, ERROR_LEN, "stack overflow ( > STACKSIZE)");
		return -1;
	}
	return 0;
}

/* restore words into stack */
static inline
int exec_free_stack(struct exec_run *r, int size)
{
	r->stack_ptr -= size;
	if (r->stack_ptr < 0) {
		snprintf(r->error, ERROR_LEN, "stack overflow ( < 0)");
		return -1;
	}
	return 0;
}

/* relative acces into stack */
#define egt(__relindex) \
	r->stack[r->stack_ptr + __relindex]

/* execute function */
#define exec_NODE(__node, __id, __ret) \
	do { \
		struct exec_node *node = __node; \
		if (exec_reserve_stack(r, 2) != 0) \
			return -1; \
		egt(-2).v.ent = __id; \
		egt(-1).v.n = node; \
		switch (node->type) { \
		case X_NULL: goto exec_X_NULL; \
		case X_COLLEC: goto exec_X_COLLEC; \
		case X_PRINT: goto exec_X_PRINT; \
		case X_ADD: \
		case X_SUB: \
		case X_MUL: \
		case X_DIV: \
		case X_MOD: \
		case X_EQUAL: \
		case X_STREQ: \
		case X_DIFF: \
		case X_LT: \
		case X_GT: \
		case X_LE: \
		case X_GE: \
		case X_AND: \
		case X_OR: goto exec_X_TWO_OPERAND; \
		case X_ASSIGN: goto exec_X_ASSIGN; \
		case X_DISPLAY: goto exec_X_DISPLAY; \
		case X_FUNCTION: goto exec_X_FUNCTION; \
		case X_VAR: goto exec_X_VAR; \
		case X_INTEGER: goto exec_X_INTEGER; \
		case X_STRING: goto exec_X_STRING; \
		case X_SWITCH: goto exec_X_SWITCH; \
		case X_FOR: goto exec_X_FOR; \
		case X_WHILE: goto exec_X_WHILE; \
		case X_IF: goto exec_X_IF; \
		case X_BREAK: \
		case X_CONT: \
		default: \
			snprintf(r->error, ERROR_LEN, \
			         "[%s:%d] unknown function code <%d>", \
			         __FILE__, __LINE__, node->type); \
			return -1; \
		} \
		exec_ ## __id: \
		if (exec_free_stack(r, 2) != 0) \
			return -1; \
		/* c'est un peu degeulasse: ja vais chercher \
		   des données dans la stack libérée */ \
		memcpy(&__ret, &egt(1), sizeof(__ret)); \
	} while(0)

/* return to caller
 * for updating this function, run :r !./get_return_points.sh */
#define switchline(index) case index: goto exec_ ## index;
#define exec_return() \
	switch (egt(-2).v.ent) { \
		default: \
		switchline(1) \
		switchline(2) \
		switchline(3) \
		switchline(4) \
		switchline(5) \
		switchline(30) \
		switchline(31) \
		switchline(32) \
		switchline(33) \
		switchline(34) \
		switchline(35) \
		switchline(36) \
		switchline(37) \
		switchline(38) \
		switchline(39) \
		switchline(40) \
		switchline(41) \
		switchline(42) \
			snprintf(r->error, ERROR_LEN, "[%s:%d] error in return code", __FILE__, __LINE__); \
			return -1; \
	}

/* this stats pseudo function */
#define exec_function(xxx) \
	exec_ ## xxx: \
	do

/* this end pseudo function */
#define end_function \
	while(0); \
	snprintf(r->error, ERROR_LEN, \
	         "[%s:%d] this is normally never executed !", __FILE__, __LINE__); \
	return -1;

int exec_run_now(struct exec_run *r) {
	struct exec_args ret;

	/* go back at position in ENOENT write case */
	if (r->retry != NULL)
		goto *r->retry;


	/* call first node */
	exec_NODE(r->n, 1, ret);
	return 0;

/**********************************************************************
* 
* X_NULL
*
**********************************************************************/
	exec_function(X_NULL) {
		memset(&egt(-1), 0, sizeof(egt(-1)));
		exec_return();
	} end_function

/**********************************************************************
* 
* X_DISPLAY
*
**********************************************************************/
	exec_function(X_DISPLAY) {
		/* -5: (n) execute node
		 * -4: (n) children
		 * -3: (char *) display it
		 * -2: (int) cur len
		 * -1: (int) len
		 */
		if (exec_reserve_stack(r, 4) != 0)
			return -1;

		/* get children containing how to display */
		egt(-4).v.n = container_of(egt(-5).v.n->c.next,
		                           struct exec_node, b);

		/* execute children */
		exec_NODE(egt(-4).v.n, 2, egt(-3));

		if (egt(-3).type == XT_PTR ||
		    egt(-3).type == XT_NULL)
			goto X_DISPLAY_end;

		if (egt(-3).type == XT_INTEGER) {
			char convert[128];
			egt(-3).len = snprintf(convert, 128, "%d", egt(-3).v.ent);
			egt(-3).v.str = strdup(convert);
			if (egt(-3).v.str == NULL) {
				snprintf(r->error, ERROR_LEN, "[%s:%d] strdup(\"%s\"): %s",
				         __FILE__, __LINE__, convert, strerror(errno));
				return -1;
			}
			egt(-3).freeit = 1;
		}

		egt(-1).v.ent = egt(-3).len;
		egt(-2).v.ent = egt(-1).v.ent;
	
		/* write */
X_DISPLAY_retry:
		egt(-2).v.ent -= r->w(r->arg,
		                      egt(-3).v.str + ( egt(-1).v.ent - egt(-2).v.ent ),
		                      egt(-2).v.ent);

		/* end of write ? */
		if (egt(-2).v.ent > 0) {
			r->retry = &&X_DISPLAY_retry;
			return 1;
		}

X_DISPLAY_end:
		if (exec_free_stack(r, 4) != 0)
			return -1;
		exec_return();
	} end_function


/**********************************************************************
* 
* X_PRINT
*
**********************************************************************/
	exec_function(X_PRINT) {
		/* -3: (n) execute node
		 * -2: (int) cur len
		 * -1: (int) len
		 */
		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		egt(-1).v.ent = egt(-3).v.n->v.len;
		egt(-2).v.ent = egt(-1).v.ent;

		/* write */
X_PRINT_retry:
		egt(-2).v.ent -= r->w(r->arg,
		                      egt(-3).v.n->v.v.str + ( egt(-1).v.ent - egt(-2).v.ent ),
		                      egt(-2).v.ent);

		/* end of write ? */
		if (egt(-2).v.ent > 0) {
			r->retry = &&X_PRINT_retry;
			return 1;
		}

		if (exec_free_stack(r, 2) != 0)
			return -1;
		exec_return();
	} end_function


/**********************************************************************
* 
* X_COLLEC
*
**********************************************************************/
	exec_function(X_COLLEC) {
		/* -2: (n) execute node
		 * -1: (n) current execute node
		 */
		if (exec_reserve_stack(r, 1) != 0)
			return -1;

		egt(-1).v.n = container_of(egt(-2).v.n->c.next, struct exec_node, b);

		//exec_X_COLLEC_loop:
		while (1) {

			/* if break */
			if (egt(-1).v.n->type == X_BREAK) {
				egt(-2).v.ent = -1;
				break;
			}

			/* if continue */
			if (egt(-1).v.n->type == X_CONT) {
				egt(-2).v.ent = -2;
				break;
			}

			/* execute */
			exec_NODE(egt(-1).v.n, 3, ret);

			/* if "if" return break or continue */
			if (egt(-1).v.n->type == X_IF && ret.v.ent != 0) {
				egt(-2).v.ent = ret.v.ent;
				break;
			}

			/* next var */
			egt(-1).v.n = container_of(egt(-1).v.n->b.next, struct exec_node, b);
			if (&egt(-1).v.n->b == &egt(-2).v.n->c) {
				egt(-2).v.ptr = NULL;
				break;
			}
		}

		if (exec_free_stack(r, 1) != 0)
			return -1;
		exec_return();
	} end_function


/**********************************************************************
* 
* X_ADD
* X_SUB
* X_MUL
* X_DIV
* X_MOD
* X_EQUAL
* X_STREQ
* X_DIFF
* X_LT
* X_GT
* X_LE
* X_GE
* X_AND
* X_OR
*
**********************************************************************/
	exec_function(X_TWO_OPERAND) {
		/* -3 : n
		 * -2 : a et val(a)
		 * -1 : b et val(b)
		 */
		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		egt(-2).v.n = container_of(egt(-3).v.n->c.next, struct exec_node, b);
		egt(-1).v.n = container_of(egt(-2).v.n->b.next, struct exec_node, b);

		exec_NODE(egt(-2).v.n, 4, egt(-2));
		exec_NODE(egt(-1).v.n, 5, egt(-1));

		switch (egt(-3).v.n->type) {
		case X_ADD:   egt(-3).v.ent = egt(-2).v.ent  + egt(-1).v.ent;            break;
		case X_SUB:   egt(-3).v.ent = egt(-2).v.ent  - egt(-1).v.ent;            break;
		case X_MUL:   egt(-3).v.ent = egt(-2).v.ent  * egt(-1).v.ent;            break;
		case X_DIV:   egt(-3).v.ent = egt(-2).v.ent  / egt(-1).v.ent;            break;
		case X_MOD:   egt(-3).v.ent = egt(-2).v.ent  % egt(-1).v.ent;            break;
		case X_EQUAL: egt(-3).v.ent = egt(-2).v.ent == egt(-1).v.ent;            break;
		case X_STREQ: egt(-3).v.ent = strcmp(egt(-2).v.str, egt(-1).v.str) == 0; break;
		case X_DIFF:  egt(-3).v.ent = egt(-2).v.ent != egt(-1).v.ent;            break;
		case X_LT:    egt(-3).v.ent = egt(-2).v.ent  < egt(-1).v.ent;            break;
		case X_GT:    egt(-3).v.ent = egt(-2).v.ent  > egt(-1).v.ent;            break;
		case X_LE:    egt(-3).v.ent = egt(-2).v.ent <= egt(-1).v.ent;            break;
		case X_GE:    egt(-3).v.ent = egt(-2).v.ent >= egt(-1).v.ent;            break;
		case X_AND:   egt(-3).v.ent = egt(-2).v.ent && egt(-1).v.ent;            break;
		case X_OR:    egt(-3).v.ent = egt(-2).v.ent || egt(-1).v.ent;            break;
		/* just for warnings ! */
		case X_NULL:
		case X_COLLEC:
		case X_PRINT:
		case X_ASSIGN:
		case X_DISPLAY:
		case X_FUNCTION:
		case X_VAR:
		case X_INTEGER:
		case X_STRING:
		case X_SWITCH:
		case X_FOR:
		case X_WHILE:
		case X_IF:
		case X_BREAK:
		case X_CONT:
		default:
			snprintf(r->error, ERROR_LEN, 
			         "[%s:%d] unknown function code", __FILE__, __LINE__);
			return -1;
		}
		if (exec_free_stack(r, 2) != 0)
			return -1;
		exec_return();
	} end_function


/**********************************************************************
* 
* X_ASSIGN
*
**********************************************************************/
	exec_function(X_ASSIGN) {
		/* -3 : n
		 * -2 : a
		 * -1 : b
		 */
		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		egt(-2).v.n = container_of(egt(-3).v.n->c.next, struct exec_node, b);
		egt(-1).v.n = container_of(egt(-2).v.n->b.next, struct exec_node, b);

		exec_NODE(egt(-1).v.n, 30, r->vars[ egt(-2).v.n->v.v.var->offset ]);

		memcpy(&egt(-3), &r->vars[ egt(-2).v.n->v.v.var->offset], sizeof(egt(-3)));
		if (exec_free_stack(r, 2) != 0)
			return -1;
		exec_return();
	} end_function


/**********************************************************************
* 
* X_FUNCTION
*
**********************************************************************/
	exec_function(X_FUNCTION) {
		/* - (MXARGS+3) : n
		 * - (MXARGS+2) : children are params
		 * - (MXARGS+1) : nargs
		 * - (       1) : args
		 */
		if (exec_reserve_stack(r, MXARGS + 2) != 0)
			return -1;
		int i;
		struct exec_args args[MXARGS];

		/* nargs = 0 */
		egt(-(MXARGS+1)).v.ent = 0;

		/* build args array */
		egt(-(MXARGS+2)).v.n = container_of(egt(-(MXARGS+3)).v.n->c.next,
		                                    struct exec_node, b);
		while (1) {
			/* le tour est fait */
			if (&egt(-(MXARGS+2)).v.n->b == &egt(-(MXARGS+3)).v.n->c)
				break;

			/* check for aversize */
			if (egt(-(MXARGS+1)).v.ent == MXARGS) {
				snprintf(r->error, ERROR_LEN, "function are more than 20 args");
				return -1;
			}

			exec_NODE(egt(-(MXARGS+2)).v.n, 31, egt(-1-egt(-(MXARGS+1)).v.ent));
			egt(-(MXARGS+1)).v.ent++;

			/* next var */
			egt(-(MXARGS+2)).v.n = container_of(egt(-(MXARGS+2)).v.n->b.next, struct exec_node, b);
		}

X_FUNCTION_retry:

		/* copy args TODO: faut voir si il ne vaut mieux pas donner un morceau de pile ... */
		for (i=0; i<egt(-(MXARGS+1)).v.ent; i++)
			memcpy(&args[i], &egt(-1-i), sizeof(egt(-1-i)));

		/* exec function */
		i = egt(-(MXARGS+3)).v.n->v.v.func->f(r->arg, args,
		                               egt(-(MXARGS+1)).v.ent, &egt(-(MXARGS+3)));

		if (i != 0) {
			r->retry = &&X_FUNCTION_retry;
			return 1;
		}

		if (exec_free_stack(r, MXARGS + 2) != 0)
			return -1;
		exec_return();
	} end_function


/**********************************************************************
* 
* X_SWITCH
*
**********************************************************************/
	exec_function(X_SWITCH) {
		/* -5 : n
		 * -4 : var
		 * -3 : val
		 * -2 : exec
		 * -1 : ok
		 */
		if (exec_reserve_stack(r, 4) != 0)
			return -1;

		/* get var */
		egt(-4).v.n = container_of(egt(-5).v.n->c.next, struct exec_node, b);

		/* set reference into -4. with this, the first get of
		   the loop point on the good value */
		egt(-2).v.n = egt(-4).v.n;

		/* init already exec at 0 */
		egt(-1).v.ent = 0;

		while (1) {

			egt(-3).v.n = container_of(egt(-2).v.n->b.next, struct exec_node, b);
			egt(-2).v.n = container_of(egt(-3).v.n->b.next, struct exec_node, b);

			/* on a fait le tour */
			if (&egt(-3).v.n->b == &egt(-5).v.n->c)
				break;

			/* on execute si:
			   - on a deja executé
			   - c'est default
			   - c'est la bonne valeur */
			exec_NODE(egt(-4).v.n, 32, ret);
			if (egt(-1).v.ent == 1 ||
			    egt(-3).v.n->type == X_NULL ||
			    egt(-3).v.n->v.v.ent == ret.v.ent) {

				/* execute instrictions */
				exec_NODE(egt(-2).v.n, 33, ret);

				/* already matched = 1 */
				egt(-1).v.ent = 1;

				/* c'est fini si on a recu un break; */
				if (ret.v.ent != 0)
					break;
			}
		}

		if (exec_free_stack(r, 4) != 0)
			return -1;
		exec_return();
	} end_function


/**********************************************************************
* 
* X_FOR
*
**********************************************************************/
	exec_function(X_FOR) {
		/* -5 : n
		 * -4 : init
		 * -3 : cond
		 * -2 : next
		 * -1 : exec
		 */
		if (exec_reserve_stack(r, 4) != 0)
			return -1;

		egt(-4).v.n = container_of(egt(-5).v.n->c.next, struct exec_node, b);
		egt(-3).v.n = container_of(egt(-4).v.n->b.next, struct exec_node, b);
		egt(-2).v.n = container_of(egt(-3).v.n->b.next, struct exec_node, b);
		egt(-1).v.n = container_of(egt(-2).v.n->b.next, struct exec_node, b);

		/* init */
		exec_NODE(egt(-4).v.n, 34, ret);

		while (1) {
		
			/* cond */
			exec_NODE(egt(-3).v.n, 35, ret);
			if (ret.v.ent == 0)
				break;

			/* exec */
			exec_NODE(egt(-1).v.n, 36, ret);

			/* check break */
			if (ret.v.ent == -1)
				break;

			/* implicit continue */

			/* next */
			exec_NODE(egt(-2).v.n, 37, ret);
		}

		if (exec_free_stack(r, 4) != 0)
			return -1;
		exec_return();
	} end_function


/**********************************************************************
* 
* X_WHILE
*
**********************************************************************/
	exec_function(X_WHILE) {
		/* -3 : n
		 * -2 : cond
		 * -1 : exec
		 */
		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		egt(-2).v.n = container_of(egt(-3).v.n->c.next, struct exec_node, b);
		egt(-1).v.n = container_of(egt(-2).v.n->b.next, struct exec_node, b);

		while(1) {
		
			/* test condition */
			exec_NODE(egt(-2).v.n, 38, ret);
			if (ret.v.ent == 0)
				break;

			/* exec code */
			exec_NODE(egt(-1).v.n, 39, ret);

			/* check break */
			if (ret.v.ent == -1)
				break;

			/* implicit continue */
		}

		if (exec_free_stack(r, 2) != 0)
			return -1;
		exec_return();
	} end_function


/**********************************************************************
* 
* X_IF
*
**********************************************************************/
	exec_function(X_IF) {
		/* -4 : n
		 * -3 : cond
		 * -2 : exec
		 * -1 : exec_else
		 */
		if (exec_reserve_stack(r, 3) != 0)
			return -1;

		// TODO: deux variables suffisent ... on n'exucute pas le true et le false en meme temps !
		egt(-3).v.n = container_of(egt(-4).v.n->c.next, struct exec_node, b);
		egt(-2).v.n = container_of(egt(-3).v.n->b.next, struct exec_node, b);
		egt(-1).v.n = container_of(egt(-2).v.n->b.next, struct exec_node, b);

		exec_NODE(egt(-3).v.n, 40, ret);

		/* check true */
		if (ret.v.ent != 0)
			exec_NODE(egt(-2).v.n, 41, egt(-4));

		/* if false and else block is present */
		else if (&egt(-1).v.n->b != &egt(-4).v.n->c)
			exec_NODE(egt(-1).v.n, 42, egt(-4));

		/* if false */
		else
			egt(-4).v.ent = 0;

		if (exec_free_stack(r, 3) != 0)
			return -1;
		exec_return();
	} end_function

/* end point functions */

/**********************************************************************
* 
* X_VAR
*
**********************************************************************/
	exec_function(X_VAR) {
		memcpy(&egt(-1), &r->vars[egt(-1).v.n->v.v.var->offset], sizeof(egt(-1)));
		exec_return();
	} end_function

/**********************************************************************
*
* X_INTEGER
*
**********************************************************************/
	exec_function(X_INTEGER) {
		memcpy(&egt(-1), &egt(-1).v.n->v, sizeof(egt(-1)));
		exec_return();
	} end_function

/**********************************************************************
* 
* X_STRING
*
**********************************************************************/
	exec_function(X_STRING) {
		memcpy(&egt(-1), &egt(-1).v.n->v, sizeof(egt(-1)));
		exec_return();
	} end_function

}
