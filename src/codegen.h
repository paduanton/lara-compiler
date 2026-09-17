/*
 * codegen.h — AST to TAC generation interface
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symtab.h"
#include "tac.h"

typedef struct {
    tac_instr_t  *code;
    symtab_t     *symtab;
    char         *current_func;
    int           local_offset;
} codegen_ctx_t;

codegen_ctx_t *codegen_new(symtab_t *symtab);

/* The caller frees ctx->code separately with tac_free(). */
void codegen_free(codegen_ctx_t *ctx);

void codegen_program(codegen_ctx_t *ctx, ast_node_t *program);

void codegen_fun(codegen_ctx_t *ctx, ast_node_t *fun_decl);

void codegen_stmt(codegen_ctx_t *ctx, ast_node_t *stmt);

/* Returns an owned address string; the caller must free it after emission. */
char *codegen_expr(codegen_ctx_t *ctx, ast_node_t *expr);

void codegen_emit(codegen_ctx_t *ctx, tac_op_t op,
                  const char *result, const char *arg1, const char *arg2);

#endif
