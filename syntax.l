/* with this, flex make reentrant code */
%option reentrant

/* the function yywrap is not used */
%option noyywrap

/* build code for using bison with reentrant flex */
%option bison-bridge

/* flex count lines */
%option yylineno

/* use stack for recording context parsing */
%option stack

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

/* this create new node, check return value and return error if needed */
#define my_exec_new(action) \
	do { \
		struct yyargs_t *arg = yyget_extra(yyscanner); \
		*yylval = exec_new(arg->e, action, yylineno); \
		if (*yylval == NULL) \
			return YYPARSE_ABORT; \
	} while(0);

/* this create new str node, check return value and return error if needed */
#define my_exec_new_str(action, argument, len) \
	do { \
		struct yyargs_t *arg = yyget_extra(yyscanner); \
		*yylval = exec_new_str(arg->e, action, argument, len, yylineno); \
		if (*yylval == NULL) \
			return YYPARSE_ABORT; \
	} while(0);

/* this create new int node, check return value and return error if needed */
#define my_exec_new_ent(action, value) \
	do { \
		struct yyargs_t *arg = yyget_extra(yyscanner); \
		*yylval = exec_new_ent(arg->e, action, value, yylineno); \
		if (*yylval == NULL) \
			return YYPARSE_ABORT; \
	} while(0);

/* this create new ptr node, check return value and return error if needed */
#define my_exec_new_ptr(action, value) \
	do { \
		struct yyargs_t *arg = yyget_extra(yyscanner); \
		*yylval = exec_new_ptr(arg->e, action, value, yylineno); \
		if (*yylval == NULL) \
			return YYPARSE_ABORT; \
	} while(0);

/* this add char into current print space */
#define ADDCHARBLOCK 1024
#define add_char(__cc) \
	do { \
		/* new block ? */ \
		if (extra->printsize == extra->printblock) { \
			char *tmp; \
			extra->printblock += ADDCHARBLOCK; \
			tmp = realloc(extra->print, extra->printblock); \
			if (tmp == NULL) { \
				free(extra->print); \
				extra->print = NULL; \
				seterror("realloc(%p, %d): %s", extra->print, \
				         extra->printblock, strerror(errno)); \
				return YYPARSE_ABORT; \
			} \
			extra->print = tmp; \
		} \
		extra->print[extra->printsize] = __cc; \
		extra->printsize++; \
	} while(0);

/* this get extra variable */
#define get_extra struct yyargs_t *extra = yyget_extra(yyscanner)

/* this convert hexa char into decimal */
#define hex_conv(__cc, __dest) \
	do { \
		__dest = __cc; \
		if (__dest >= 'a' && __dest <= 'f') __dest = __dest - 'a' + 0xa; \
		else if (__dest >= 'A' && __dest <= 'F') __dest = __dest - 'A' + 0xa; \
		else if (__dest >= '0' && __dest <= '9') __dest = __dest - '0'; \
		else { \
			seterror("'%c' is not hexa", __dest); \
			return YYPARSE_ABORT; \
		} \
	} while (0);

%}

function        [a-zA-Z][a-zA-Z0-9_]*
var             $[a-zA-Z][a-zA-Z0-9]*
num             -?[0-9]+
hex             0x[0-9a-fA-F]+
soctal          \\[0-7][0-7][0-7]
shex            \\x[0-9a-fA-F][0-9a-fA-F] 

%x COMMENT LANGUAGE STRING

%%

<INITIAL>{

	<<EOF>>    {
			get_extra;
			if (extra->printsize != 0) {
				char *blockdup;
				add_char('\0');
				blockdup = exec_blockdup(extra->e, extra->print, extra->printsize);
				if (blockdup == NULL)
					return YYPARSE_ABORT;
				my_exec_new_str(X_PRINT, blockdup, extra->printsize);
				extra->printsize = 0;
				yy_push_state(LANGUAGE, yyscanner);
				return PRINT; 
			}
	}

	"<%"       { 
			get_extra;
			yy_push_state(LANGUAGE, yyscanner);
			if (extra->printsize != 0) {
				char *blockdup;
				add_char('\0');
				blockdup = exec_blockdup(extra->e, extra->print, extra->printsize);
				if (blockdup == NULL)
					return YYPARSE_ABORT;
				my_exec_new_str(X_PRINT, blockdup, extra->printsize);
				extra->printsize = 0;
				return PRINT; 
			}
		} BEGIN(LANGUAGE);

	(\n|.)     {
			get_extra;
			add_char(yytext[0]);
		}

}

<LANGUAGE>{

	"/*"       { yy_push_state(COMMENT, yyscanner);      } BEGIN(COMMENT);
	"%>"       { yy_pop_state(yyscanner); return ENDTAG; } BEGIN(INITIAL);
	"\""       { yy_push_state(STRING, yyscanner);       } BEGIN(STRING);

	"{"        { return OPENBLOCK; }
	"}"        { return CLOSEBLOCK; }
	
	"("        { return OPENPAR; }
	")"        { return CLOSEPAR; }
	
	","        { return COMMA; }
	";"        { return SEP; }
	":"        { return COLON; }
	
	"for"      { my_exec_new(X_FOR); return FOR; }
	"while"    { my_exec_new(X_WHILE); return WHILE; }
	"if"       { my_exec_new(X_IF); return IF; }
	"else"     { return ELSE; }
	"break"    { my_exec_new(X_BREAK); return BREAK; }
	"continue" { my_exec_new(X_CONT); return CONT; }
	"switch"   { my_exec_new(X_SWITCH); return SWITCH; }
	"case"     { return CASE; }
	"default"  { return DEFAULT; }
	
	"="        { my_exec_new(X_ASSIGN); return ASSIGN; }
	"++"       { my_exec_new(X_ASSIGN); return ASSIGNPP; }
	"--"       { my_exec_new(X_ASSIGN); return ASSIGNMM; }

	"+"        { my_exec_new(X_ADD); return ADD; }
	"-"        { my_exec_new(X_SUB); return SUB; }
	"*"        { my_exec_new(X_MUL); return MUL; }
	"/"        { my_exec_new(X_DIV); return DIV; }
	"%"        { my_exec_new(X_MOD); return MOD; }
	
	"=="       { my_exec_new(X_EQUAL); return EQUAL; }
	"==="      { my_exec_new(X_STREQ); return STREQ; }
	"!="       { my_exec_new(X_DIFF); return DIFF; }
	"&&"       { my_exec_new(X_AND); return AND; }
	"||"       { my_exec_new(X_OR); return OR; }
	"<"        { my_exec_new(X_LT); return LT; }
	">"        { my_exec_new(X_GT); return GT; }
	">="       { my_exec_new(X_GE); return GE; }
	"<="       { my_exec_new(X_LE); return LE; }
	
	"display"  { my_exec_new(X_DISPLAY); return DISPLAY; }
	
	"NULL"     {
			my_exec_new_ent(X_INTEGER, 0);
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
			my_exec_new_ent(X_INTEGER, val);
			return NUM;
		}

	{var}      {
			struct exec_vars *val;
			struct yyargs_t *arg = yyget_extra(yyscanner);
			val = exec_var(arg->e, yytext);
			if (val == NULL)
				return YYPARSE_ABORT;
			my_exec_new_ptr(X_VAR, val);
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
			my_exec_new_ptr(X_FUNCTION, val);
			return FUNCTION;
		}
	
	(" "|"\t"|"\n"|"\r") { }

	.          {
			seterror("line %d: '%s' unexpected character", yylineno, yytext);
			return YYPARSE_ABORT;
		}
}

<STRING>{

	{soctal}   {
			get_extra;
			add_char(
				((yytext[1] - '0') * 64 ) +
				((yytext[2] - '0') * 8  ) +
				((yytext[3] - '0') * 1  )
			);
		}

	{shex}     {
			char a, b;
			get_extra;
			hex_conv(yytext[2], a);
			hex_conv(yytext[3], b);
			add_char(a * 16 + b );
		}

	"\\n"      { get_extra; add_char('\n'); }
	"\\t"      { get_extra; add_char('\t'); }
	"\\e"      { get_extra; add_char('\e'); }
	"\\r"      { get_extra; add_char('\r'); }
	"\\\""     { get_extra; add_char('"');  }
	"\\\\"     { get_extra; add_char('\\'); }
	"\\a"      { get_extra; add_char('\a'); }
	"\\b"      { get_extra; add_char('\b'); }
	"\\v"      { get_extra; add_char('\v'); }
	"\\f"      { get_extra; add_char('\f'); }
	"\\'"      { get_extra; add_char('\''); }
	"\\?"      { get_extra; add_char('\?'); }
	"\\0"      { get_extra; add_char('\0'); }

	"\""       {
			get_extra;
			char *blockdup;
			yy_pop_state(yyscanner);
			add_char('\0');
			blockdup = exec_blockdup(extra->e, extra->print, extra->printsize);
			if (blockdup == NULL)
				return YYPARSE_ABORT;
			my_exec_new_str(X_STRING, blockdup, extra->printsize);
			extra->printsize = 0;
			return STR; 
		} BEGIN(LANGUAGE);

	.          { 
			get_extra;
			add_char(yytext[0]);
		}
}

<COMMENT>{
	"*/"       { yy_pop_state(yyscanner); } BEGIN(LANGUAGE);
	.          { }
	\n         { }
}
%%

/* just for removing this warning:
syntax.c:1259: warning: 'yyunput' defined but not used
*/
static inline void ________stupid_function_never_called(void) {
	yyunput('c', NULL, 0);
	yy_top_state(NULL);
}
