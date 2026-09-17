/*
 * main.c — LARA compiler entry point (TAC or AST inspection)
 *
 * Exit code: 0 on success, 1 on lexical/syntax error.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"
#include "codegen.h"

extern ast_node_t *ast_root;      /* set by parser.y's start rule */

symtab_t *global_symtab = NULL;

extern int yyparse(void);
extern int yylex_destroy(void);
extern void main_walk(const ast_node_t *root);

int main(int argc, char *argv[])
{
    int show_ast = argc == 2 && strcmp(argv[1], "--ast") == 0;
    if (argc != 1 && !show_ast) {
        fprintf(stderr, "Uso: %s [--ast] < programa.lc\n", argv[0]);
        return 1;
    }
    int status = 1;

    global_symtab = symtab_new();
    if (global_symtab == NULL) {
        fprintf(stderr, "ERRO FATAL: não foi possível alocar a tabela de símbolos.\n");
        return 1;
    }

    int parse_result = yyparse();

    if (parse_result != 0) {
        fprintf(stderr, "[LARA] Análise FALHOU (código %d).\n", parse_result);
        goto cleanup;
    }

    if (show_ast) {
        printf("=== Árvore de Sintaxe Abstrata (AST) ===\n");
        ast_print(ast_root, 0, stdout);
        printf("=========================================\n\n");
        main_walk(ast_root);
        symtab_print(global_symtab, stdout);
    } else {
        codegen_ctx_t *ctx = codegen_new(global_symtab);
        if (!ctx) {
            fprintf(stderr, "ERRO FATAL: não foi possível alocar o gerador TAC.\n");
            goto cleanup;
        }
        codegen_program(ctx, ast_root);
        tac_print(ctx->code, stdout);
        tac_free(ctx->code);
        codegen_free(ctx);
    }
    status = 0;

cleanup:
    yylex_destroy();
    ast_free(ast_root);
    symtab_free(global_symtab);

    return status;
}
