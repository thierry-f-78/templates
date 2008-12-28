/* with this, flex make reentrant code */
%option reentrant

/* the function yywrap is not used */
%option noyywrap

/* build code for using bison with reentrant flex */
%option bison-bridge

/* flex count lines */
%option yylineno

%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "templates.h"
#include "exec_internals.h"
#include "template.h"

/* this return abort yyparse */
#define YYPARSE_ABORT 1

/* this macro set error message into struct exec */
#define seterror(fmt, args...) \
	do { \
		struct yyargs_t *arg = yyget_extra(yyscanner); \
		snprintf(arg->e->error, ERROR_LEN, fmt, ##args); \
		return YYPARSE_ABORT; \
	} while(0);

/* this create nex node, check return value and return error if needed */
#define my_exec_new(action, argument) \
	do { \
		struct yyargs_t *arg = yyget_extra(yyscanner); \
		*yylval = exec_new(arg->e, action, argument, yylineno); \
		if (*yylval == NULL) \
			return YYPARSE_ABORT; \
	} while(0);

%}

space           [ \t]+
function        [a-zA-Z][a-zA-Z0-9_]*
var             $[a-zA-Z][a-zA-Z0-9]*
num             -?[0-9]+
hex             0x[0-9a-fA-F]+
str             \"[^\"]*\"
block           "%>"([^<]|<[^\%])"<%"

%x COMMENT

%%

<INITIAL>{

"%>"([^<]|<[^%])*"<%" {
		struct yyargs_t *extra = yyget_extra(yyscanner);
		char *blockdup = exec_blockdup(extra->e, yytext);
		if (blockdup == NULL)
			return YYPARSE_ABORT;
		my_exec_new(X_PRINT, blockdup);
		return PRINT; 
	}

"/*"       BEGIN(COMMENT);

"{"        { return OPENBLOCK; }
"}"        { return CLOSEBLOCK; }

"("        { return OPENPAR; }
")"        { return CLOSEPAR; }

","        { return COMMA; }
";"        { return SEP; }
":"        { return COLON; }

"for"      { my_exec_new(X_FOR, NULL); return FOR; }
"while"    { my_exec_new(X_WHILE, NULL); return WHILE; }
"if"       { my_exec_new(X_IF, NULL); return IF; }
"else"     { return ELSE; }
"break"    { my_exec_new(X_BREAK, NULL); return BREAK; }
"continue" { my_exec_new(X_CONT, NULL); return CONT; }
"switch"   { my_exec_new(X_SWITCH, NULL); return SWITCH; }
"case"     { return CASE; }
"default"  { return DEFAULT; }

"="        { my_exec_new(X_ASSIGN, NULL); return ASSIGN; }
"+"        { my_exec_new(X_ADD, NULL); return ADD; }
"-"        { my_exec_new(X_SUB, NULL); return SUB; }
"*"        { my_exec_new(X_MUL, NULL); return MUL; }
"/"        { my_exec_new(X_DIV, NULL); return DIV; }
"%"        { my_exec_new(X_MOD, NULL); return MOD; }

"=="       { my_exec_new(X_EQUAL, NULL); return EQUAL; }
"==="      { my_exec_new(X_STREQ, NULL); return STREQ; }
"!="       { my_exec_new(X_DIFF, NULL); return DIFF; }
"&&"       { my_exec_new(X_AND, NULL); return AND; }
"||"       { my_exec_new(X_OR, NULL); return OR; }
"<"        { my_exec_new(X_LT, NULL); return LT; }
">"        { my_exec_new(X_GT, NULL); return GT; }
">="       { my_exec_new(X_GE, NULL); return GE; }
"<="       { my_exec_new(X_LE, NULL); return LE; }

"display"  { my_exec_new(X_DISPLAY, NULL); return DISPLAY; }

{str}      {
		char *val;
		struct yyargs_t *extra = yyget_extra(yyscanner);
		val = exec_strdup(extra->e, yytext);
		if (val == NULL)
			return YYPARSE_ABORT;
		my_exec_new(X_STRING, val);
		return STR;
	}
"NULL"     {
		my_exec_new(X_INTEGER, 0);
		return NUM;
	}
({num}|{hex}) {
		long val;
		char *error;
		val = strtol(yytext, &error, 0);
		if (*error != '\0') {
			seterror("line %d: '%s' not a number", yylineno, yytext);
			return YYPARSE_ABORT;
		}
		my_exec_new(X_INTEGER, (void *)val);
		return NUM;
	}
{var}      {
		struct exec_vars *val;
		struct yyargs_t *arg = yyget_extra(yyscanner);
		val = exec_var(arg->e, yytext);
		if (val == NULL)
			return YYPARSE_ABORT;
		my_exec_new(X_VAR, val);
		return VAR;
	}
{function} {
		struct exec_funcs *val;
		struct yyargs_t *arg = yyget_extra(yyscanner);
		val = exec_func(arg->e, yytext);
		if (val == NULL) {
			seterror("line %d: '%s' function not found", yylineno, yytext);
			return YYPARSE_ABORT;
		}
		my_exec_new(X_FUNCTION, val);
		return FUNCTION;
	}

{space}    { }
"\n"       { }
.          {
		seterror("line %d: '%s' unexpected character", yylineno, yytext);
		return YYPARSE_ABORT;
	}
}

<COMMENT>{
"*/"       BEGIN(INITIAL);
.          { }
\n         { }
}
%%

/* just for removing this warning:
syntax.c:1259: warning: 'yyunput' defined but not used
*/
static inline void ________stupid_function_never_called(void) {
	yyunput('c', NULL, 0);
}
