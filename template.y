%{

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "syntax.h"
#include "exec.h"

#define YYSTYPE struct exec_node *
#define STACK_SIZE 150

#ifdef DEBUGING
#	define DEBUG(fmt, args...) \
	       fprintf(stderr, "[%s:%s:%d] " fmt "\n", \
                  __FILE__, __FUNCTION__, __LINE__, ##args);
#endif

struct exec_node *base;
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
	fprintf (stderr, "\n%s en '%s'\n\n", str, yytext);
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

%token SEP
%token COMMA
%token ASSIGN

// operators
%token ADD
%token SUB
%token MUL
%token DIV
%token MOD

// comparaisons
%token EQUAL
%token LT
%token GT
%token LE
%token GE
%token AND
%token OR

%start Input

%%

Input:
	| Input For               { my_list_add_tail(&$2->b, stack_cur); }
	| Input While             { my_list_add_tail(&$2->b, stack_cur); }
	| Input If                { my_list_add_tail(&$2->b, stack_cur); }
	| Input Expr
	;

While:
	WHILE OPENPAR RValue CLOSEPAR OPENBLOCK { stack_push(); } Expressions CLOSEBLOCK
		{
			struct exec_node *n;
			n = exec_new(X_COLLEC, NULL);
			list_replace(stack_cur, &n->c);
			stack_pop();
			my_list_add_tail(&$3->b, &$1->c);
			my_list_add_tail(&n->b, &$1->c);
			$$ = $1;
		}
	;

If:
	IF OPENPAR RValue CLOSEPAR OPENBLOCK { stack_push(); } Expressions CLOSEBLOCK
		{
			struct exec_node *n;
			n = exec_new(X_COLLEC, NULL);
			list_replace(stack_cur, &n->c);
			stack_pop();
			my_list_add_tail(&$3->b, &$1->c);
			my_list_add_tail(&n->b, &$1->c);
			$$ = $1;
		}
	;

For:
	FOR OPENPAR Expression SEP RValue SEP Expression CLOSEPAR OPENBLOCK { stack_push(); } Expressions CLOSEBLOCK
		{
			struct exec_node *n;
			n = exec_new(X_COLLEC, NULL);
			list_replace(stack_cur, &n->c);
			stack_pop();
			my_list_add_tail(&$3->b, &$1->c);
			my_list_add_tail(&$5->b, &$1->c);
			my_list_add_tail(&$7->b, &$1->c);
			my_list_add_tail(&n->b, &$1->c);
			$$ = $1;
		}
	;

Expressions:
	Expr
	| Expressions Expr
	;

Expr:
	| PRINT                   { my_list_add_tail(&$1->b, stack_cur); }
	| Expression SEP          { my_list_add_tail(&$1->b, stack_cur); }
	;

Expression:
	RValue                    { $$ = $1; }
	| LValue Expression       {
			my_list_add_tail(&$2->b, &$1->c);
			$$ = $1;
		}
	;

LValue:
	VAR ASSIGN                {
			my_list_add_tail(&$1->b, &$2->c);
			$$ = $2;
		}
	;

RValue:
	Value                     { $$ = $1; }
	| RValue ADD RValue       {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue SUB RValue       {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue MUL RValue       {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
		}
	| RValue DIV RValue       {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue MOD RValue       {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue EQUAL RValue     {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue AND RValue        {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue OR RValue        {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue LT RValue        {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue GT RValue        {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue LE RValue        {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| RValue GE RValue        {
			my_list_add_tail(&$1->b, &$2->c);
			my_list_add_tail(&$3->b, &$2->c);
			$$ = $2;
		}
	| OPENPAR RValue CLOSEPAR { $$ = $2; }
	;

Value:
	| VAR      { DEBUG("> VAR  %p", $1); $$ = $1; }
	| NUM      { DEBUG("> NUM  %p", $1); $$ = $1; }
	| STR      { DEBUG("> STR  %p", $1); $$ = $1; }
	| Function { DEBUG("> FUNC %p", $1); $$ = $1; }
	| Display  { DEBUG("> DISP %p", $1); $$ = $1; }
	;

Display:
	DISPLAY OPENPAR VAR CLOSEPAR {
			my_list_add_tail(&$3->b, &$1->c);
			$$ = $1;
		}

Function:
	FUNCTION OPENPAR { stack_push(); } ArgsList {
			list_replace(stack_cur, &$1->c);
			stack_pop();
			$$ = $1;
		}
	;

ArgsList:
	CLOSEPAR
	| Args CLOSEPAR
	;

Args:
	RValue              { my_list_add_tail(&$1->b, stack_cur); }
	| Args COMMA RValue { my_list_add_tail(&$3->b, stack_cur); }
	;
%%

int main(void) {
	char *file = "test.html";
	struct exec_node *n;
	struct exec *e;

	//yydebug = 1;

	/* init stack */
	INIT_LIST_HEAD(stack_cur);

	/* init new template */
	exec_new_template();

	/* open, parse file and close */
	yyinputfd = open(file, O_RDONLY);
	if (yyinputfd < 0) {
		ERRS("open(%s): %s\n", file, strerror(errno));
		exit(1);
	}
	yyparse();
	close(yyinputfd);

	/* final and first node, set it into template */
	n = exec_new(X_COLLEC, NULL);
	list_replace(stack_cur, &n->c);
	exec_set_template(n);

	/* get exec */
	e = exec_get_template();

	exec_display(n);

	return 0;
}
