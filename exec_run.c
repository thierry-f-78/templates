/*
 * Copyright (c) 2009-2017 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

#include "exec_internals.h"
#include "templates.h"

/* the size of arg struct, this is used for arg copy */
#define ARG_SIZE sizeof(struct exec_args)

/* reserve words in stack */
static inline int exec_reserve_stack(struct exec_run *r, int size) {
	r->stack_ptr += size;
	if (r->stack_ptr >= STACKSIZE) {
		snprintf(r->error, ERROR_LEN, "stack overflow ( > STACKSIZE)");
		return -1;
	}
	return 0;
}

/* restore words into stack */
static inline int exec_free_stack(struct exec_run *r, int size) {
	r->stack_ptr -= size;
	if (r->stack_ptr < 0) {
		snprintf(r->error, ERROR_LEN, "stack overflow ( < 0)");
		return -1;
	}
	return 0;
}

/*
 * NOTE: for more information on context execution
 * stack and accessors, see doc/stack.pdf
 *
 */

/* ent accessor */
static inline int *get_ent(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].v.ent;
}

/* str accessor */
static inline char **get_str(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].v.str;
}

/* ptr accessor */
static inline void **get_ptr(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].v.ptr;
}

/* var accessor */
static inline struct exec_vars **get_var(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].v.var;
}

/* func accessor */
static inline struct exec_funcs **get_func(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].v.func;
}

/* node accessor */
static inline struct exec_node **get_node(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].v.n;
}

/* len accessor */
static inline int *get_len(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].len;
}

/* type accessor */
static inline enum exec_args_type *get_type(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].type;
}

/* free_it accessor */
static inline char *get_freeit(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].freeit;
}

/* args ptr accessor */
static inline struct exec_args **get_parg(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index].v.arg;
}

/* args accessor */
static inline struct exec_args *get_arg(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index];
}


/* theses accessors return arg value of node, 
 * when the node is in the stack
 */

/* node value str accessor */
static inline char **get_node_str(struct exec_run *r, int relative_index) {
	return &(*get_node(r, relative_index))->v.v.str;
}

/* node value var accessor */
static inline struct exec_vars **get_node_var(struct exec_run *r, int relative_index) {
	return &(*get_node(r, relative_index))->v.v.var;
}

/* node value func accessor */
static inline struct exec_funcs **get_node_func(struct exec_run *r, int relative_index) {
	return &(*get_node(r, relative_index))->v.v.func;
}

/* node value type accessor */
static inline enum exec_type *get_node_type(struct exec_run *r, int relative_index) {
	return &(*get_node(r, relative_index))->type;
}

/* node value ent accessor */
static inline int *get_node_ent(struct exec_run *r, int relative_index) {
	return &(*get_node(r, relative_index))->v.v.ent;
}

/* node value arg accessor */
static inline struct exec_args *get_node_arg(struct exec_run *r, int relative_index) {
	return &(*get_node(r, relative_index))->v;
}

/* node value len accessor */
static inline int *get_node_len(struct exec_run *r, int relative_index) {
	return &(*get_node(r, relative_index))->v.len;
}


/* theses accessors return arg value of pointer on arg,
 * when arg ptr is in the stack
 */

/* arg value ent accessor */
static inline int *get_parg_ent(struct exec_run *r, int relative_index) {
	return &(*get_parg(r, relative_index))->v.ent;
}

/* theses macros permit to use previous accessor functions
 * same as variable
 */
#define ENT(__var)       (*get_ent(r, __var))       /* int                   */
#define STR(__var)       (*get_str(r, __var))       /* char *                */
#define PTR(__var)       (*get_ptr(r, __var))       /* void *                */
#define VAR(__var)       (*get_var(r, __var))       /* struct exec_vars *    */
#define FUNC(__var)      (*get_func(r, __var))      /* struct exec_funcs *   */
#define NODE(__var)      (*get_node(r, __var))      /* struct exec_node *    */
#define LEN(__var)       (*get_len(r, __var))       /* int                   */
#define TYPE(__var)      (*get_type(r, __var))      /* enum exec_args_type   */
#define FREEIT(__var)    (*get_freeit(r, __var))    /* char                  */
#define PARG(__var)      (*get_parg(r, __var))      /* struct exec_args *    */
#define ARG(__var)         get_arg(r, __var)        /* struct exec_args *    */

#define NODE_ENT(__var)  (*get_node_ent(r, __var))  /* int                   */
#define NODE_STR(__var)  (*get_node_str(r, __var))  /* char *                */
#define NODE_VAR(__var)  (*get_node_var(r, __var))  /* struct exec_vars *    */
#define NODE_FUNC(__var) (*get_node_func(r, __var)) /* struct exec_funcs *   */
#define NODE_TYPE(__var) (*get_node_type(r, __var)) /* enum exec_type        */
#define NODE_LEN(__var)  (*get_node_len(r, __var))  /* int                   */
#define NODE_ARG(__var)    get_node_arg(r, __var)   /* struct exec_args *    */

#define PARG_ENT(__var)  (*get_parg_ent(r, __var))  /* int                   */


/* return children of node */
static inline struct exec_node *exec_get_children(struct exec_node *n) {
	return container_of(n->c.next, struct exec_node, b);
}

/* return brother of node */
static inline struct exec_node *exec_get_brother(struct exec_node *n) {
	return container_of(n->b.next, struct exec_node, b);
}

/* execute function */
#define EXEC_NODE(__node, __label, __ret) \
	do { \
		/* NOTE, if __node or __ret are relative, \
		 * theses take bad value after reserving stack \
		 */ \
		struct exec_node *node = __node; \
		struct exec_args *arg = __ret; \
		if (exec_reserve_stack(r, 3) != 0) \
			return -1; \
		PTR(-3)  = && exec_ ## __label; \
		PARG(-2) = arg; \
		NODE(-1) = node; \
		goto *goto_function[node->type]; \
		exec_ ## __label: \
		if (exec_free_stack(r, 3) != 0) \
			return -1; \
	} while(0)

/* return to caller */
#define EXEC_RETURN() \
	goto *PTR(-3);

/* this stats pseudo function */
#define EXEC_FUNCTION(__xxx) \
	exec_ ## __xxx: \
	do

/* this end pseudo function */
#define END_FUNCTION \
	while(0); \
	snprintf(r->error, ERROR_LEN, \
	         "[%s:%d] this is normally never executed !", __FILE__, __LINE__); \
	return -1;

int exec_run_now(struct exec_run *r) {
	struct exec_args ret_node;

	static const void *goto_function[] = {
		[X_NULL]     = && exec_X_NULL,
		[X_COLLEC]   = && exec_X_COLLEC,
		[X_PRINT]    = && exec_X_PRINT,
		[X_ADD]      = && exec_X_TWO_OPERAND,
		[X_SUB]      = && exec_X_TWO_OPERAND,
		[X_MUL]      = && exec_X_TWO_OPERAND,
		[X_DIV]      = && exec_X_TWO_OPERAND,
		[X_MOD]      = && exec_X_TWO_OPERAND,
		[X_EQUAL]    = && exec_X_TWO_OPERAND,
		[X_STREQ]    = && exec_X_TWO_OPERAND,
		[X_DIFF]     = && exec_X_TWO_OPERAND,
		[X_LT]       = && exec_X_TWO_OPERAND,
		[X_GT]       = && exec_X_TWO_OPERAND,
		[X_LE]       = && exec_X_TWO_OPERAND,
		[X_GE]       = && exec_X_TWO_OPERAND,
		[X_AND]      = && exec_X_TWO_OPERAND,
		[X_OR]       = && exec_X_TWO_OPERAND,
		[X_ASSIGN]   = && exec_X_ASSIGN,
		[X_DISPLAY]  = && exec_X_DISPLAY,
		[X_FUNCTION] = && exec_X_FUNCTION,
		[X_VAR]      = && exec_X_VAR,
		[X_INTEGER]  = && exec_X_INTEGER,
		[X_STRING]   = && exec_X_STRING,
		[X_SWITCH]   = && exec_X_SWITCH,
		[X_FOR]      = && exec_X_FOR,
		[X_WHILE]    = && exec_X_WHILE,
		[X_IF]       = && exec_X_IF
	};

	/* go back at position in ENOENT write case */
	if (r->retry != NULL)
		goto *r->retry;


	/* call first node */
	EXEC_NODE(r->n, 1, &ret_node);
	return 0;

/**********************************************************************
* 
* X_NULL
*
**********************************************************************/
	EXEC_FUNCTION ( X_NULL ) {
		/* -2: return NULL
		 * -1: node
		 */

		static const int ret = -2;

		PARG(ret)->v.ptr = NULL;
		PARG(ret)->len = 0;
		PARG(ret)->type = XT_NULL;
		PARG(ret)->freeit = 0;

		EXEC_RETURN();

	} END_FUNCTION

/**********************************************************************
* 
* X_DISPLAY
*
**********************************************************************/
	EXEC_FUNCTION ( X_DISPLAY ) {
		/* -6: return --> no return value !
		 * -5: (n) execute node
		 * -4: (n) children
		 * -3: (char *) display it
		 * -2: (int) cur len
		 * -1: (int) len
		 */

		/* no return ...         = -6; */
		static const int node    = -5;
		static const int child   = -4;
		static const int dsp     = -3;
		static const int cur_len = -2;
		static const int len     = -1;
		char convert[128];

		if (exec_reserve_stack(r, 4) != 0)
			return -1;

		/* get children containing how to display */
		NODE(child) = exec_get_children(NODE(node));

		/* execute children */
		EXEC_NODE(NODE(child), 2, ARG(dsp));

		/* convert pointer into displayable format */
		if (TYPE(dsp) == XT_PTR) {
			LEN(dsp) = snprintf(convert, 128, "%p", PTR(dsp));
			STR(dsp) = strdup(convert);
			if (STR(dsp) == NULL) {
				snprintf(r->error, ERROR_LEN, "[%s:%d] strdup(\"%s\"): %s",
				         __FILE__, __LINE__, convert, strerror(errno));
				return -1;
			}
			FREEIT(dsp) = 1;
		}

		/* convert NULL into displayable format */
		else if (TYPE(dsp) == XT_NULL) {
			STR(dsp) = "(null)";
			FREEIT(dsp) = 0;
		}

		/* convert interger into displayable format */
		else if (TYPE(dsp) == XT_INTEGER) {
			LEN(dsp) = snprintf(convert, 128, "%d", ENT(dsp));
			STR(dsp) = strdup(convert);
			if (STR(dsp) == NULL) {
				snprintf(r->error, ERROR_LEN, "[%s:%d] strdup(\"%s\"): %s",
				         __FILE__, __LINE__, convert, strerror(errno));
				return -1;
			}
			FREEIT(dsp) = 1;
		}

		/* if type is XT_STRING, do none */

		ENT(len)     = LEN(dsp);
		ENT(cur_len) = LEN(dsp);
	
		/* write */
X_DISPLAY_retry:
		ENT(cur_len) -= r->w(r->arg,
		                     STR(dsp) + ( ENT(len) - ENT(cur_len) ),
		                     ENT(cur_len));

		/* end of write ? */
		if (ENT(cur_len) > 0) {
			r->retry = &&X_DISPLAY_retry;
			return 1;
		}

		if (exec_free_stack(r, 4) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_PRINT
*
**********************************************************************/
	EXEC_FUNCTION ( X_PRINT ) {
		/* -4: return value
		 * -3: node
		 * -2: cur len
		 * -1: len
		 */

		/* no return value ...   = -4; */
		static const int n       = -3;
		static const int cur_len = -2;
		static const int len     = -1;

		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		ENT(len)     = NODE_LEN(n);
		ENT(cur_len) = NODE_LEN(n);

		/* write */
X_PRINT_retry:
		ENT(cur_len) -= r->w(r->arg,
		                     NODE_STR(n) + ( ENT(len) - ENT(cur_len) ),
		                     ENT(cur_len));

		/* end of write ? */
		if (ENT(cur_len) > 0) {
			r->retry = &&X_PRINT_retry;
			return 1;
		}

		if (exec_free_stack(r, 2) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_COLLEC
*
**********************************************************************/
	EXEC_FUNCTION ( X_COLLEC ) {
		/* -4: return code
		 * -3: executed node
		 * -2: current execute node
		 * -1: ret
		 */
		static const int ret     = -4;
		static const int n       = -3;
		static const int exec    = -2;
		static const int retval  = -1;

		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		NODE(exec) = exec_get_children(NODE(n));

		//exec_X_COLLEC_loop:
		while (1) {

			/* if break */
			if (NODE(exec)->type == X_BREAK) {
				PARG_ENT(ret) = -1;
				break;
			}

			/* if continue */
			if (NODE(exec)->type == X_CONT) {
				PARG_ENT(ret) = -2;
				break;
			}

			/* execute */
			EXEC_NODE(NODE(exec), 3, ARG(retval));

			/* if "if" return break or continue */
			if (NODE(exec)->type == X_IF && ENT(retval) != 0) {
				PARG_ENT(ret) = ENT(retval);
				break;
			}

			/* next var */
			NODE(exec) = exec_get_brother(NODE(exec));
			if (&NODE(exec)->b == &NODE(n)->c) {
				PARG_ENT(ret) = 0;
				break;
			}
		}

		if (exec_free_stack(r, 2) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


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
	EXEC_FUNCTION ( X_TWO_OPERAND ) {
		/* -4 : ret
		 * -3 : n
		 * -2 : a et val(a)
		 * -1 : b et val(b)
		 */
		static const int ret = -4;
		static const int n   = -3;
		static const int a   = -2;
		static const int b   = -1;
		static const char *aa;
		static const char *bb;

		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		NODE(a) = exec_get_children(NODE(n));
		NODE(b) = exec_get_brother(NODE(a));

		EXEC_NODE(NODE(a), 4, ARG(a));
		EXEC_NODE(NODE(b), 5, ARG(b));

		switch (NODE(n)->type) {
		case X_ADD:   PARG_ENT(ret) = ENT(a)  + ENT(b);            break;
		case X_SUB:   PARG_ENT(ret) = ENT(a)  - ENT(b);            break;
		case X_MUL:   PARG_ENT(ret) = ENT(a)  * ENT(b);            break;
		case X_DIV:   PARG_ENT(ret) = ENT(a)  / ENT(b);            break;
		case X_MOD:   PARG_ENT(ret) = ENT(a)  % ENT(b);            break;
		case X_EQUAL: PARG_ENT(ret) = ENT(a) == ENT(b);            break;
		case X_DIFF:  PARG_ENT(ret) = ENT(a) != ENT(b);            break;
		case X_LT:    PARG_ENT(ret) = ENT(a)  < ENT(b);            break;
		case X_GT:    PARG_ENT(ret) = ENT(a)  > ENT(b);            break;
		case X_LE:    PARG_ENT(ret) = ENT(a) <= ENT(b);            break;
		case X_GE:    PARG_ENT(ret) = ENT(a) >= ENT(b);            break;
		case X_AND:   PARG_ENT(ret) = ENT(a) && ENT(b);            break;
		case X_OR:    PARG_ENT(ret) = ENT(a) || ENT(b);            break;
		case X_STREQ: 
			aa = STR(a);
			bb = STR(b);
			if (aa == NULL)
				aa = "";
			if (bb == NULL)
				bb = "";
			PARG_ENT(ret) = strcmp(aa, bb) == 0;
			break;
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

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_ASSIGN
*
**********************************************************************/
	EXEC_FUNCTION ( X_ASSIGN ) {
		/* -4 : ret
		 * -3 : n
		 * -2 : a
		 * -1 : b
		 */
		/* no return ....    = -4; */
		static const int n   = -3;
		static const int a   = -2;
		static const int b   = -1;

		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		/* a = fils 1 */
		NODE(a) = exec_get_children(NODE(n));

		/* b = fils 2 */
		NODE(b) = exec_get_brother(NODE(a));

		/* exec b, resultat -> a */
		EXEC_NODE(NODE(b), 30, &r->vars[ NODE_VAR(a)->offset ]);

		if (exec_free_stack(r, 2) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_FUNCTION
*
**********************************************************************/
	EXEC_FUNCTION ( X_FUNCTION ) {
		/* - (MXARGS+4) : ret
		 * - (MXARGS+3) : n
		 * - (MXARGS+2) : children are params
		 * - (MXARGS+1) : nargs
		 * - (       1) : args
		 */
		static const int ret     = -(MXARGS+4);
		static const int n       = -(MXARGS+3);
		static const int cap     = -(MXARGS+2);
		static const int nargs   = -(MXARGS+1);
		static const int args    = -1;

		int i;
		struct exec_args stack_args[MXARGS];

		if (exec_reserve_stack(r, MXARGS + 2) != 0)
			return -1;

		/* nargs = 0 */
		ENT(nargs) = 0;

		/* build args array */
		for ( NODE(cap) = exec_get_children(NODE(n));
		      &NODE(cap)->b != &NODE(n)->c;
		      NODE(cap) = exec_get_brother(NODE(cap)) ) {

			/* check for args oversize */
			if (ENT(nargs) == MXARGS) {
				snprintf(r->error, ERROR_LEN, "function are more than 20 args");
				return -1;
			}

			/* execute argument and store it into stack */
			EXEC_NODE ( NODE(cap), 31, ARG(args - ENT(nargs)) );

			/* one more argument */
			ENT(nargs)++;
		}

X_FUNCTION_retry:

		/* copy args TODO: faut voir si il ne vaut mieux pas donner un morceau de pile ... */
		for (i = 0; i < ENT(nargs); i++)
			memcpy(&stack_args[i], ARG(args - i), ARG_SIZE);

		/* exec function */
		i = NODE_FUNC(n)->f(r->arg, stack_args, ENT(nargs), PARG(ret));

		if (i != 0) {
			r->retry = &&X_FUNCTION_retry;
			return 1;
		}

		if (exec_free_stack(r, MXARGS + 2) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_SWITCH
*
**********************************************************************/
	EXEC_FUNCTION ( X_SWITCH ) {
		/* -7 : return code
		 * -6 : node
		 * -5 : var
		 * -4 : val
		 * -3 : exec
		 * -2 : ok
		 * -1 : ret
		 */
		/* no return ...      = -7; */
		static const int node = -6;
		static const int var  = -5;
		static const int val  = -4;
		static const int exec = -3;
		static const int ok   = -2;
		static const int ret  = -1;

		if (exec_reserve_stack(r, 5) != 0)
			return -1;

		/* get var */
		NODE(var) = exec_get_children(NODE(node));

		/* set reference into -4. with this, the first get of
		   the loop point on the good value */
		NODE(exec) = NODE(var);

		/* init already exec at 0 */
		ENT(ok) = 0;

		while (1) {

			NODE(val)  = exec_get_brother(NODE(exec));
			NODE(exec) = exec_get_brother(NODE(val));

			/* on a fait le tour */
			if (&NODE(val)->b == &NODE(node)->c)
				break;

			/* on execute si:
			   - on a deja executé
			   - c'est default
			   - c'est la bonne valeur */
			EXEC_NODE(NODE(var), 32, ARG(ret));
			if (ENT(ok) == 1 ||
			    NODE_TYPE(val) == X_NULL ||
			    NODE_ENT(val) == ENT(ret)) {

				/* execute instrictions */
				EXEC_NODE(NODE(exec), 33, ARG(ret));

				/* already matched = 1 */
				ENT(ok) = 1;

				/* c'est fini si on a recu un break; */
				if (ENT(ret) != 0)
					break;
			}
		}

		if (exec_free_stack(r, 5) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_FOR
*
**********************************************************************/
	EXEC_FUNCTION ( X_FOR ) {
		/* -7 : return code
		 * -6 : n
		 * -5 : init
		 * -4 : cond
		 * -3 : next
		 * -2 : exec
		 * -1 : ret
		 */
		/* no return ...      = -7; */
		static const int n    = -6;
		static const int init = -5;
		static const int cond = -4;
		static const int next = -3;
		static const int exec = -2;
		static const int ret  = -1;

		if (exec_reserve_stack(r, 5) != 0)
			return -1;

		/* get 4 parameters for for loop */
		NODE(init) = exec_get_children(NODE(n));
		NODE(cond) = exec_get_brother(NODE(init));
		NODE(next) = exec_get_brother(NODE(cond));
		NODE(exec) = exec_get_brother(NODE(next));

		/* execute init */
		EXEC_NODE(NODE(init), 34, ARG(ret));

		while (1) {
		
			/* execute cond - quit if false */
			EXEC_NODE(NODE(cond), 35, ARG(ret));
			if (ENT(ret) == 0)
				break;

			/* execute code */
			EXEC_NODE(NODE(exec), 36, ARG(ret));

			/* check break */
			if (ENT(ret) == -1)
				break;

			/* implicit continue */

			/* execute next */
			EXEC_NODE(NODE(next), 37, ARG(ret));
		}

		if (exec_free_stack(r, 5) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_WHILE
*
**********************************************************************/
	EXEC_FUNCTION ( X_WHILE ) {
		/* -5 : return code
		 * -4 : n
		 * -3 : cond
		 * -2 : exec
		 * -1 : ret
		 */
		/* no return ...         = -5; */
		static const int retcode = -4;
		static const int cond    = -3;
		static const int exec    = -2;
		static const int ret     = -1;

		if (exec_reserve_stack(r, 3) != 0)
			return -1;

		NODE(cond) = exec_get_children(NODE(retcode));
		NODE(exec) = exec_get_brother(NODE(cond));

		while(1) {
		
			/* execute condition - if false break */
			EXEC_NODE(NODE(cond), 38, ARG(ret));
			if (ENT(ret) == 0)
				break;

			/* execute code */
			EXEC_NODE(NODE(exec), 39, ARG(ret));

			/* check break */
			if (ENT(ret) == -1)
				break;

			/* implicit continue */
		}

		if (exec_free_stack(r, 3) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_IF
*
**********************************************************************/
	EXEC_FUNCTION ( X_IF ) {
		/* -6 : return code
		 * -5 : n
		 * -4 : cond
		 * -3 : exec
		 * -2 : exec_else
		 * -1 : ret
		 */
		static const int retc      = -6;
		static const int n         = -5;
		static const int cond      = -4;
		static const int exec      = -3;
		static const int exec_else = -2;
		static const int ret       = -1;

		if (exec_reserve_stack(r, 4) != 0)
			return -1;

		NODE(cond)      = exec_get_children(NODE(n));
		NODE(exec)      = exec_get_brother(NODE(cond));
		NODE(exec_else) = exec_get_brother(NODE(exec));

		/* execute condition */
		EXEC_NODE(NODE(cond), 40, ARG(ret));

		/* if true, execute "exec" */
		if (ENT(ret) != 0)
			EXEC_NODE(NODE(exec), 41, PARG(retc));

		/* if false and else block is present */
		else if (&NODE(exec_else)->b != &NODE(n)->c)
			EXEC_NODE(NODE(exec_else), 42, PARG(retc));

		/* if false */
		else
			PARG_ENT(retc) = 0;

		if (exec_free_stack(r, 4) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION

/* end point functions */

/**********************************************************************
* 
* X_VAR
*
**********************************************************************/
	EXEC_FUNCTION ( X_VAR ) {

		memcpy( PARG(-2), &r->vars[NODE_VAR(-1)->offset], ARG_SIZE);

		EXEC_RETURN();

	} END_FUNCTION

/**********************************************************************
*
* X_INTEGER
*
**********************************************************************/
	EXEC_FUNCTION ( X_INTEGER ) {

		memcpy( PARG(-2), NODE_ARG(-1), ARG_SIZE);

		EXEC_RETURN();

	} END_FUNCTION

/**********************************************************************
* 
* X_STRING
*
**********************************************************************/
	EXEC_FUNCTION ( X_STRING ) {

		memcpy( PARG(-2), NODE_ARG(-1), ARG_SIZE);

		EXEC_RETURN();

	} END_FUNCTION

}
