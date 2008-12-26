%option reentrant
%option noyywrap
%option bison-bridge
%option yylineno

%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "templates.h"
#include "exec_internals.h"
#include "template.h"

#define seterror(fmt, args...) \
	do { \
		struct yyargs_t *arg = yyget_extra(yyscanner); \
		snprintf(arg->e->error, ERROR_LEN, fmt, ##args); \
		return 1; \
	} while(0);

%}

space           [ \t]+
function        [a-zA-Z][a-zA-Z0-9]*
var             $[a-zA-Z][a-zA-Z0-9]*
num             -?[0-9]+
hex             0x[0-9a-fA-F]+
str             \"[^\"]*\"
block           "%>"([^<]|<[^\%])"<%"

%%

"%>"([^<]|<[^%])*"<%" {
           	*yylval = exec_new(X_PRINT, exec_blockdup(yytext), yylineno);
           	return PRINT; 
           }

"{"        { return OPENBLOCK; }
"}"        { return CLOSEBLOCK; }

"("        {return OPENPAR; }
")"        { return CLOSEPAR; }

","        { return COMMA; }
";"        { return SEP; }
":"        { return COLON; }

"for"      { *yylval = exec_new(X_FOR,    NULL, yylineno); return FOR; }
"while"    { *yylval = exec_new(X_WHILE,  NULL, yylineno); return WHILE; }
"if"       { *yylval = exec_new(X_IF,     NULL, yylineno); return IF; }
"else"     { return ELSE; }
"break"    { *yylval = exec_new(X_BREAK,  NULL, yylineno); return BREAK; }
"continue" { *yylval = exec_new(X_CONT,   NULL, yylineno); return CONT; }
"switch"   { *yylval = exec_new(X_SWITCH, NULL, yylineno); return SWITCH; }
"case"     { return CASE; }
"default"  { return DEFAULT; }

"="        { *yylval = exec_new(X_ASSIGN, NULL, yylineno); return ASSIGN; }
"+"        { *yylval = exec_new(X_ADD,    NULL, yylineno); return ADD; }
"-"        { *yylval = exec_new(X_SUB,    NULL, yylineno); return SUB; }
"*"        { *yylval = exec_new(X_MUL,    NULL, yylineno); return MUL; }
"/"        { *yylval = exec_new(X_DIV,    NULL, yylineno); return DIV; }
"%"        { *yylval = exec_new(X_MOD,    NULL, yylineno); return MOD; }

"=="       { *yylval = exec_new(X_EQUAL,  NULL, yylineno); return EQUAL; }
"==="      { *yylval = exec_new(X_STREQ,  NULL, yylineno); return STREQ; }
"!="       { *yylval = exec_new(X_DIFF,   NULL, yylineno); return DIFF; }
"&&"       { *yylval = exec_new(X_AND,    NULL, yylineno); return AND; }
"||"       { *yylval = exec_new(X_OR,     NULL, yylineno); return OR; }
"<"        { *yylval = exec_new(X_LT,     NULL, yylineno); return LT; }
">"        { *yylval = exec_new(X_GT,     NULL, yylineno); return GT; }
">="       { *yylval = exec_new(X_GE,     NULL, yylineno); return GE; }
"<="       { *yylval = exec_new(X_LE,     NULL, yylineno); return LE; }

"display"  { *yylval = exec_new(X_DISPLAY,NULL, yylineno); return DISPLAY; }

{str}      {
		void *val;
		val = exec_strdup(yytext);
		*yylval = exec_new(X_STRING, val, yylineno);
		return STR;
	}
"NULL"     {
		*yylval = exec_new(X_INTEGER, 0, yylineno);
		return NUM;
	}
({num}|{hex}) {
		void *val;
		char *error;
		val = (void *)strtol(yytext, &error, 0);
		if (*error != '\0') {
			seterror("line %d: '%s' not a number", yylineno, yytext);
			return 0;
		}
		*yylval = exec_new(X_INTEGER, val, yylineno);
		return NUM;
	}
{var}      {
		void *val;
		struct yyargs_t *arg = yyget_extra(yyscanner);
		val = exec_var(arg->e, yytext);
		*yylval = exec_new(X_VAR, val, yylineno);
		return VAR;
	}
{function} {
		void *val;
		struct yyargs_t *arg = yyget_extra(yyscanner);
		val = exec_func(arg->e, yytext);
		if (val == NULL) {
			seterror("line %d: '%s' function not found", yylineno, yytext);
			return 0;
		}
		*yylval = exec_new(X_FUNCTION, val, yylineno);
		return FUNCTION;
	}

{space}    { }
"\n"       { }
.          {
		seterror("line %d: '%s' unexpected character", yylineno, yytext);
		return 0;
	}

%%

/* just for removing this warning:
syntax.c:1259: warning: 'yyunput' defined but not used
*/
static inline void ________stupid_function_not_called(void) {
	yyunput('c', NULL, 0);
}
