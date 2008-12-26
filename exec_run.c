#include "templates.h"

/* reserve words in stack */
#define exec_reserve_stack(number) \
	do { \
		r->stack_ptr += number; \
		if (r->stack_ptr >= STACKSIZE) { \
			snprintf(r->e->error, ERROR_LEN, "stack overflow ( > STACKSIZE)"); \
			return -1; \
		} \
	} while (0)

/* restore words into stack */
#define exec_free_stack(number) \
	do { \
		r->stack_ptr -= number; \
		if (r->stack_ptr <= 0) { \
			snprintf(r->e->error, ERROR_LEN, "stack overflow ( < 0)"); \
			return -1; \
		} \
	} while (0)

/* relative acces into stack */
#define egt(relindex) \
	r->stack[r->stack_ptr + relindex]

/* execute function */
#define exec_NODE(n, id, ret) \
	do { \
		int nid = id; \
		struct exec_node *node = n; \
		void *varret; \
		\
		exec_reserve_stack(2); \
		egt(-2).ent = nid;
		egt(-1).n = node;
		switch ((node)->type) { \
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
		case X_BREAK:\
		case X_CONT:\
		default: \
			snprintf(r->e->error, ERROR_LEN, \
			         "[%s:%d] unknown function code", __FILE__, __LINE__); \
			return -1; \
		} \
		exec_ ## id: \
		ret = egt(-1).ptr;
		exec_free_stack(2);
	} while(0)

/* return to caller
 * for updating this function, run :r !./get_return_points.sh */
#define switchline(index) case index: goto exec_ ## index;
#define exec_return() \
	switch (egt(-2).ent) { \
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
			snprintf(r->e->error, ERROR_LEN, "[%s:%d] error in return code", __FILE__, __LINE__); \
			return -1; \
	}

/* this stats pseudo function */
#define exec_function(xxx) \
	exec_ ## xxx: \
	do

/* this end pseudo function */
#define end_function \
	while(0); \
	snprintf(r->e->error, ERROR_LEN, \
	         "[%s:%d] this is normally never executed !", __FILE__, __LINE__); \
	return -1;

int exec_run_now(struct exec_run *r) {
	void *ret;

	/* go back at position in ENOENT write case */
	if (r->retry != 0) switch(r->retry) {
	case 1: goto retry_write_1;
	case 2: goto retry_write_2;
	}

	/* call first node */
	exec_NODE(r->n, 1, ret);
	return 0;

/**********************************************************************
* 
* X_NULL
*
**********************************************************************/
	exec_function(X_NULL) {
		egt(-1).ptr = NULL;
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
		exec_reserve_stack(4);

		/* get children containing how to display */
		egt(-4).n = container_of(egt(-5).n->c.next,
		                         struct exec_node, b);

		/* execute children */
		exec_NODE(egt(-4).n, 2, egt(-3).ptr);

		egt(-1).ent = strlen(egt(-3).string);
		egt(-2).ent = egt(-1).ent;
	
		/* write */
		retry_write_1:
		egt(-2).ent -= r->w(r->arg,
		                    egt(-3).string + ( egt(-1).ent - egt(-2).ent ),
		                    egt(-2).ent);

		/* end of write ? */
		if (egt(-2).ent > 0) {
			r->retry = 1;
			return 1;
		}

		egt(-5).ptr = NULL;
		exec_free_stack(4);
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
		exec_reserve_stack(2);

		egt(-1).ent = strlen(egt(-3).n->v.string);
		egt(-2).ent = egt(-1).ent;

		/* write */
		retry_write_2:
		egt(-2).ent -= r->w(r->arg,
		                    egt(-3).n->v.string + ( egt(-1).ent - egt(-2).ent ),
		                    egt(-2).ent);

		/* end of write ? */
		if (egt(-2).ent > 0) {
			r->retry = 2;
			return 1;
		}

		egt(-1).ptr = NULL;
		exec_free_stack(2);
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
		exec_reserve_stack(1);

		egt(-1).n = container_of(egt(-2).n->c.next, struct exec_node, b);

		//exec_X_COLLEC_loop:
		while (1) {

			/* if break */
			if (egt(-1).n->type == X_BREAK) {
				egt(-2).ent = -1;
				break;
			}

			/* if continue */
			if (egt(-1).n->type == X_CONT) {
				egt(-2).ent = -2;
				break;
			}

			/* execute */
			exec_NODE(egt(-1).n, 3, ret);

			/* if "if" return break or continue */
			if (egt(-1).n->type == X_IF && ret != NULL) {
				egt(-2).ptr = ret;
				break;
			}

			/* next var */
			egt(-1).n = container_of(egt(-1).n->b.next, struct exec_node, b);
			if (&egt(-1).n->b == &egt(-2).n->c) {
				egt(-2).ptr = NULL;
				break;
			}
		}

		exec_free_stack(1);
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
		exec_reserve_stack(2);

		egt(-2).n = container_of(egt(-3).n->c.next, struct exec_node, b);
		egt(-1).n = container_of(egt(-2).n->b.next, struct exec_node, b);

		exec_NODE(egt(-2).n, 4, egt(-2).ptr);
		exec_NODE(egt(-1).n, 5, egt(-1).ptr);

		switch (egt(-3).n->type) {
		case X_ADD:   egt(-3).ent = egt(-2).ent  + egt(-1).ent;             break;
		case X_SUB:   egt(-3).ent = egt(-2).ent  - egt(-1).ent;             break;
		case X_MUL:   egt(-3).ent = egt(-2).ent  * egt(-1).ent;             break;
		case X_DIV:   egt(-3).ent = egt(-2).ent  / egt(-1).ent;             break;
		case X_MOD:   egt(-3).ent = egt(-2).ent  % egt(-1).ent;             break;
		case X_EQUAL: egt(-3).ent = egt(-2).ent == egt(-1).ent;             break;
		case X_STREQ: egt(-3).ent = strcmp(egt(-2).string, egt(-1).string); break;
		case X_DIFF:  egt(-3).ent = egt(-2).ent != egt(-1).ent;             break;
		case X_LT:    egt(-3).ent = egt(-2).ent  < egt(-1).ent;             break;
		case X_GT:    egt(-3).ent = egt(-2).ent  > egt(-1).ent;             break;
		case X_LE:    egt(-3).ent = egt(-2).ent <= egt(-1).ent;             break;
		case X_GE:    egt(-3).ent = egt(-2).ent >= egt(-1).ent;             break;
		case X_AND:   egt(-3).ent = egt(-2).ent && egt(-1).ent;             break;
		case X_OR:    egt(-3).ent = egt(-2).ent || egt(-1).ent;             break;
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
			snprintf(r->e->error, ERROR_LEN, 
			         "[%s:%d] unknown function code", __FILE__, __LINE__);
			return -1;
		}
		exec_free_stack(2);
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
		exec_reserve_stack(2);
		void *val;

		egt(-2).n = container_of(egt(-3).n->c.next, struct exec_node, b);
		egt(-1).n = container_of(egt(-2).n->b.next, struct exec_node, b);

		exec_NODE(egt(-1).n, 30, val);
		r->vars[egt(-2).n->v.var->offset].ptr = val;

		egt(-3).ptr = r->vars[egt(-2).n->v.var->offset].ptr;
		exec_free_stack(2);
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
		exec_reserve_stack(MXARGS + 2);
		int i;
		union exec_args args[MXARGS];

		/* nargs = 0 */
		egt(-(MXARGS+1)).ent = 0;

		/* build args array */
		egt(-(MXARGS+2)).n = container_of(egt(-(MXARGS+3)).n->c.next,
		                                  struct exec_node, b);
		while (1) {
			/* le tour est fait */
			if (&egt(-(MXARGS+2)).n->b == &egt(-(MXARGS+3)).n->c)
				break;

			exec_NODE(egt(-(MXARGS+2)).n, 31, ret);
			if (egt(-(MXARGS+1)).ent == MXARGS) {
				snprintf(r->e->error, ERROR_LEN, "function are more than 20 args");
				return -1;
			}
			egt(-1-egt(-(MXARGS+1)).ent).ptr = ret;
			egt(-(MXARGS+1)).ent++;

			/* next var */
			egt(-(MXARGS+2)).n = container_of(egt(-(MXARGS+2)).n->b.next, struct exec_node, b);
		}

		for (i=0; i<egt(-(MXARGS+1)).ent; i++)
			args[i].ptr = egt(-1-i).ptr;

		egt(-(MXARGS+3)).ptr =
		      egt(-(MXARGS+3)).n->v.func->f(r->arg, args, egt(-(MXARGS+1)).ent);

		exec_free_stack(MXARGS + 2);
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
		exec_reserve_stack(4);

		egt(-4).n = container_of(egt(-5).n->c.next, struct exec_node, b);
		egt(-2).n = egt(-4).n;
		egt(-1).ent = 0;

		while (1) {

			egt(-3).n = container_of(egt(-2).n->b.next, struct exec_node, b);
			egt(-2).n = container_of(egt(-3).n->b.next, struct exec_node, b);

			/* on a fait le tour */
			if (&egt(-3).n->b == &egt(-5).n->c)
				break;

			/* on execute si:
			   - on a deja executé
			   - c'est default
			   - c'est la bonne valeur */
			exec_NODE(egt(-4).n, 32, ret);
			if (egt(-1).ent == 1 ||
			    egt(-3).n->type == X_NULL ||
			    egt(-3).n->v.ptr == ret) {
				exec_NODE(egt(-2).n, 33, ret);
				egt(-1).ent = 1;

				/* c'est ficni si on a recu un break; */
				if (ret != NULL)
					break;
			}
		}

		exec_free_stack(4);
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
		exec_reserve_stack(4);

		egt(-4).n = container_of(egt(-5).n->c.next, struct exec_node, b);
		egt(-3).n = container_of(egt(-4).n->b.next, struct exec_node, b);
		egt(-2).n = container_of(egt(-3).n->b.next, struct exec_node, b);
		egt(-1).n = container_of(egt(-2).n->b.next, struct exec_node, b);

		/* init */
		exec_NODE(egt(-4).n, 34, ret);

		while (1) {
		
			/* cond */
			exec_NODE(egt(-3).n, 35, ret);
			if (ret == 0)
				break;

			/* exec */
			exec_NODE(egt(-1).n, 36, ret);

			/* check break */
			if ((long)ret == -1)
				break;

			/* implicit continue */

			/* next */
			exec_NODE(egt(-2).n, 37, ret);
		}

		exec_free_stack(4);
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
		exec_reserve_stack(2);

		egt(-2).n = container_of(egt(-3).n->c.next, struct exec_node, b);
		egt(-1).n = container_of(egt(-2).n->b.next, struct exec_node, b);

		while(1) {
		
			/* test condition */
			exec_NODE(egt(-2).n, 38, ret);
			if (ret == NULL)
				break;

			/* exec code */
			exec_NODE(egt(-1).n, 39, ret);

			/* check break */
			if ((long)ret == -1)
				break;

			/* implicit continue */
		}

		exec_free_stack(2);
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
		exec_reserve_stack(3);

		egt(-3).n = container_of(egt(-4).n->c.next, struct exec_node, b);
		egt(-2).n = container_of(egt(-3).n->b.next, struct exec_node, b);
		egt(-1).n = container_of(egt(-2).n->b.next, struct exec_node, b);

		exec_NODE(egt(-3).n, 40, ret);

		if (ret != NULL)
			exec_NODE(egt(-2).n, 41, egt(-4).ptr);

		else if (&egt(-1).n->b != &egt(-4).n->c)
			exec_NODE(egt(-1).n, 42, egt(-4).ptr);

		else
			egt(-4).ptr = NULL;

		exec_free_stack(3);
		exec_return();
	} end_function

/* end point functions */

/**********************************************************************
* 
* X_VAR
*
**********************************************************************/
	exec_function(X_VAR) {
		egt(-1).ptr = r->vars[egt(-1).n->v.var->offset].ptr;
		exec_return();
	} end_function

/**********************************************************************
 
* X_INTEGER
*
**********************************************************************/
	exec_function(X_INTEGER) {
		egt(-1).ptr = egt(-1).n->v.ptr;
		exec_return();
	} end_function

/**********************************************************************
* 
* X_STRING
*
**********************************************************************/
	exec_function(X_STRING) {
		egt(-1).ptr = egt(-1).n->v.ptr;
		exec_return();
	} end_function

}
