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

/* args accessor */
static inline struct exec_args *get_arg(struct exec_run *r, int relative_index) {
	return &r->stack[r->stack_ptr + relative_index];
}


/* theses macros permit to use previous accessor functions
 * same as variable
 */
#define ENT(__var)    (*get_ent(r, __var))
#define STR(__var)    (*get_str(r, __var))
#define PTR(__var)    (*get_ptr(r, __var))
#define VAR(__var)    (*get_var(r, __var))
#define FUNC(__var)   (*get_func(r, __var))
#define NODE(__var)   (*get_node(r, __var))
#define LEN(__var)    (*get_len(r, __var))
#define TYPE(__var)   (*get_type(r, __var))
#define FREEIT(__var) (*get_freeit(r, __var))
#define ARG(__var)    get_arg(r, __var)

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
		struct exec_node *node = __node; \
		if (exec_reserve_stack(r, 2) != 0) \
			return -1; \
		PTR(-2) = &&exec_ ## __label; \
		NODE(-1) = node; \
		if (goto_function[node->type] == NULL) { \
			snprintf(r->error, ERROR_LEN, \
			         "[%s:%d] unknown function code <%d>", \
			         __FILE__, __LINE__, node->type); \
			return -1; \
		} \
		goto *goto_function[node->type];\
		exec_ ## __label: \
		if (exec_free_stack(r, 2) != 0) \
			return -1; \
		/* c'est un peu degeulasse: ja vais chercher \
		   des données dans la stack libérée */ \
		memcpy(__ret, ARG(1), sizeof(*__ret)); \
	} while(0)

/* return to caller */
#define EXEC_RETURN() \
	if (PTR(-2) == NULL) { \
		snprintf(r->error, ERROR_LEN, "[%s:%d] error in return code", \
		         __FILE__, __LINE__); \
		return -1; \
	} \
	goto *PTR(-2);

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
	struct exec_args ret;

	static const void *goto_function[] = {
		[X_NULL]     = &&exec_X_NULL,
		[X_COLLEC]   = &&exec_X_COLLEC,
		[X_PRINT]    = &&exec_X_PRINT,
		[X_ADD]      = &&exec_X_TWO_OPERAND,
		[X_SUB]      = &&exec_X_TWO_OPERAND,
		[X_MUL]      = &&exec_X_TWO_OPERAND,
		[X_DIV]      = &&exec_X_TWO_OPERAND,
		[X_MOD]      = &&exec_X_TWO_OPERAND,
		[X_EQUAL]    = &&exec_X_TWO_OPERAND,
		[X_STREQ]    = &&exec_X_TWO_OPERAND,
		[X_DIFF]     = &&exec_X_TWO_OPERAND,
		[X_LT]       = &&exec_X_TWO_OPERAND,
		[X_GT]       = &&exec_X_TWO_OPERAND,
		[X_LE]       = &&exec_X_TWO_OPERAND,
		[X_GE]       = &&exec_X_TWO_OPERAND,
		[X_AND]      = &&exec_X_TWO_OPERAND,
		[X_OR]       = &&exec_X_TWO_OPERAND,
		[X_ASSIGN]   = &&exec_X_ASSIGN,
		[X_DISPLAY]  = &&exec_X_DISPLAY,
		[X_FUNCTION] = &&exec_X_FUNCTION,
		[X_VAR]      = &&exec_X_VAR,
		[X_INTEGER]  = &&exec_X_INTEGER,
		[X_STRING]   = &&exec_X_STRING,
		[X_SWITCH]   = &&exec_X_SWITCH,
		[X_FOR]      = &&exec_X_FOR,
		[X_WHILE]    = &&exec_X_WHILE,
		[X_IF]       = &&exec_X_IF
	};

	/* go back at position in ENOENT write case */
	if (r->retry != NULL)
		goto *r->retry;


	/* call first node */
	EXEC_NODE(r->n, 1, &ret);
	return 0;

/**********************************************************************
* 
* X_NULL
*
**********************************************************************/
	EXEC_FUNCTION(X_NULL) {
		memset(ARG(-1), 0, sizeof(ARG(-1)));
		EXEC_RETURN();
	} END_FUNCTION

/**********************************************************************
* 
* X_DISPLAY
*
**********************************************************************/
	EXEC_FUNCTION(X_DISPLAY) {
		/* -5: (n) execute node
		 * -4: (n) children
		 * -3: (char *) display it
		 * -2: (int) cur len
		 * -1: (int) len
		 */
		static const int retnode = -5;
		static const int child   = -4;
		static const int dsp     = -3;
		static const int cur_len = -2;
		static const int len     = -1;
		char convert[128];

		if (exec_reserve_stack(r, 4) != 0)
			return -1;

		/* get children containing how to display */
		NODE(child) = exec_get_children(NODE(retnode));

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
	EXEC_FUNCTION(X_PRINT) {
		/* -3: (n) execute node
		 * -2: (int) cur len
		 * -1: (int) len
		 */
		static const int node    = -3;
		static const int cur_len = -2;
		static const int len     = -1;

		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		ENT(len)     = NODE(node)->v.len;
		ENT(cur_len) = NODE(node)->v.len;

		/* write */
X_PRINT_retry:
		ENT(cur_len) -= r->w(r->arg,
		                     NODE(node)->v.v.str + ( ENT(len) - ENT(cur_len) ),
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
	EXEC_FUNCTION(X_COLLEC) {
		/* -2: (n) execute node
		 * -1: (n) current execute node
		 */
		static const int retcode = -2;
		static const int exec    = -1;

		if (exec_reserve_stack(r, 1) != 0)
			return -1;

		NODE(exec) = exec_get_children(NODE(retcode));

		//exec_X_COLLEC_loop:
		while (1) {

			/* if break */
			if (NODE(exec)->type == X_BREAK) {
				ENT(retcode) = -1;
				break;
			}

			/* if continue */
			if (NODE(exec)->type == X_CONT) {
				ENT(retcode) = -2;
				break;
			}

			/* execute */
			EXEC_NODE(NODE(exec), 3, &ret);

			/* if "if" return break or continue */
			if (NODE(exec)->type == X_IF && ret.v.ent != 0) {
				ENT(retcode) = ret.v.ent;
				break;
			}

			/* next var */
			NODE(exec) = exec_get_brother(NODE(exec));
			if (&NODE(exec)->b == &NODE(retcode)->c) {
				PTR(retcode) = NULL;
				break;
			}
		}

		if (exec_free_stack(r, 1) != 0)
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
	EXEC_FUNCTION(X_TWO_OPERAND) {
		/* -3 : n
		 * -2 : a et val(a)
		 * -1 : b et val(b)
		 */
		static const int n = -3;
		static const int a = -2;
		static const int b = -1;

		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		NODE(a) = exec_get_children(NODE(n));
		NODE(b) = exec_get_brother(NODE(a));

		EXEC_NODE(NODE(a), 4, ARG(a));
		EXEC_NODE(NODE(b), 5, ARG(b));

		switch (NODE(n)->type) {
		case X_ADD:   ENT(n) = ENT(a)  + ENT(b);            break;
		case X_SUB:   ENT(n) = ENT(a)  - ENT(b);            break;
		case X_MUL:   ENT(n) = ENT(a)  * ENT(b);            break;
		case X_DIV:   ENT(n) = ENT(a)  / ENT(b);            break;
		case X_MOD:   ENT(n) = ENT(a)  % ENT(b);            break;
		case X_EQUAL: ENT(n) = ENT(a) == ENT(b);            break;
		case X_STREQ: ENT(n) = strcmp(STR(a), STR(b)) == 0; break;
		case X_DIFF:  ENT(n) = ENT(a) != ENT(b);            break;
		case X_LT:    ENT(n) = ENT(a)  < ENT(b);            break;
		case X_GT:    ENT(n) = ENT(a)  > ENT(b);            break;
		case X_LE:    ENT(n) = ENT(a) <= ENT(b);            break;
		case X_GE:    ENT(n) = ENT(a) >= ENT(b);            break;
		case X_AND:   ENT(n) = ENT(a) && ENT(b);            break;
		case X_OR:    ENT(n) = ENT(a) || ENT(b);            break;
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
	EXEC_FUNCTION(X_ASSIGN) {
		/* -3 : n
		 * -2 : a
		 * -1 : b
		 */
		static const int n = -3;
		static const int a = -2;
		static const int b = -1;

		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		/* a = fils 1 */
		NODE(a) = exec_get_children(NODE(n));

		/* b = fils 2 */
		NODE(b) = exec_get_brother(NODE(a));

		/* exec b, resultat -> a */
		EXEC_NODE(NODE(b), 30, &r->vars[ NODE(a)->v.v.var->offset ]);

		memcpy( ARG(n), &r->vars[ NODE(a)->v.v.var->offset ], sizeof(*ARG(n)) );

		if (exec_free_stack(r, 2) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_FUNCTION
*
**********************************************************************/
	EXEC_FUNCTION(X_FUNCTION) {
		/* - (MXARGS+3) : n
		 * - (MXARGS+2) : children are params
		 * - (MXARGS+1) : nargs
		 * - (       1) : args
		 */
		static const int retcode = -(MXARGS+3);
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
		for ( NODE(cap) = exec_get_children(NODE(retcode));
		      &NODE(cap)->b != &NODE(retcode)->c;
		      NODE(cap) = exec_get_brother(NODE(cap)) ) {

			/* check for aversize */
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
			memcpy(&stack_args[i], ARG(args - i), sizeof(*ARG(args - i)));

		/* exec function */
		i = NODE(retcode)->v.v.func->f(r->arg, stack_args, ENT(nargs), ARG(retcode));

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
	EXEC_FUNCTION(X_SWITCH) {
		/* -5 : n
		 * -4 : var
		 * -3 : val
		 * -2 : exec
		 * -1 : ok
		 */
		static const int n    = -5;
		static const int var  = -4;
		static const int val  = -3;
		static const int exec = -2;
		static const int ok   = -1;

		if (exec_reserve_stack(r, 4) != 0)
			return -1;

		/* get var */
		NODE(var) = exec_get_children(NODE(n));

		/* set reference into -4. with this, the first get of
		   the loop point on the good value */
		NODE(exec) = NODE(var);

		/* init already exec at 0 */
		ENT(ok) = 0;

		while (1) {

			NODE(val)  = exec_get_brother(NODE(exec));
			NODE(exec) = exec_get_brother(NODE(val));

			/* on a fait le tour */
			if (&NODE(val)->b == &NODE(n)->c)
				break;

			/* on execute si:
			   - on a deja executé
			   - c'est default
			   - c'est la bonne valeur */
			EXEC_NODE(NODE(var), 32, &ret);
			if (ENT(ok) == 1 ||
			    NODE(val)->type == X_NULL ||
			    NODE(val)->v.v.ent == ret.v.ent) {

				/* execute instrictions */
				EXEC_NODE(NODE(exec), 33, &ret);

				/* already matched = 1 */
				ENT(ok) = 1;

				/* c'est fini si on a recu un break; */
				if (ret.v.ent != 0)
					break;
			}
		}

		if (exec_free_stack(r, 4) != 0)
			return -1;
		EXEC_RETURN();
	} END_FUNCTION


/**********************************************************************
* 
* X_FOR
*
**********************************************************************/
	EXEC_FUNCTION(X_FOR) {
		/* -5 : n
		 * -4 : init
		 * -3 : cond
		 * -2 : next
		 * -1 : exec
		 */
		static const int n    = -5;
		static const int init = -4;
		static const int cond = -3;
		static const int next = -2;
		static const int exec = -1;

		if (exec_reserve_stack(r, 4) != 0)
			return -1;

		/* get 4 parameters for for loop */
		NODE(init) = exec_get_children(NODE(n));
		NODE(cond) = exec_get_brother(NODE(init));
		NODE(next) = exec_get_brother(NODE(cond));
		NODE(exec) = exec_get_brother(NODE(next));

		/* execute init */
		EXEC_NODE(NODE(init), 34, &ret);

		while (1) {
		
			/* execute cond - quit if false */
			EXEC_NODE(NODE(cond), 35, &ret);
			if (ret.v.ent == 0)
				break;

			/* execute code */
			EXEC_NODE(NODE(exec), 36, &ret);

			/* check break */
			if (ret.v.ent == -1)
				break;

			/* implicit continue */

			/* execute next */
			EXEC_NODE(NODE(next), 37, &ret);
		}

		if (exec_free_stack(r, 4) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_WHILE
*
**********************************************************************/
	EXEC_FUNCTION(X_WHILE) {
		/* -3 : n
		 * -2 : cond
		 * -1 : exec
		 */
		static const int retcode = -3;
		static const int cond    = -2;
		static const int exec    = -1;

		if (exec_reserve_stack(r, 2) != 0)
			return -1;

		NODE(cond) = exec_get_children(NODE(retcode));
		NODE(exec) = exec_get_brother(NODE(cond));

		while(1) {
		
			/* execute condition - if false break */
			EXEC_NODE(NODE(cond), 38, &ret);
			if (ret.v.ent == 0)
				break;

			/* execute code */
			EXEC_NODE(NODE(exec), 39, &ret);

			/* check break */
			if (ret.v.ent == -1)
				break;

			/* implicit continue */
		}

		if (exec_free_stack(r, 2) != 0)
			return -1;

		EXEC_RETURN();

	} END_FUNCTION


/**********************************************************************
* 
* X_IF
*
**********************************************************************/
	EXEC_FUNCTION(X_IF) {
		/* -4 : n
		 * -3 : cond
		 * -2 : exec
		 * -1 : exec_else
		 */
		static const int retcode   = -4;
		static const int cond      = -3;
		static const int exec      = -2;
		static const int exec_else = -1;

		if (exec_reserve_stack(r, 3) != 0)
			return -1;

		// TODO: deux variables suffisent ... on n'exucute pas le true et le false en meme temps !
		NODE(cond)      = exec_get_children(NODE(retcode));
		NODE(exec)      = exec_get_brother(NODE(cond));
		NODE(exec_else) = exec_get_brother(NODE(exec));

		/* execute condition */
		EXEC_NODE(NODE(cond), 40, &ret);

		/* if true, execute "exec" */
		if (ret.v.ent != 0)
			EXEC_NODE(NODE(exec), 41, ARG(retcode));

		/* if false and else block is present */
		else if (&NODE(exec_else)->b != &NODE(retcode)->c)
			EXEC_NODE(NODE(exec_else), 42, ARG(retcode));

		/* if false */
		else
			ENT(retcode) = 0;

		if (exec_free_stack(r, 3) != 0)
			return -1;
		EXEC_RETURN();
	} END_FUNCTION

/* end point functions */

/**********************************************************************
* 
* X_VAR
*
**********************************************************************/
	EXEC_FUNCTION(X_VAR) {
		memcpy(ARG(-1), &r->vars[NODE(-1)->v.v.var->offset], sizeof(*ARG(-1)));
		EXEC_RETURN();
	} END_FUNCTION

/**********************************************************************
*
* X_INTEGER
*
**********************************************************************/
	EXEC_FUNCTION(X_INTEGER) {
		memcpy( ARG(-1), &NODE(-1)->v, sizeof(*ARG(-1)) );
		EXEC_RETURN();
	} END_FUNCTION

/**********************************************************************
* 
* X_STRING
*
**********************************************************************/
	EXEC_FUNCTION(X_STRING) {
		memcpy( ARG(-1), &NODE(-1)->v, sizeof(*ARG(-1)) );
		EXEC_RETURN();
	} END_FUNCTION

}
