/*
 * test_core.c — symbol offsets, AST ownership and TAC representation checks
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"
#include "codegen.h"

symtab_t *global_symtab;
extern ast_node_t *ast_root;
extern int yyparse(void);
extern int yylex_destroy(void);
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *source);
extern int ast_count_nodes(const ast_node_t *node);
extern int ast_count_leaves(const ast_node_t *node);
extern int ast_max_depth(const ast_node_t *node);

static void check_symbol(const char *name, int offset, sym_scope_t scope,
                         sym_datatype_t datatype, int array_size)
{
    sym_entry_t *entry = symtab_lookup(global_symtab, name);
    assert(entry);
    assert(entry->offset == offset);
    assert(entry->scope == scope);
    assert(entry->datatype == datatype);
    assert(entry->array_size == array_size);
}

static void test_offsets(void)
{
    const char *source =
        "int ga; float gb; int gc; char gd; bool ge;"
        "array 3 of float vector; array 10 of char chars;"
        "fun int first(int p, int q) { let local_a := p + q; return local_a; }"
        "fun void second() { let local_b := 2 * 3; } int tail;";
    global_symtab = symtab_new();
    assert(global_symtab);
    assert(yy_scan_string(source));
    assert(yyparse() == 0);
    yylex_destroy();
    tac_reset_counters();
    codegen_ctx_t *ctx = codegen_new(global_symtab);
    assert(ctx);
    codegen_program(ctx, ast_root);
    check_symbol("ga", 0, SYM_SCOPE_GLOBAL, SYM_TYPE_INT, 0);
    check_symbol("gb", 4, SYM_SCOPE_GLOBAL, SYM_TYPE_FLOAT, 0);
    check_symbol("gc", 12, SYM_SCOPE_GLOBAL, SYM_TYPE_INT, 0);
    check_symbol("gd", 16, SYM_SCOPE_GLOBAL, SYM_TYPE_CHAR, 0);
    check_symbol("ge", 17, SYM_SCOPE_GLOBAL, SYM_TYPE_BOOL, 0);
    check_symbol("vector", 18, SYM_SCOPE_GLOBAL, SYM_TYPE_FLOAT, 3);
    check_symbol("chars", 42, SYM_SCOPE_GLOBAL, SYM_TYPE_CHAR, 10);
    check_symbol("tail", 52, SYM_SCOPE_GLOBAL, SYM_TYPE_INT, 0);
    check_symbol("p", -4, SYM_SCOPE_LOCAL, SYM_TYPE_UNKNOWN, 0);
    check_symbol("q", -8, SYM_SCOPE_LOCAL, SYM_TYPE_UNKNOWN, 0);
    check_symbol("local_a", 0, SYM_SCOPE_LOCAL, SYM_TYPE_UNKNOWN, 0);
    check_symbol("local_b", 0, SYM_SCOPE_LOCAL, SYM_TYPE_UNKNOWN, 0);
    int expressions = 0;
    for (tac_instr_t *i = ctx->code; i; i = i->next) {
        assert(i->op != TAC_NOP);
        if (i->op == TAC_ADD) {
            assert(strcmp(i->result, "_t1") == 0);
            expressions++;
        } else if (i->op == TAC_MUL) {
            assert(strcmp(i->result, "_t2") == 0);
            expressions++;
        }
    }
    assert(expressions == 2);
    tac_free(ctx->code);
    codegen_free(ctx);
    ast_free(ast_root);
    ast_root = NULL;
    symtab_free(global_symtab);
    global_symtab = NULL;
}

static void test_ast_statistics(void)
{
    assert(ast_count_nodes(NULL) == 0);
    assert(ast_count_leaves(NULL) == 0);
    assert(ast_max_depth(NULL) == -1);
    ast_node_t *root = ast_new(AST_PROGRAM, NULL, 1);
    ast_node_t *first = ast_new(AST_VAR_DECL, "a", 1);
    ast_node_t *second = ast_new(AST_VAR_DECL, "b", 1);
    ast_node_t *sum = ast_new(AST_EXPR_BINARY, "+", 1);
    root->children[0] = ast_append(first, second);
    first->children[1] = ast_new(AST_LIT_INT, "1", 1);
    second->children[1] = sum;
    sum->children[0] = ast_new(AST_LIT_INT, "2", 1);
    sum->children[1] = ast_new(AST_LIT_INT, "3", 1);
    assert(ast_count_nodes(root) == 7);
    assert(ast_count_leaves(root) == 3);
    assert(ast_max_depth(root) == 3);
    ast_free(root);
    first = ast_new(AST_LIT_INT, "1", 1);
    first->next = ast_new(AST_LIT_INT, "2", 1);
    assert(ast_count_nodes(first) == 2);
    assert(ast_count_leaves(first) == 1);
    assert(ast_max_depth(first) == 0);
    ast_free(first);
}

static void test_store_contract(void)
{
    char array[] = "v";
    tac_instr_t *store = tac_new(TAC_STORE, array, "2", "10");
    assert(store);
    array[0] = 'x';
    assert(strcmp(store->result, "v") == 0);
    FILE *output = tmpfile();
    assert(output);
    tac_print(store, output);
    rewind(output);
    char text[64];
    assert(fgets(text, sizeof(text), output));
    assert(strcmp(text, "    v[2] = 10\n") == 0);
    assert(fgetc(output) == EOF);
    fclose(output);
    tac_free(store);
}

int main(void)
{
    test_offsets();
    test_ast_statistics();
    test_store_contract();
    puts("OK: symbol offsets, temporary names, AST statistics and TAC storage");
    return 0;
}
