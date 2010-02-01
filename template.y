/*
 * Copyright (c) 2009 Thierry FOURNIER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License.
 *
 */

/* with this, bison make reentrant code */
%pure-parser

/* this is the reentrant parameter given at all functions */
%parse-param { struct yyargs_t *args }

/* this is the reentrant parameter given at reentrant yylex function */
%lex-param { yyscan_t args_scanner }

%{

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "exec_internals.h"
#include "syntax.h"
#include "templates.h"

extern char **environ;

/* this fucking bison parser does not understand
   "%lex-param { yyscan_t args->scanner }" */
#define args_scanner args->scanner

/* return the current "struct list_head" */
#define stack_cur \
	(&(args->stack[args->stack_idx]))

/* init new stack element */
#define stack_push() \
	do { \
		args->stack_idx++; \
		if (args->stack_idx == STACK_SIZE) { \
			yyerror(args, "stack_push: stack full: too much overlap"); \
			YYABORT; \
		} \
		INIT_LIST_HEAD(stack_cur); \
	} while(0);

/* pop the current element - this element id lost */
#define stack_pop() \
	do { \
		args->stack_idx--; \
		if (args->stack_idx == -1) { \
			yyerror(args, "stack_pop: pop under index 0"); \
			YYABORT; \
		} \
	} while(0);

/* buils error message */
int yyerror(struct yyargs_t *args, char *str) { 
	if (args->e->error[0] != '\0')
		return 0;
	snprintf(args->e->error, ERROR_LEN, "line %d: %s near '%s'",
	         yyget_lineno(args->scanner), str, yyget_text(args->scanner)); 
	return 0; 
}

/* this create new node, check return value and return error if needed */
#define my_exec_new_nul(_var, _action, _line) \
	do { \
		_var = exec_new(args->e, _action, _line); \
		if (_var == NULL) \
			YYABORT; \
	} while(0);

/* this create new str node, check return value and return error if needed */
#define my_exec_new_str(_var, _action, _argument, _len, _line) \
	do { \
		_var = exec_new_str(args->e, _action, _argument, _len, _line); \
		if (_var == NULL) \
			YYABORT; \
	} while(0);

/* this create new int node, check return value and return error if needed */
#define my_exec_new_ent(_var, _action, _value, _line) \
	do { \
		_var = exec_new_ent(args->e, _action, _value, _line); \
		if (_var == NULL) \
			YYABORT; \
	} while(0);

/* this create new ptr node, check return value and return error if needed */
#define my_exec_new_ptr(_var, _action, _value, _line) \
	do { \
		_var = exec_new_ptr(args->e, _action, _value, _line); \
		if (_var == NULL) \
			YYABORT; \
	} while(0);


%}

%token PRINT

/* data const */
%token VAR
%token STR
%token NUM

/* function */
%token FUNCTION
%token DISPLAY

/* statement */
%token FOR
%token WHILE
%token IF
%token ELSE
%token BREAK
%token CONT
%token SWITCH
%token CASE
%token DEFAULT

/* separators */
%token OPENPAR
%token CLOSEPAR
%token OPENBLOCK
%token CLOSEBLOCK
%token SEP
%token COMMA
%token ASSIGN
%token ASSIGNPP
%token ASSIGNMM
%token ASS_ADD
%token ASS_SUB
%token ASS_MUL
%token ASS_DIV
%token ASS_MOD
%token COLON
%token ENDTAG

/* operators */
%token ADD
%token SUB
%token MUL
%token DIV
%token MOD

/* comparators*/
%token EQUAL
%token STREQ
%token DIFF
%token LT
%token GT
%token LE
%token GE
%token AND
%token OR

%left ADD
%left SUB
%left MUL
%left DIV
%left MOD

%left EQUAL
%left STREQ
%left DIFF
%left LT
%left GT
%left LE
%left GE
%left AND
%left OR

%left COMMA

%start Input

%%

Input:
	| Input InputElement
	;

InputElement:
	ENDTAG
	| PRINT             { list_add_tail(&$1->b, stack_cur); }
	| BREAK SEP         { list_add_tail(&$1->b, stack_cur); }
	| CONT SEP          { list_add_tail(&$1->b, stack_cur); }
	| Expression SEP    { list_add_tail(&$1->b, stack_cur); }
	| For               { list_add_tail(&$1->b, stack_cur); }
	| While             { list_add_tail(&$1->b, stack_cur); }
	| If                { list_add_tail(&$1->b, stack_cur); }
	| Switch            { list_add_tail(&$1->b, stack_cur); }
	| FastDisplay       { list_add_tail(&$1->b, stack_cur); }
	;

While:
	WHILE OPENPAR RValue CLOSEPAR Block
		{
			list_add_tail(&$3->b, &$1->c);
			list_add_tail(&$5->b, &$1->c);
			$$ = $1;
		}
	;

If:
	IF OPENPAR RValue CLOSEPAR Block
		{
			list_add_tail(&$3->b, &$1->c);
			list_add_tail(&$5->b, &$1->c);
			$$ = $1;
		}
	| If ELSE Block
		{
			list_add_tail(&$3->b, &$1->c);
			$$ = $1;
		}

For:
	FOR OPENPAR Expression SEP RValue SEP Expression CLOSEPAR Block
		{
			list_add_tail(&$3->b, &$1->c);
			list_add_tail(&$5->b, &$1->c);
			list_add_tail(&$7->b, &$1->c);
			list_add_tail(&$9->b, &$1->c);
			$$ = $1;
		}
	;

Block:
	{ stack_push(); } InputElement
		{
			struct exec_node *n;
			n = exec_new(args->e, X_COLLEC, -1);
			if (n == NULL)
				YYABORT;
			list_replace(stack_cur, &n->c);
			stack_pop();
			$$ = n;
		}
	| OPENBLOCK { stack_push(); } Input CLOSEBLOCK
		{
			struct exec_node *n;
			n = exec_new(args->e, X_COLLEC, -1);
			if (n == NULL)
				YYABORT;
			list_replace(stack_cur, &n->c);
			stack_pop();
			$$ = n;
		}
	;

Switch:
	SWITCH OPENPAR
		{ stack_push(); }
	SwitchArgs 
		{ list_add_tail(&$4->b, stack_cur); }
	CLOSEPAR OPENBLOCK IntoSwitch CLOSEBLOCK
		{
			list_replace(stack_cur, &$1->c);
			stack_pop();
			$$ = $1;
		}
	;

IntoSwitch:
	SwitchBlock
	| IntoSwitch SwitchBlock
	;

SwitchBlock:
	SwitchKey { stack_push(); } Input
		{
			struct exec_node *n;
			n = exec_new(args->e, X_COLLEC, -1);
			if (n == NULL)
				YYABORT;
			list_replace(stack_cur, &n->c);
			stack_pop();
			list_add_tail(&n->b, stack_cur);
		}
	;

SwitchKey:
	CASE NUM COLON   { list_add_tail(&$2->b, stack_cur); }
	| DEFAULT COLON
		{
			struct exec_node *n;
			n = exec_new(args->e, X_NULL, -1);
			if (n == NULL)
				YYABORT;
			list_add_tail(&n->b, stack_cur);
		}

Expression:
	RValue                    { $$ = $1; }
	| LValue Expression
		{
			/* LValue:
			 * 
			 *      =
			 *    /   \
			 *  VAR   ope
			 *       /
			 *     VAR
			 */
			if ($1->c.next->next != &$1->c) {
				struct exec_node *ope;
				/* search ope */
				ope = container_of($1->c.next->next, struct exec_node, b);
				list_add_tail(&$2->b, &ope->c);
			}
			else
				list_add_tail(&$2->b, &$1->c);
			$$ = $1;
		}
	| LValuePpMm              { $$ = $1; }
	;

LValue:
	VAR ASSIGN
		{
			list_add_tail(&$1->b, &$2->c);
			$$ = $2;
		}
	| VAR ASS_ADD
		{
			struct exec_node *ope;
			struct exec_node *var;

			my_exec_new_nul(ope, X_ADD, $2->line);
			my_exec_new_ptr(var, X_VAR, $1->v.v.var, $2->line);

			list_add_tail(&var->b, &ope->c);

			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&ope->b, &$2->c);
			$$ = $2;
		}
	| VAR ASS_SUB
		{
			struct exec_node *ope;
			struct exec_node *var;

			my_exec_new_nul(ope, X_SUB, $2->line);
			my_exec_new_ptr(var, X_VAR, $1->v.v.var, $2->line);

			list_add_tail(&var->b, &ope->c);

			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&ope->b, &$2->c);
			$$ = $2;
		}
	| VAR ASS_MUL
		{
			struct exec_node *ope;
			struct exec_node *var;

			my_exec_new_nul(ope, X_MUL, $2->line);
			my_exec_new_ptr(var, X_VAR, $1->v.v.var, $2->line);

			list_add_tail(&var->b, &ope->c);

			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&ope->b, &$2->c);
			$$ = $2;
		}
	| VAR ASS_DIV
		{
			struct exec_node *ope;
			struct exec_node *var;

			my_exec_new_nul(ope, X_DIV, $2->line);
			my_exec_new_ptr(var, X_VAR, $1->v.v.var, $2->line);

			list_add_tail(&var->b, &ope->c);

			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&ope->b, &$2->c);
			$$ = $2;
		}
	| VAR ASS_MOD
		{
			struct exec_node *ope;
			struct exec_node *var;

			my_exec_new_nul(ope, X_MOD, $2->line);
			my_exec_new_ptr(var, X_VAR, $1->v.v.var, $2->line);

			list_add_tail(&var->b, &ope->c);

			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&ope->b, &$2->c);
			$$ = $2;
		}
	;

LValuePpMm:
	VAR ASSIGNPP
		{
			struct exec_node *plus;
			struct exec_node *un;
			struct exec_node *var;

			my_exec_new_nul(plus, X_ADD, $2->line);
			my_exec_new_ent(un,   X_INTEGER, 1, $2->line);
			my_exec_new_ptr(var,  X_VAR, $1->v.v.var, $2->line);

			list_add_tail(&var->b, &plus->c);
			list_add_tail(&un->b,  &plus->c);

			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&plus->b, &$2->c);
			$$ = $2;
		}
	| VAR ASSIGNMM
		{
			struct exec_node *moins;
			struct exec_node *un;
			struct exec_node *var;

			my_exec_new_nul(moins, X_SUB, $2->line);
			my_exec_new_ent(un,    X_INTEGER, 1, $2->line);
			my_exec_new_ptr(var,   X_VAR, $1->v.v.var, $2->line);

			list_add_tail(&var->b, &moins->c);
			list_add_tail(&un->b,  &moins->c);

			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&moins->b, &$2->c);
			$$ = $2;
		}
	;

RValue:
	Value                     { $$ = $1; }
	| RValue ADD RValue 
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue SUB RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue MUL RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
		}
	| RValue DIV RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue MOD RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue EQUAL RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue STREQ RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue DIFF RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue AND RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue OR RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue LT RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue GT RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue LE RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue GE RValue
		{
			list_add_tail(&$1->b, &$2->c);
			list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| OPENPAR RValue CLOSEPAR { $$ = $2; }
	;

Value:
	VAR        { $$ = $1; }
	| NUM      { $$ = $1; }
	| STR      { $$ = $1; }
	| Function { $$ = $1; }
	| Display  { $$ = $1; }
	;

FastDisplay:
	ASSIGN DisplayArg ENDTAG
		{
			list_add_tail(&$2->b, &$1->c);
			$1->type = X_DISPLAY;
			$$ = $1;
		}
	;
		
Display:
	DISPLAY OPENPAR DisplayArg CLOSEPAR
		{
			list_add_tail(&$3->b, &$1->c);
			$$ = $1;
		}
	;

SwitchArgs:
	VAR        { $$ = $1; }
	| Function { $$ = $1; }
	;

DisplayArg:
	VAR        { $$ = $1; }
	| Function { $$ = $1; }
	| STR      { $$ = $1; }
	;

Function:
	FUNCTION OPENPAR { stack_push(); } ArgsList CLOSEPAR
		{
			list_replace(stack_cur, &$1->c);
			stack_pop();
			$$ = $1;
		}
	;

ArgsList:
	| Args
	;

Args:
	RValue              { list_add_tail(&$1->b, stack_cur); }
	| Args COMMA Args
	;
%%

int exec_parse_file(struct exec *e, FILE *fd) {
	struct exec_node *n;
	int ret;
	struct yyargs_t yyargs;
	yyscan_t scanner;
	int pip[2];
	int retcode;
	char *args[4];

	/* if preprocessor is set */
	if (e->preprocessor != NULL) {

		/* build pipe */
		retcode = pipe(pip);
		if (retcode != 0) {
			snprintf(e->error, ERROR_LEN, "pipe: %s", strerror(errno));
			return -1;
		}

		/* fork */
		retcode = fork();
		if (retcode == -1) {
			snprintf(e->error, ERROR_LEN, "fork: %s", strerror(errno));
			return -1;
		}

		/* children */
		if (retcode == 0) {
			close(0);
			close(1);
			dup2(fileno(fd), 0); /* use the stream file ptr as input */
			dup2(pip[1], 1); /* use pipe for writing */
			close(pip[1]);
			close(fileno(fd));
			args[0] = "/bin/sh";
			args[1] = "-c";
			args[2] = e->preprocessor;
			args[3] = NULL;
			retcode = execve("/bin/sh", args, environ);
			if (retcode != 0) {
				fprintf(stderr, "execve(%s): %s\n", e->preprocessor, strerror(errno));
				exit(1);
			}
			exit(0);
		}

		/* use pipe as input */
		close(fileno(fd));
		close(pip[1]);
		fd = fdopen(pip[0], "r");
		if (fd == NULL) {
			snprintf(e->error, ERROR_LEN, "fdopen: %s", strerror(errno));
			return -1;
		}
	}

	/* init flex context */
	yylex_init(&scanner);
	yyset_extra(&yyargs, scanner);
	yyset_in(fd, scanner);

	/* init stack and general context */
	yyargs.stack_idx = 0;
	INIT_LIST_HEAD(&(yyargs.stack[yyargs.stack_idx]));
	yyargs.scanner = scanner;
	yyargs.e = e;
	yyargs.print = NULL;
	yyargs.printsize = 0;
	yyargs.printblock = 0;
	e->error[0] = '\0';

	/* execute parsing */
	ret = yyparse(&yyargs);

	/* destroy data */
	yylex_destroy(scanner);

	/* wait for preprocessor */
	if (e->preprocessor != NULL) {
		waitpid(retcode, NULL, 0);
	}

	/* syntax error */
	if (ret == 1)
		return -1;

	/* memory hexausted */
	else if (ret == 2) {
		snprintf(e->error, ERROR_LEN, "parsing failed due to memory exhaustion.");
		return -1;
	}

	/* attach tree to the first node, set it into struct exec*/
	n = exec_new(e, X_COLLEC, -1);
	if (n == NULL)
		return -1;
	n->p = NULL;
	list_replace(&(yyargs.stack[yyargs.stack_idx]), &n->c);
	e->program = n;

	/* this recurssive function update parent pointeur into the tree */
	exec_set_parents(n);

	return 0;
}

int exec_parse(struct exec *e, char *file) {
	FILE *fd;
	int ret;

#ifdef DEBUG
	fprintf(stderr, "[%s:%s:%d] parse new template [%s]\n",
	        __FILE__, __FUNCTION__, __LINE__, file);
#endif

	/* open file */
	fd = fopen(file, "r");
	if (fd == NULL) {
		snprintf(e->error, ERROR_LEN, "fopen(%s): %s\n", file, strerror(errno));
		return -1;
	}

	/* exec parse */
	ret = exec_parse_file(e, fd);

	/* close */
	fclose(fd);

	return ret;
}
