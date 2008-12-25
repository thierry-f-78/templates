%{

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "syntax.h"
#include "exec.h"
#include "exec_internals.h"

#define STACK_SIZE 150

#ifdef DEBUGING
#	define DEBUG(fmt, args...) \
	       fprintf(stderr, "\e[33m[%s:%s:%d] " fmt "\e[0m\n", \
                  __FILE__, __FUNCTION__, __LINE__, ##args);
#else
#	define DEBUG(fmt, args...)
#endif

int yyinputfd;
struct list_head stack[STACK_SIZE];
int stack_idx = 0;

#define stack_cur (&stack[stack_idx])

#define stack_push() \
	do { \
		stack_idx++; \
		DEBUG("stack_push: %d", stack_idx); \
		if (stack_idx == STACK_SIZE) { \
			fprintf(stderr, "stack full\n"); \
			exit(1); \
		} \
		INIT_LIST_HEAD(&stack[stack_idx]); \
	} while(0);

#define stack_pop() \
	do { \
		stack_idx--; \
		DEBUG("stack_pop: %d", stack_idx); \
		if (stack_idx == -1) { \
			ERRS("stack_pop: negative index"); \
			exit(1); \
		} \
	} while(0);

#define my_list_add_tail(new, head) \
	DEBUG("my_list_add_tail(%p, %p)", new, head); \
	list_add_tail(new, head);

int yyerror(char *str) {
	fprintf (stderr, "\nline %d: %s en '%s'\n\n", yylineno, str, yytext);
	exit(1);
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
	PRINT               { my_list_add_tail(&$1->b, stack_cur); }
	| BREAK SEP         { my_list_add_tail(&$1->b, stack_cur); }
	| CONT SEP          { my_list_add_tail(&$1->b, stack_cur); }
	| Expression SEP    { my_list_add_tail(&$1->b, stack_cur); }
	| For               { my_list_add_tail(&$1->b, stack_cur); }
	| While             { my_list_add_tail(&$1->b, stack_cur); }
	| If                { my_list_add_tail(&$1->b, stack_cur); }
	| Switch            { my_list_add_tail(&$1->b, stack_cur); }
	;

While:
	WHILE OPENPAR RValue CLOSEPAR Block
		{
			my_list_add_tail(&$3->b, &$1->c);
			my_list_add_tail(&$5->b, &$1->c);
			$$ = $1;
		}
	;

If:
	IF OPENPAR RValue CLOSEPAR Block
		{
			my_list_add_tail(&$3->b, &$1->c);
			my_list_add_tail(&$5->b, &$1->c);
			$$ = $1;
		}
	| If ELSE Block
		{
			my_list_add_tail(&$3->b, &$1->c);
			$$ = $1;
		}

For:
	FOR OPENPAR Expression SEP RValue SEP Expression CLOSEPAR Block
		{
			my_list_add_tail(&$3->b, &$1->c);
			my_list_add_tail(&$5->b, &$1->c);
			my_list_add_tail(&$7->b, &$1->c);
			my_list_add_tail(&$9->b, &$1->c);
			$$ = $1;
		}
	;

Block:
	{ stack_push(); } InputElement
		{
			struct exec_node *n;
			n = exec_new(X_COLLEC, NULL, yylineno);
			list_replace(stack_cur, &n->c);
			stack_pop();
			$$ = n;
		}
	| OPENBLOCK { stack_push(); } Input CLOSEBLOCK
		{
			struct exec_node *n;
			n = exec_new(X_COLLEC, NULL, yylineno);
			list_replace(stack_cur, &n->c);
			stack_pop();
			$$ = n;
		}
	;

Switch:
	SWITCH OPENPAR
		{ stack_push(); }
	SwitchArgs 
		{ my_list_add_tail(&$4->b, stack_cur); }
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
			n = exec_new(X_COLLEC, NULL, yylineno);
			list_replace(stack_cur, &n->c);
			stack_pop();
			my_list_add_tail(&n->b, stack_cur);
		}
	;

SwitchKey:
	CASE NUM COLON   { my_list_add_tail(&$2->b, stack_cur); }
	| DEFAULT COLON
		{
			struct exec_node *n;
			n = exec_new(X_NULL, NULL, yylineno);
			my_list_add_tail(&n->b, stack_cur);
		}

Expression:
	RValue                    { $$ = $1; }
	| LValue Expression
		{
			my_list_add_tail(&$2->b, &$1->c);
			$$ = $1;
		}
	;

LValue:
	VAR ASSIGN
		{
			my_list_add_tail(&$1->b, &$2->c);
			$$ = $2;
		}
	;

RValue:
	Value                     { $$ = $1; }
	| RValue ADD RValue 
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue SUB RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue MUL RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
		}
	| RValue DIV RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue MOD RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue EQUAL RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue DIFF RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue AND RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue OR RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue LT RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue GT RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue LE RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue GE RValue
		{
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| OPENPAR RValue CLOSEPAR { $$ = $2; }
	;

Value:
	VAR        { DEBUG("> VAR  %p", $1); $$ = $1; }
	| NUM      { DEBUG("> NUM  %p", $1); $$ = $1; }
	| STR      { DEBUG("> STR  %p", $1); $$ = $1; }
	| Function { DEBUG("> FUNC %p", $1); $$ = $1; }
	| Display  { DEBUG("> DISP %p", $1); $$ = $1; }
	;

Display:
	DISPLAY OPENPAR DisplayArg CLOSEPAR
		{
			my_list_add_tail(&$3->b, &$1->c);
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
	RValue              { my_list_add_tail(&$1->b, stack_cur); }
	| Args COMMA Args
	;
%%

void exec_parse(struct exec *e, char *file) {
	struct exec_node *n;

#ifdef DEBUGING
	yydebug = 1;
#endif

	exec_template = e;

	/* init stack */
	INIT_LIST_HEAD(stack_cur);

	/* open, parse file and close */
	yyinputfd = open(file, O_RDONLY);
	if (yyinputfd < 0) {
		ERRS("open(%s): %s\n", file, strerror(errno));
		exit(1);
	}
	yyparse();
	close(yyinputfd);

	/* final and first node, set it into template */
	n = exec_new(X_COLLEC, NULL, yylineno);
	list_replace(stack_cur, &n->c);
	n->line = 0;
	e->program = n;

	n->p = NULL;
	exec_set_parents(n);
}
