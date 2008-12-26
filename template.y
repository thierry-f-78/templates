%pure-parser
%parse-param { struct yyargs_t *args }
%lex-param { yyscan_t args_scanner }

%{

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "exec_internals.h"
#include "syntax.h"
#include "templates.h"

#define args_scanner args->scanner

#define stack_cur \
	(&(args->stack[args->stack_idx]))

#define stack_push() \
	do { \
		args->stack_idx++; \
		if (args->stack_idx == STACK_SIZE) { \
			yyerror(args, "stack_push: stack full: too much overlap"); \
			YYABORT; \
		} \
		INIT_LIST_HEAD(stack_cur); \
	} while(0);

#define stack_pop() \
	do { \
		args->stack_idx--; \
		if (args->stack_idx == -1) { \
			yyerror(args, "stack_pop: pop under index 0"); \
			YYABORT; \
		} \
	} while(0);

int yyerror(struct yyargs_t *args, char *str) { 
	snprintf(args->e->error, ERROR_LEN, "line %d: %s near '%s'",
	         yyget_lineno(args->scanner), str, yyget_text(args->scanner)); 
	return 0; 
}

%}

%token PRINT

%token OPENPAR
%token CLOSEPAR

%token OPENBLOCK
%token CLOSEBLOCK

// data const
%token VAR
%token STR
%token NUM

// fucntion
%token FUNCTION
%token DISPLAY

// statement
%token FOR
%token WHILE
%token IF
%token ELSE
%token BREAK
%token CONT
%token SWITCH
%token CASE
%token DEFAULT

%token SEP
%token COMMA
%token ASSIGN
%token COLON

// operators
%token ADD
%token SUB
%token MUL
%token DIV
%token MOD

// comparaisons
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
	PRINT               { list_add_tail(&$1->b, stack_cur); }
	| BREAK SEP         { list_add_tail(&$1->b, stack_cur); }
	| CONT SEP          { list_add_tail(&$1->b, stack_cur); }
	| Expression SEP    { list_add_tail(&$1->b, stack_cur); }
	| For               { list_add_tail(&$1->b, stack_cur); }
	| While             { list_add_tail(&$1->b, stack_cur); }
	| If                { list_add_tail(&$1->b, stack_cur); }
	| Switch            { list_add_tail(&$1->b, stack_cur); }
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
			n = exec_new(X_COLLEC, NULL, -1);
			list_replace(stack_cur, &n->c);
			stack_pop();
			$$ = n;
		}
	| OPENBLOCK { stack_push(); } Input CLOSEBLOCK
		{
			struct exec_node *n;
			n = exec_new(X_COLLEC, NULL, -1);
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
			n = exec_new(X_COLLEC, NULL, -1);
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
			n = exec_new(X_NULL, NULL, -1);
			list_add_tail(&n->b, stack_cur);
		}

Expression:
	RValue                    { $$ = $1; }
	| LValue Expression
		{
			list_add_tail(&$2->b, &$1->c);
			$$ = $1;
		}
	;

LValue:
	VAR ASSIGN
		{
			list_add_tail(&$1->b, &$2->c);
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

int exec_parse(struct exec *e, char *file) {
	struct exec_node *n;
	int ret;
	struct yyargs_t yyargs;
	struct yyargs_t *args = &yyargs;
	yyscan_t scanner;
	FILE *fd;

	/* open, parse file and close */
	fd = fopen(file, "r");
	if (fd == NULL) {
		ERRS("fopen(%s): %s\n", file, strerror(errno));
		exit(1);
	}

	/* init flex context */
	yylex_init(&scanner);
	yyset_extra(args, scanner);
	yyset_in(fd, scanner);

	/* init stack */
	args->stack_idx = 0;
	args->scanner = scanner;
	args->e = e;
	INIT_LIST_HEAD(stack_cur);
	e->error[0] = '\0';

	ret = yyparse(args);

	yylex_destroy(scanner);
	fclose(fd);

	/* syntax error */
	if (ret == 1)
		return -1;

	/* memory hexausted */
	else if (ret == 2) {
		snprintf(e->error, ERROR_LEN, "parsing failed due to memory exhaustion.");
		return -1;
	}

	/* final and first node, set it into template */
	n = exec_new(X_COLLEC, NULL, 0);
	list_replace(stack_cur, &n->c);
	n->line = 0;
	e->program = n;

	n->p = NULL;
	exec_set_parents(n);

	return 0;
}
