/*
 * main.c — LARA compiler entry point (Stage 1)
 *
 * Exit code: 0 on success, 1 on lexical/syntax error.
 */

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "symtab.h"

extern ast_node_t *ast_root;      /* set by parser.y's start rule */

symtab_t *global_symtab = NULL;

extern int yyparse(void);
extern void main_walk(const ast_node_t *root);

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    global_symtab = symtab_new();
    if (global_symtab == NULL) {
        fprintf(stderr, "ERRO FATAL: não foi possível alocar a tabela de símbolos.\n");
        return 1;
    }

    int parse_result = yyparse();

    if (parse_result != 0) {
        fprintf(stderr, "[ETAPA 1] Análise FALHOU (código %d).\n", parse_result);
        symtab_free(global_symtab);
        return 1;
    }

    fprintf(stderr, "[ETAPA 1] Análise CONCLUÍDA com SUCESSO.\n");

    printf("=== Árvore de Sintaxe Abstrata (AST) ===\n");
    ast_print(ast_root, 0, stdout);
    printf("=========================================\n\n");

    main_walk(ast_root);

    symtab_print(global_symtab, stdout);

    ast_free(ast_root);
    symtab_free(global_symtab);

    return 0;
}
