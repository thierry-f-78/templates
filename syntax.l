%option noyywrap

%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "exec.h"

#define YYSTYPE struct exec_node *
#include "template.h"

/*
 [0m			reset; clears all colors and styles (to white on black)
 [30m		set foreground color to black
 [31m		set foreground color to red
 [32m		set foreground color to green
 [33m		set foreground color to yellow
 [34m		set foreground color to blue
 [35m		set foreground color to magenta (purple)
 [36m		set foreground color to cyan
 [37m		set foreground color to white
 [39m		set foreground color to default (white)
 [40m		set background color to black
 [41m		set background color to red
 [42m		set background color to green
 [43m		set background color to yellow
 [44m		set background color to blue
 [45m		set background color to magenta (purple)
 [46m		set background color to cyan
 [47m		set background color to white
 [49m		set background color to default (black)

\e[0m

*/

static char str[128];

const char *n2str(int code) {
	switch (code) {
	case 258: return "PRINT";
	case 259: return "OPENPAR";
	case 260: return "CLOSEPAR";
	case 261: return "OPENBLOCK";
	case 262: return "CLOSEBLOCK";
	case 263: return "VAR";
	case 264: return "STR";
	case 265: return "NUM";
	case 266: return "FUNCTION";
	case 267: return "DISPLAY";
	case 268: return "FOR";
	case 269: return "WHILE";
	case 270: return "IF";
	case 271: return "SEP";
	case 272: return "COMMA";
	case 273: return "ASSIGN";
	case 274: return "ADD";
	case 275: return "SUB";
	case 276: return "MUL";
	case 277: return "DIV";
	case 278: return "MOD";
	case 279: return "EQUAL";
	case 280: return "LT";
	case 281: return "GT";
	case 282: return "LE";
	case 283: return "GE";
	case 284: return "AND";
	case 285: return "OR";
	default:
		snprintf(str, 128, "Unknown code <%d>", code);
		return str;
	}
}

#ifdef DEBUGING
#  define DEBUG(fmt, args...) \
          fprintf(stderr, "\e[34m[%s:%s:%d] " fmt "\e[0m\n", \
			         __FILE__, __FUNCTION__, __LINE__, ##    args);
#  define DEBUGI(fmt, args...) \
          fprintf(stderr, "\e[31m[%s:%s:%d] " fmt "\e[0m\n", \
			         __FILE__, __FUNCTION__, __LINE__, ##    args);
#	define RETURN(xxx) \
	       DEBUG("return %s (%d)", n2str(xxx), xxx); \
	       return xxx;
#else

#	define RETURN(xxx) return xxx;

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
	do { DEBUGI("***[%s]***", yytext); } while (0); 

#define A \
	do { DEBUGI("***[%s]***", yytext); } while (0); 

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

"%>"([^<]|<[^%])*"<%" {D; yylval = exec_new(X_PRINT, exec_blockdup(yytext)); RETURN(PRINT); }

"{"        {D; RETURN(OPENBLOCK); }
"}"        {D; RETURN(CLOSEBLOCK); }

"("        {D; RETURN(OPENPAR); }
")"        {D; RETURN(CLOSEPAR); }

","        {D; RETURN(COMMA); }
";"        {D; RETURN(SEP); }

"for"      {D; yylval = exec_new(X_FOR,    NULL); RETURN(FOR); }
"while"    {D; yylval = exec_new(X_WHILE,  NULL); RETURN(WHILE); }
"if"       {D; yylval = exec_new(X_IF,     NULL); RETURN(IF); }
"else"     {D; RETURN(ELSE); }
"break"    {D; yylval = exec_new(X_BREAK,  NULL); RETURN(BREAK); }
"continue" {D; yylval = exec_new(X_CONT,   NULL); RETURN(CONT); }

"="        {D; yylval = exec_new(X_ASSIGN, NULL); RETURN(ASSIGN); }
"+"        {D; yylval = exec_new(X_ADD,    NULL); RETURN(ADD); }
"-"        {D; yylval = exec_new(X_SUB,    NULL); RETURN(SUB); }
"*"        {D; yylval = exec_new(X_MUL,    NULL); RETURN(MUL); }
"/"        {D; yylval = exec_new(X_DIV,    NULL); RETURN(DIV); }
"%"        {D; yylval = exec_new(X_MOD,    NULL); RETURN(MOD); }

"=="       {D; yylval = exec_new(X_EQUAL,  NULL); RETURN(EQUAL); }
"!="       {D; yylval = exec_new(X_DIFF,   NULL); RETURN(DIFF); }
"&&"       {D; yylval = exec_new(X_AND,    NULL); RETURN(AND); }
"||"       {D; yylval = exec_new(X_OR,     NULL); RETURN(OR); }
"<"        {D; yylval = exec_new(X_LT,     NULL); RETURN(LT); }
">"        {D; yylval = exec_new(X_GT,     NULL); RETURN(GT); }
">="       {D; yylval = exec_new(X_GE,     NULL); RETURN(GE); }
"<="       {D; yylval = exec_new(X_LE,     NULL); RETURN(LE); }

{str}      {D; yylval = exec_new(X_STRING,   exec_strdup(yytext));  RETURN(STR); }
{num}      {D; yylval = exec_new(X_INTEGER,  (void *)atoi(yytext)); RETURN(NUM); }
{var}      {D; yylval = exec_new(X_VAR,      exec_var(yytext));     RETURN(VAR); }
"display"  {D; yylval = exec_new(X_DISPLAY,  NULL);                 RETURN(DISPLAY); }
{function} {D; yylval = exec_new(X_FUNCTION, exec_func(yytext));    RETURN(FUNCTION); }

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
