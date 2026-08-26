/*
 * parser.y — LARA grammar (bison, LALR(1))
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"

extern int   yylex(void);
extern int   yylineno;
extern char *yytext;

extern symtab_t *global_symtab;   /* defined in main.c */

ast_node_t *ast_root = NULL;      /* set by the `program` rule below */

void yyerror(const char *msg)
{
    fprintf(stderr, "[ERRO SINTÁTICO] linha %d: %s (token: '%s')\n",
            yylineno, msg, yytext);
}
%}

%union {
    char        *sval;   /* lexeme: ID, LIT_INT, LIT_FLOAT, ... */
    ast_node_t  *node;
    int          ival;
}

/* keep in sync with tokens.h */
%token TK_PR_INT TK_PR_FLOAT TK_PR_BOOL TK_PR_CHAR
%token TK_PR_IF TK_PR_ELSE TK_PR_WHILE TK_PR_FOR
%token TK_PR_RETURN TK_PR_VOID TK_PR_ARRAY TK_PR_OF
%token TK_PR_LET TK_PR_FUN TK_PR_DO TK_PR_PRINT TK_PR_READ
%token TK_LIT_TRUE TK_LIT_FALSE
%token TK_OC_LE TK_OC_GE TK_OC_EQ TK_OC_NE
%token TK_OC_AND TK_OC_OR TK_OC_ARROW TK_OC_ASSIGN
%token TK_OC_PLUSEQ TK_OC_MINUSEQ
%token TK_ERROR

%token <sval> TK_ID
%token <sval> TK_LIT_INT
%token <sval> TK_LIT_FLOAT
%token <sval> TK_LIT_CHAR
%token <sval> TK_LIT_STRING

%type <node> program
%type <node> toplevel_list toplevel_decl
%type <node> fun_decl param_list param_list_ne param
%type <node> var_decl array_decl
%type <node> type_spec
%type <node> block stmt_list stmt simple_stmt compound_stmt
%type <node> assign_stmt if_stmt while_stmt for_stmt
%type <node> return_stmt print_stmt read_stmt call_stmt
%type <node> expr expr_list expr_list_ne
%type <node> lvalue literal

/* lowest to highest precedence */
%right TK_OC_ASSIGN TK_OC_PLUSEQ TK_OC_MINUSEQ
%left  TK_OC_OR
%left  TK_OC_AND
%left  TK_OC_EQ TK_OC_NE
%left  '<' '>' TK_OC_LE TK_OC_GE
%left  '+' '-'
%left  '*' '/' '%'
%right '!' UMINUS
%left  '[' TK_OC_ARROW

%start program

%%

program
    : toplevel_list
        {
            $$ = ast_new(AST_PROGRAM, NULL, 1);
            $$->children[0] = $1;
            ast_root = $$;
        }
    | /* empty program */
        {
            $$ = ast_new(AST_PROGRAM, NULL, 1);
            ast_root = $$;
        }
    ;

toplevel_list
    : toplevel_decl
        { $$ = $1; }
    | toplevel_list toplevel_decl
        { $$ = ast_append($1, $2); }
    ;

toplevel_decl
    : fun_decl    { $$ = $1; }
    | var_decl    { $$ = $1; }
    | array_decl  { $$ = $1; }
    ;

fun_decl
    : TK_PR_FUN type_spec TK_ID '(' param_list ')' block
        {
            $$ = ast_new(AST_FUN_DECL, $3, yylineno);
            $$->children[0] = $2;   /* return type */
            $$->children[1] = $5;   /* param list, may be NULL */
            $$->children[2] = $7;   /* body */
            free($3);
        }
    ;

param_list
    : /* empty */       { $$ = NULL; }
    | param_list_ne     { $$ = $1;   }
    ;

param_list_ne
    : param                         { $$ = $1; }
    | param_list_ne ',' param       { $$ = ast_append($1, $3); }
    ;

param
    : type_spec TK_ID
        {
            $$ = ast_new(AST_PARAM, $2, yylineno);
            $$->children[0] = $1;
            free($2);
        }
    ;

var_decl
    : type_spec TK_ID ';'
        {
            $$ = ast_new(AST_VAR_DECL, $2, yylineno);
            $$->children[0] = $1;
            free($2);
        }
    ;

array_decl
    : TK_PR_ARRAY TK_LIT_INT TK_PR_OF type_spec TK_ID ';'
        {
            $$ = ast_new(AST_ARRAY_DECL, $5, yylineno);
            $$->children[0] = ast_new(AST_LIT_INT, $2, yylineno);  /* size */
            $$->children[1] = $4;                                    /* element type */
            free($2);
            free($5);
        }
    ;

type_spec
    : TK_PR_INT    { $$ = ast_new(AST_SYMBOL, "int",   yylineno); }
    | TK_PR_FLOAT  { $$ = ast_new(AST_SYMBOL, "float", yylineno); }
    | TK_PR_BOOL   { $$ = ast_new(AST_SYMBOL, "bool",  yylineno); }
    | TK_PR_CHAR   { $$ = ast_new(AST_SYMBOL, "char",  yylineno); }
    | TK_PR_VOID   { $$ = ast_new(AST_SYMBOL, "void",  yylineno); }
    ;

block
    : '{' stmt_list '}'
        {
            $$ = ast_new(AST_BLOCK, NULL, yylineno);
            $$->children[0] = $2;
        }
    | '{' '}'
        {
            $$ = ast_new(AST_BLOCK, NULL, yylineno);
        }
    ;

stmt_list
    : stmt
        { $$ = $1; }
    | stmt_list stmt
        { $$ = ast_append($1, $2); }
    ;

stmt
    : simple_stmt ';' { $$ = $1; }
    | compound_stmt   { $$ = $1; }
    ;

simple_stmt
    : assign_stmt  { $$ = $1; }
    | return_stmt  { $$ = $1; }
    | print_stmt   { $$ = $1; }
    | read_stmt    { $$ = $1; }
    | call_stmt    { $$ = $1; }
    | var_local    { $$ = NULL; }
    ;

compound_stmt
    : if_stmt    { $$ = $1; }
    | while_stmt { $$ = $1; }
    | for_stmt   { $$ = $1; }
    ;

var_local
    : TK_PR_LET TK_ID TK_OC_ASSIGN expr
        {
            /* no AST_VAR_DECL node yet at this stage — the identifier is
             * already tracked via the scanner's symtab_insert */
            symtab_insert(global_symtab, $2, yylineno);
            ast_free($4);
            free($2);
        }
    ;

assign_stmt
    : lvalue TK_OC_ASSIGN expr
        {
            $$ = ast_new(AST_ASSIGN, ":=", yylineno);
            $$->children[0] = $1;
            $$->children[1] = $3;
        }
    | lvalue TK_OC_PLUSEQ expr
        {
            $$ = ast_new(AST_ASSIGN, "+=", yylineno);
            $$->children[0] = $1;
            $$->children[1] = $3;
        }
    | lvalue TK_OC_MINUSEQ expr
        {
            $$ = ast_new(AST_ASSIGN, "-=", yylineno);
            $$->children[0] = $1;
            $$->children[1] = $3;
        }
    ;

if_stmt
    : TK_PR_IF '(' expr ')' block
        {
            $$ = ast_new(AST_IF, NULL, yylineno);
            $$->children[0] = $3;
            $$->children[1] = $5;
        }
    | TK_PR_IF '(' expr ')' block TK_PR_ELSE block
        {
            $$ = ast_new(AST_IF, NULL, yylineno);
            $$->children[0] = $3;
            $$->children[1] = $5;
            $$->children[2] = $7;
        }
    ;

while_stmt
    : TK_PR_WHILE '(' expr ')' TK_PR_DO block
        {
            $$ = ast_new(AST_WHILE, NULL, yylineno);
            $$->children[0] = $3;
            $$->children[1] = $6;
        }
    ;

for_stmt
    : TK_PR_FOR '(' assign_stmt ';' expr ';' assign_stmt ')' block
        {
            $$ = ast_new(AST_FOR, NULL, yylineno);
            $$->children[0] = $3;  /* init */
            $$->children[1] = $5;  /* cond */
            $$->children[2] = $7;  /* step */
            $$->children[3] = $9;  /* body */
        }
    ;

return_stmt
    : TK_PR_RETURN expr
        {
            $$ = ast_new(AST_RETURN, NULL, yylineno);
            $$->children[0] = $2;
        }
    ;

print_stmt
    : TK_PR_PRINT expr
        {
            $$ = ast_new(AST_PRINT, NULL, yylineno);
            $$->children[0] = $2;
        }
    ;

read_stmt
    : TK_PR_READ lvalue
        {
            $$ = ast_new(AST_READ, NULL, yylineno);
            $$->children[0] = $2;
        }
    ;

call_stmt
    : TK_ID '(' expr_list ')'
        {
            $$ = ast_new(AST_CALL, $1, yylineno);
            $$->children[0] = $3;
            free($1);
        }
    ;

expr
    : expr '+' expr  { $$ = ast_new(AST_EXPR_BINARY, "+",  yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr '-' expr  { $$ = ast_new(AST_EXPR_BINARY, "-",  yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr '*' expr  { $$ = ast_new(AST_EXPR_BINARY, "*",  yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr '/' expr  { $$ = ast_new(AST_EXPR_BINARY, "/",  yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr '%' expr  { $$ = ast_new(AST_EXPR_BINARY, "%",  yylineno); $$->children[0]=$1; $$->children[1]=$3; }

    | expr '<' expr         { $$ = ast_new(AST_EXPR_BINARY, "<",  yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr '>' expr         { $$ = ast_new(AST_EXPR_BINARY, ">",  yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr TK_OC_LE expr    { $$ = ast_new(AST_EXPR_BINARY, "<=", yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr TK_OC_GE expr    { $$ = ast_new(AST_EXPR_BINARY, ">=", yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr TK_OC_EQ expr    { $$ = ast_new(AST_EXPR_BINARY, "==", yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr TK_OC_NE expr    { $$ = ast_new(AST_EXPR_BINARY, "!=", yylineno); $$->children[0]=$1; $$->children[1]=$3; }

    | expr TK_OC_AND expr   { $$ = ast_new(AST_EXPR_BINARY, "&&", yylineno); $$->children[0]=$1; $$->children[1]=$3; }
    | expr TK_OC_OR expr    { $$ = ast_new(AST_EXPR_BINARY, "||", yylineno); $$->children[0]=$1; $$->children[1]=$3; }

    | '!' expr
        {
            $$ = ast_new(AST_EXPR_UNARY, "!", yylineno);
            $$->children[0] = $2;
        }

    | '-' expr %prec UMINUS
        {
            $$ = ast_new(AST_EXPR_UNARY, "-", yylineno);
            $$->children[0] = $2;
        }

    | '(' expr ')'  { $$ = $2; }

    | TK_ID '(' expr_list ')'
        {
            $$ = ast_new(AST_EXPR_CALL, $1, yylineno);
            $$->children[0] = $3;
            free($1);
        }

    | TK_ID '[' expr ']'
        {
            $$ = ast_new(AST_EXPR_INDEX, $1, yylineno);
            $$->children[0] = $3;
            free($1);
        }

    | TK_ID TK_OC_ARROW TK_ID
        {
            $$ = ast_new(AST_EXPR_ARROW, $1, yylineno);
            $$->children[0] = ast_new(AST_SYMBOL, $3, yylineno);
            free($1);
            free($3);
        }

    | TK_ID
        {
            $$ = ast_new(AST_SYMBOL, $1, yylineno);
            free($1);
        }

    | literal { $$ = $1; }
    ;

expr_list
    : /* empty */    { $$ = NULL; }
    | expr_list_ne   { $$ = $1;   }
    ;

expr_list_ne
    : expr                         { $$ = $1; }
    | expr_list_ne ',' expr        { $$ = ast_append($1, $3); }
    ;

lvalue
    : TK_ID
        {
            $$ = ast_new(AST_SYMBOL, $1, yylineno);
            free($1);
        }
    | TK_ID '[' expr ']'
        {
            $$ = ast_new(AST_EXPR_INDEX, $1, yylineno);
            $$->children[0] = $3;
            free($1);
        }
    | TK_ID TK_OC_ARROW TK_ID
        {
            $$ = ast_new(AST_EXPR_ARROW, $1, yylineno);
            $$->children[0] = ast_new(AST_SYMBOL, $3, yylineno);
            free($1);
            free($3);
        }
    ;

literal
    : TK_LIT_INT    { $$ = ast_new(AST_LIT_INT,    $1, yylineno); free($1); }
    | TK_LIT_FLOAT  { $$ = ast_new(AST_LIT_FLOAT,  $1, yylineno); free($1); }
    | TK_LIT_CHAR   { $$ = ast_new(AST_LIT_CHAR,   $1, yylineno); free($1); }
    | TK_LIT_STRING { $$ = ast_new(AST_LIT_STRING,  $1, yylineno); free($1); }
    | TK_LIT_TRUE   { $$ = ast_new(AST_LIT_BOOL,   "true",  yylineno); }
    | TK_LIT_FALSE  { $$ = ast_new(AST_LIT_BOOL,   "false", yylineno); }
    ;

%%
