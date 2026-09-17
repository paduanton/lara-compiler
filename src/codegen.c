/*
 * codegen.c — AST to TAC generation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

static int type_size(sym_datatype_t dt)
{
    switch (dt) {
        case SYM_TYPE_INT:   return 4;
        case SYM_TYPE_FLOAT: return 8;
        case SYM_TYPE_CHAR:  return 1;
        case SYM_TYPE_BOOL:  return 1;
        default:             return 4;
    }
}

static tac_op_t op_to_tac(const char *op)
{
    if (!op) return TAC_NOP;
    if (strcmp(op, "+")  == 0) return TAC_ADD;
    if (strcmp(op, "-")  == 0) return TAC_SUB;
    if (strcmp(op, "*")  == 0) return TAC_MUL;
    if (strcmp(op, "/")  == 0) return TAC_DIV;
    if (strcmp(op, "%")  == 0) return TAC_MOD;
    if (strcmp(op, "<")  == 0) return TAC_LT;
    if (strcmp(op, ">")  == 0) return TAC_GT;
    if (strcmp(op, "<=") == 0) return TAC_LE;
    if (strcmp(op, ">=") == 0) return TAC_GE;
    if (strcmp(op, "==") == 0) return TAC_EQ;
    if (strcmp(op, "!=") == 0) return TAC_NE;
    if (strcmp(op, "&&") == 0) return TAC_AND;
    if (strcmp(op, "||") == 0) return TAC_OR;
    return TAC_NOP;
}

codegen_ctx_t *codegen_new(symtab_t *symtab)
{
    codegen_ctx_t *ctx = calloc(1, sizeof(codegen_ctx_t));
    if (!ctx) return NULL;
    ctx->symtab        = symtab;
    ctx->code          = NULL;
    ctx->current_func  = NULL;
    ctx->local_offset  = 0;
    return ctx;
}

void codegen_free(codegen_ctx_t *ctx)
{
    if (!ctx) return;
    free(ctx->current_func);
    free(ctx);
}

void codegen_emit(codegen_ctx_t *ctx, tac_op_t op,
                  const char *result, const char *arg1, const char *arg2)
{
    tac_instr_t *i = tac_new(op, result, arg1, arg2);
    ctx->code = tac_append(ctx->code, i);
}

void codegen_program(codegen_ctx_t *ctx, ast_node_t *program)
{
    if (!program) return;

    ast_node_t *decl = program->children[0];
    int global_offset = 0;

    while (decl) {
        if (decl->type == AST_FUN_DECL) {
            codegen_fun(ctx, decl);
        } else if (decl->type == AST_VAR_DECL || decl->type == AST_ARRAY_DECL) {
            sym_entry_t *entry = symtab_lookup(ctx->symtab, decl->value);
            if (entry) {
                int size = type_size(entry->datatype);
                if (decl->type == AST_ARRAY_DECL)
                    size *= entry->array_size;
                entry->scope = SYM_SCOPE_GLOBAL;
                entry->offset = global_offset;
                global_offset += size;
                char size_str[16];
                snprintf(size_str, sizeof(size_str), "%d", size);
                codegen_emit(ctx, TAC_DECL_GLOBAL, decl->value, size_str, NULL);
            }
        }

        decl = decl->next;
    }
}

void codegen_fun(codegen_ctx_t *ctx, ast_node_t *fun_decl)
{
    if (!fun_decl || fun_decl->type != AST_FUN_DECL) return;

    const char *fname = fun_decl->value;

    free(ctx->current_func);
    ctx->current_func = strdup(fname);
    ctx->local_offset = 0;

    codegen_emit(ctx, TAC_BEGINFUNC, fname, NULL, NULL);

    ast_node_t *param = fun_decl->children[1];
    int param_offset = -4;
    while (param) {
        if (param->type == AST_PARAM && param->value) {
            sym_entry_t *e = symtab_lookup(ctx->symtab, param->value);
            if (e) {
                e->scope  = SYM_SCOPE_LOCAL;
                e->offset = param_offset;
                param_offset -= type_size(e->datatype);
            }
        }
        param = param->next;
    }

    ast_node_t *body = fun_decl->children[2];
    if (body && body->type == AST_BLOCK) {
        ast_node_t *stmt = body->children[0];
        while (stmt) {
            codegen_stmt(ctx, stmt);
            stmt = stmt->next;
        }
    }

    codegen_emit(ctx, TAC_ENDFUNC, fname, NULL, NULL);
}

void codegen_stmt(codegen_ctx_t *ctx, ast_node_t *stmt)
{
    if (!stmt) return;

    switch (stmt->type) {

        case AST_VAR_DECL: {
            const char *vname = stmt->value;
            sym_entry_t *e = symtab_lookup(ctx->symtab, vname);
            if (e) {
                e->scope  = SYM_SCOPE_LOCAL;
                e->offset = ctx->local_offset;
                ctx->local_offset += type_size(e->datatype);
                char offset_str[16];
                snprintf(offset_str, sizeof(offset_str), "%d", e->offset);
                codegen_emit(ctx, TAC_DECL_LOCAL, vname, offset_str, NULL);
            }

            if (stmt->children[1]) {
                char *val = codegen_expr(ctx, stmt->children[1]);
                codegen_emit(ctx, TAC_COPY, vname, val, NULL);
                free(val);
            }
            break;
        }

        case AST_ASSIGN: {
            if (strcmp(stmt->value, ":=") == 0) {
                fprintf(stderr, "[CODEGEN] Assignment is not implemented yet.\n");
            } else if (strcmp(stmt->value, "+=") == 0) {

                char *lname = stmt->children[0]->value;
                char *rval  = codegen_expr(ctx, stmt->children[1]);
                char *tmp   = tac_new_temp();
                codegen_emit(ctx, TAC_ADD, tmp, lname, rval);
                codegen_emit(ctx, TAC_COPY, lname, tmp, NULL);
                free(rval); free(tmp);
            } else if (strcmp(stmt->value, "-=") == 0) {
                char *lname = stmt->children[0]->value;
                char *rval  = codegen_expr(ctx, stmt->children[1]);
                char *tmp   = tac_new_temp();
                codegen_emit(ctx, TAC_SUB, tmp, lname, rval);
                codegen_emit(ctx, TAC_COPY, lname, tmp, NULL);
                free(rval); free(tmp);
            }
            break;
        }

        case AST_PRINT: {
            char *val = codegen_expr(ctx, stmt->children[0]);
            codegen_emit(ctx, TAC_PRINT, NULL, val, NULL);
            free(val);
            break;
        }

        case AST_READ: {
            const char *lname = stmt->children[0]->value;
            codegen_emit(ctx, TAC_READ, lname, NULL, NULL);
            break;
        }

        case AST_RETURN: {
            if (stmt->children[0]) {
                char *val = codegen_expr(ctx, stmt->children[0]);
                codegen_emit(ctx, TAC_RETURN, NULL, val, NULL);
                free(val);
            } else {
                codegen_emit(ctx, TAC_RETURN_VOID, NULL, NULL, NULL);
            }
            break;
        }

        case AST_CALL: {

            ast_node_t *arg = stmt->children[0];
            int nargs = 0;
            while (arg) { nargs++; arg = arg->next; }
            arg = stmt->children[0];
            while (arg) {
                char *val = codegen_expr(ctx, arg);
                codegen_emit(ctx, TAC_PARAM, NULL, val, NULL);
                free(val);
                arg = arg->next;
            }
            char nargs_str[16];
            snprintf(nargs_str, sizeof(nargs_str), "%d", nargs);
            codegen_emit(ctx, TAC_CALL, NULL, stmt->value, nargs_str);
            break;
        }

        case AST_BLOCK: {
            ast_node_t *s = stmt->children[0];
            while (s) { codegen_stmt(ctx, s); s = s->next; }
            break;
        }

        case AST_IF:
        case AST_WHILE:
        case AST_FOR:
            fprintf(stderr, "[CODEGEN] Controle de fluxo: implementar na Etapa 3.\n");
            break;

        default:
            fprintf(stderr, "[CODEGEN] Comando desconhecido: tipo=%d\n", stmt->type);
            break;
    }
}

char *codegen_expr(codegen_ctx_t *ctx, ast_node_t *expr)
{
    if (!expr) return strdup("_undef");

    switch (expr->type) {

        case AST_SYMBOL:
            return strdup(expr->value);

        case AST_LIT_INT:
        case AST_LIT_FLOAT:
        case AST_LIT_CHAR:
            return strdup(expr->value);
        case AST_LIT_BOOL:
            return strdup(strcmp(expr->value, "true") == 0 ? "1" : "0");
        case AST_LIT_STRING:
            return strdup(expr->value);

        case AST_EXPR_UNARY: {
            char *val = codegen_expr(ctx, expr->children[0]);
            char *tmp = tac_new_temp();
            if (strcmp(expr->value, "-") == 0)
                codegen_emit(ctx, TAC_NEG, tmp, val, NULL);
            else if (strcmp(expr->value, "!") == 0)
                codegen_emit(ctx, TAC_NOT, tmp, val, NULL);
            free(val);
            return tmp;
        }

        case AST_EXPR_BINARY: {
            char *left  = codegen_expr(ctx, expr->children[0]);
            char *right = codegen_expr(ctx, expr->children[1]);
            char *tmp   = tac_new_temp();
            tac_op_t op = op_to_tac(expr->value);

            if (op != TAC_NOP) {
                codegen_emit(ctx, TAC_NOP, tmp, left, right);
            } else {
                fprintf(stderr, "[CODEGEN] Operador desconhecido: '%s'\n", expr->value);
                codegen_emit(ctx, TAC_NOP, tmp, left, right);
            }

            free(left);
            free(right);
            return tmp;
        }

        case AST_EXPR_INDEX: {
            char *idx = codegen_expr(ctx, expr->children[0]);
            char *tmp = tac_new_temp();
            codegen_emit(ctx, TAC_LOAD, tmp, expr->value, idx);
            free(idx);
            return tmp;
        }

        case AST_EXPR_CALL: {
            ast_node_t *arg = expr->children[0];
            int nargs = 0;
            while (arg) { nargs++; arg = arg->next; }
            arg = expr->children[0];
            while (arg) {
                char *val = codegen_expr(ctx, arg);
                codegen_emit(ctx, TAC_PARAM, NULL, val, NULL);
                free(val);
                arg = arg->next;
            }
            char nargs_str[16];
            snprintf(nargs_str, sizeof(nargs_str), "%d", nargs);
            char *tmp = tac_new_temp();
            codegen_emit(ctx, TAC_CALL, tmp, expr->value, nargs_str);
            return tmp;
        }

        default:
            fprintf(stderr, "[CODEGEN] Expressão desconhecida: tipo=%d\n", expr->type);
            return strdup("_err");
    }
}
