/*
 * scanner.l — LARA lexical analyzer
 *
 * Tokenizes stdin for the parser (yylex), registering identifiers in the
 * global symbol table as they're recognized.
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"        /* must precede parser.tab.h (YYSTYPE uses ast_node_t) */
#include "parser.tab.h"
#include "symtab.h"

extern symtab_t *global_symtab;

#define LEX_ERROR(msg) \
    fprintf(stderr, "[ERRO LÉXICO] linha %d: %s: '%s'\n", yylineno, msg, yytext)
%}

%option noyywrap
%option yylineno

DIGIT       [0-9]
LETTER      [a-zA-Z_]
ALNUM       [a-zA-Z0-9_]
ID          {LETTER}{ALNUM}*
INT         {DIGIT}+

/* TODO: FLOAT needs an optional scientific-notation exponent, e.g. 1.5e10 */
FLOAT       {DIGIT}*"."{DIGIT}+|{DIGIT}+"."{DIGIT}*

%%

[ \n]+      { /* TODO: also skip \t and \r */ }

"//".*      { }

"/*"        {
    int c, prev = 0;
    while ((c = input()) != EOF) {
        if (prev == '*' && c == '/') break;
        if (c == '\n') yylineno++;
        prev = c;
    }
    if (c == EOF)
        LEX_ERROR("comentário de bloco não fechado");
}

"int"       { return TK_PR_INT;    }
"float"     { return TK_PR_FLOAT;  }
"bool"      { return TK_PR_BOOL;   }
"char"      { return TK_PR_CHAR;   }
"if"        { return TK_PR_IF;     }
"else"      { return TK_PR_ELSE;   }
"while"     { return TK_PR_WHILE;  }
"for"       { return TK_PR_FOR;    }
"return"    { return TK_PR_RETURN; }
"void"      { return TK_PR_VOID;   }
"array"     { return TK_PR_ARRAY;  }
"of"        { return TK_PR_OF;     }
"let"       { return TK_PR_LET;    }
"fun"       { return TK_PR_FUN;    }
"do"        { return TK_PR_DO;     }
"print"     { return TK_PR_PRINT;  }
"read"      { return TK_PR_READ;   }
"true"      { return TK_LIT_TRUE;  }
"false"     { return TK_LIT_FALSE; }

{INT}       { yylval.sval = strdup(yytext); return TK_LIT_INT;    }
{FLOAT}     { yylval.sval = strdup(yytext); return TK_LIT_FLOAT;  }
'[^'\n]'    { yylval.sval = strdup(yytext); return TK_LIT_CHAR;   }
\"[^"\n]*\" { yylval.sval = strdup(yytext); return TK_LIT_STRING; }

{ID}        {
    symtab_insert(global_symtab, yytext, yylineno);
    yylval.sval = strdup(yytext);
    return TK_ID;
}

"<="    { return TK_OC_LE;     }
">="    { return TK_OC_GE;     }
"=="    { return TK_OC_EQ;     }
"!="    { return TK_OC_NE;     }
"&&"    { return TK_OC_AND;    }
"||"    { return TK_OC_OR;     }
"->"    { return TK_OC_ARROW;  }
":="    { return TK_OC_ASSIGN; }
"+="    { return TK_OC_PLUSEQ; }
"-="    { return TK_OC_MINUSEQ;}

[+\-*/%<>!=()\[\]{},;:.]   { return yytext[0]; }

.           {
    LEX_ERROR("caractere inválido");
    return TK_ERROR;
}

%%
