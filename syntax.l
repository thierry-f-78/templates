%option noyywrap

%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "exec.h"

#define YYSTYPE struct exec_node *
#include "template.h"

#ifdef DEBUGING
#  define DEBUG(fmt, args...) \
          fprintf(stderr, "[%s:%s:%d] " fmt "\n", \
			         __FILE__, __FUNCTION__, __LINE__, ##    args);
#endif

extern int yyerror(char *str);
extern int yyinputfd;

#define YY_INPUT(buf,result,max_size) \
	do { \
		result = read(yyinputfd, (buf), (max_size)); \
		if ((result) == 0) \
			(result) = YY_NULL; \
	} while(0);

#define D \
	do { DEBUG("***[%s]***", yytext); } while (0); 

#define A \
	do { DEBUG("***[%s]***", yytext); } while (0); 

#undef A
#define A

%}

space           [ \t]+
function        [a-zA-Z][a-zA-Z0-9]*
var             $[a-zA-Z][a-zA-Z0-9]*
num             [0-9]+
str             \"[^\"]*\"
block           "%>"([^<]|<[^\%])"<%"

%%

"%>"([^<]|<[^%])*"<%" {A; yylval = exec_new(X_PRINT, exec_blockdup(yytext)); return PRINT; }

"{"        {D; return OPENBLOCK; }
"}"        {D; return CLOSEBLOCK; }

"("        {D; return OPENPAR; }
")"        {D; return CLOSEPAR; }

","        {D; return COMMA; }
";"        {D; return SEP; }

"for"      {D; yylval = exec_new(X_FOR,    NULL); return FOR; }
"while"    {D; yylval = exec_new(X_WHILE,  NULL); return WHILE; }
"if"       {D; yylval = exec_new(X_IF,     NULL); return IF; }

"="        {D; yylval = exec_new(X_ASSIGN, NULL); return ASSIGN; }
"+"        {D; yylval = exec_new(X_ADD,    NULL); return ADD; }
"-"        {D; yylval = exec_new(X_SUB,    NULL); return SUB; }
"*"        {D; yylval = exec_new(X_MUL,    NULL); return MUL; }
"/"        {D; yylval = exec_new(X_DIV,    NULL); return DIV; }
"%"        {D; yylval = exec_new(X_MOD,    NULL); return MOD; }

"=="       {D; yylval = exec_new(X_EQUAL,  NULL); return EQUAL; }
"&&"       {D; yylval = exec_new(X_AND,    NULL); return AND; }
"||"       {D; yylval = exec_new(X_OR,     NULL); return OR; }
"<"        {D; yylval = exec_new(X_LT,     NULL); return LT; }
">"        {D; yylval = exec_new(X_GT,     NULL); return GT; }
">="       {D; yylval = exec_new(X_GE,     NULL); return GE; }
"<="       {D; yylval = exec_new(X_LE,     NULL); return LE; }

{str}      {D; yylval = exec_new(X_STRING,   exec_strdup(yytext));  return STR; }
{num}      {D; yylval = exec_new(X_INTEGER,  (void *)atoi(yytext)); return NUM; }
{var}      {D; yylval = exec_new(X_VAR,      exec_var(yytext));     return VAR; }
"display"  {D; yylval = exec_new(X_DISPLAY,  NULL);                 return DISPLAY; }
{function} {D; yylval = exec_new(X_FUNCTION, exec_func(yytext));    return FUNCTION; }

{space}    {A; }
.          {D; yyerror("unexpected character"); }
"\n"       {A; yylineno++; }

%%

/* just for removing this warning:
syntax.c:1259: warning: 'yyunput' defined but not used
*/
static inline void ________stupid_function_not_called(void) {
	unput('c');
}
