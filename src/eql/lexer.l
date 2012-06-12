%{
#include <stdlib.h>
#include "../bstring.h"
#include "ast.h"
#include "array.h"
#include "parser.h"
#define SAVE_STRING yylval->string = bfromcstr(yytext)
#define SAVE_INT yylval->int_value = atoll(yytext)
#define SAVE_FLOAT yylval->float_value = atof(yytext)
#define INIT_STRING yylval->string = bfromcstr("")
#define APPEND_STRING_AS(STR) bcatcstr(yylval->string, STR);
#define APPEND_STRING bcatcstr(yylval->string, yytext);
#define TOKEN(t) (yylval->token = t)
%}

%option noyywrap
%option reentrant
%option bison-bridge

%x COMMENT
%x ML_COMMENT
%x STRING

%%

<INITIAL>"//"           BEGIN(COMMENT);
<COMMENT>"\n"           BEGIN(INITIAL); yylineno++;
<COMMENT>[^\n]+

<INITIAL>"/*"           BEGIN(ML_COMMENT);
<ML_COMMENT>"*/"        BEGIN(INITIAL);
<ML_COMMENT>[^*\n]+
<ML_COMMENT>\n          yylineno++;

<INITIAL>"\""           BEGIN(STRING); INIT_STRING;
<STRING>"\""            BEGIN(INITIAL); return TSTRING;
<STRING>"\\\""          APPEND_STRING_AS("\"");
<STRING>[^"\\]+         APPEND_STRING;
<STRING>.               printf("Invalid string!\n"); yyterminate();

"class"                 return TOKEN(TCLASS);
"private"               return TOKEN(TPRIVATE);
"public"                return TOKEN(TPUBLIC);
"return"                return TOKEN(TRETURN);
[ \t\n]                 ;
[a-zA-Z_][a-zA-Z0-9_]*  SAVE_STRING; return TIDENTIFIER;
[0-9]+\.[0-9]+          SAVE_FLOAT; return TFLOAT;
[0-9]+                  SAVE_INT; return TINT;
"("                     return TOKEN(TLPAREN);
")"                     return TOKEN(TRPAREN);
"{"                     return TOKEN(TLBRACE);
"}"                     return TOKEN(TRBRACE);
"["                     return TOKEN(TLBRACKET);
"]"                     return TOKEN(TRBRACKET);
"'"                     return TOKEN(TQUOTE);
"+"                     return TOKEN(TPLUS);
"-"                     return TOKEN(TMINUS);
"*"                     return TOKEN(TMUL);
"/"                     return TOKEN(TDIV);
";"                     return TOKEN(TSEMICOLON);
":"                     return TOKEN(TCOLON);
","                     return TOKEN(TCOMMA);
"="                     return TOKEN(TEQUALS);
.                       printf("Unknown token!\n"); yyterminate();

%%