%option noyywrap

%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "exec.h"
#include "exec_internals.h"
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

#ifdef DEBUGING
static char str[128];

const char *n2str(int code) {
	/* pour inserer vaec vim: ":r !./extract.sh" */
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
	case 271: return "ELSE";
	case 272: return "BREAK";
	case 273: return "CONT";
	case 274: return "SWITCH";
	case 275: return "CASE";
	case 276: return "DEFAULT";
	case 277: return "SEP";
	case 278: return "COMMA";
	case 279: return "ASSIGN";
	case 280: return "COLON";
	case 281: return "ADD";
	case 282: return "SUB";
	case 283: return "MUL";
	case 284: return "DIV";
	case 285: return "MOD";
	case 286: return "EQUAL";
	case 287: return "DIFF";
	case 288: return "LT";
	case 289: return "GT";
	case 290: return "LE";
	case 291: return "GE";
	case 292: return "AND";
	case 293: return "OR";
	default:
		snprintf(str, 128, "Unknown code <%d>", code);
		return str;
	}
}

#  define DEBUG(fmt, args...) \
          fprintf(stderr, "\e[34m[%s:%s:%d] " fmt "\e[0m\n", \
			         __FILE__, __FUNCTION__, __LINE__, ##    args);
#  define DEBUGI(fmt, args...) \
          fprintf(stderr, "\e[31m[%s:%s:%d] " fmt "\e[0m\n", \
			         __FILE__, __FUNCTION__, __LINE__, ##    args);
#	define RETURN(xxx) \
	       DEBUG("return %s (%d)", n2str(xxx), xxx); \
	       return xxx;

#	define D \
	       do { DEBUGI("***[%s]***", yytext); } while (0); 

#	define A \
	       do { DEBUGI("***[%s]***", yytext); } while (0); 

#	undef A
#	define A

#else

#	define RETURN(xxx) return xxx;
#	define A
#	define D

#endif

extern int yyerror(char *str);
extern int yyinputfd;

#define YY_INPUT(buf,result,max_size) \
	do { \
		result = read(yyinputfd, (buf), (max_size)); \
		if ((result) == 0) \
			(result) = YY_NULL; \
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
           	char *p;
           	D;
           	for (p=yytext; *p!='\0'; p++)
           		if (*p == '\n') 
           			yylineno++;
           	yylval = exec_new(X_PRINT, exec_blockdup(yytext), yylineno);
           	RETURN(PRINT); 
           }

"{"        {D; RETURN(OPENBLOCK); }
"}"        {D; RETURN(CLOSEBLOCK); }

"("        {D; RETURN(OPENPAR); }
")"        {D; RETURN(CLOSEPAR); }

","        {D; RETURN(COMMA); }
";"        {D; RETURN(SEP); }
":"        {D; RETURN(COLON); }

"for"      {D; yylval = exec_new(X_FOR,    NULL, yylineno); RETURN(FOR); }
"while"    {D; yylval = exec_new(X_WHILE,  NULL, yylineno); RETURN(WHILE); }
"if"       {D; yylval = exec_new(X_IF,     NULL, yylineno); RETURN(IF); }
"else"     {D; RETURN(ELSE); }
"break"    {D; yylval = exec_new(X_BREAK,  NULL, yylineno); RETURN(BREAK); }
"continue" {D; yylval = exec_new(X_CONT,   NULL, yylineno); RETURN(CONT); }
"switch"   {D; yylval = exec_new(X_SWITCH, NULL, yylineno); RETURN(SWITCH); }
"case"     {D; RETURN(CASE); }
"default"  {D; RETURN(DEFAULT); }

"="        {D; yylval = exec_new(X_ASSIGN, NULL, yylineno); RETURN(ASSIGN); }
"+"        {D; yylval = exec_new(X_ADD,    NULL, yylineno); RETURN(ADD); }
"-"        {D; yylval = exec_new(X_SUB,    NULL, yylineno); RETURN(SUB); }
"*"        {D; yylval = exec_new(X_MUL,    NULL, yylineno); RETURN(MUL); }
"/"        {D; yylval = exec_new(X_DIV,    NULL, yylineno); RETURN(DIV); }
"%"        {D; yylval = exec_new(X_MOD,    NULL, yylineno); RETURN(MOD); }

"=="       {D; yylval = exec_new(X_EQUAL,  NULL, yylineno); RETURN(EQUAL); }
"!="       {D; yylval = exec_new(X_DIFF,   NULL, yylineno); RETURN(DIFF); }
"&&"       {D; yylval = exec_new(X_AND,    NULL, yylineno); RETURN(AND); }
"||"       {D; yylval = exec_new(X_OR,     NULL, yylineno); RETURN(OR); }
"<"        {D; yylval = exec_new(X_LT,     NULL, yylineno); RETURN(LT); }
">"        {D; yylval = exec_new(X_GT,     NULL, yylineno); RETURN(GT); }
">="       {D; yylval = exec_new(X_GE,     NULL, yylineno); RETURN(GE); }
"<="       {D; yylval = exec_new(X_LE,     NULL, yylineno); RETURN(LE); }

"display"  {D; yylval = exec_new(X_DISPLAY,NULL, yylineno); RETURN(DISPLAY); }

{str}      {
		void *val;
		D;
		val = exec_strdup(yytext);
		yylval = exec_new(X_STRING, val, yylineno);
		RETURN(STR);
	}
({num}|{hex}) {
		void *val;
		char *error;
		D;
		val = (void *)strtol(yytext, &error, 0);
		if (*error != '\0')
			yyerror("Not a number");
		yylval = exec_new(X_INTEGER, val, yylineno);
		RETURN(NUM);
	}
{var}      {
		void *val;
		D;
		val = exec_var(yytext);
		yylval = exec_new(X_VAR, val, yylineno);
		RETURN(VAR);
	}
{function} {
		void *val;
		D;
		val = exec_func(yytext);
		if (val == NULL)
			yyerror("Function not found");
		yylval = exec_new(X_FUNCTION, val, yylineno);
		RETURN(FUNCTION);
	}

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
