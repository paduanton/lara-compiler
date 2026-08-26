/*
 * ast.h — Abstract Syntax Tree interface
 *
 * Lists (statement blocks, parameter lists, argument lists) are represented
 * as a chain via `next` rather than nested children, so a linear list of N
 * items doesn't add N levels of tree depth.
 */

#ifndef AST_H
#define AST_H

#include <stdio.h>

#define AST_MAX_CHILDREN 4

typedef enum {
    /* top-level */
    AST_PROGRAM,
    AST_FUN_DECL,
    AST_VAR_DECL,
    AST_ARRAY_DECL,
    AST_PARAM,

    /* statements */
    AST_BLOCK,
    AST_ASSIGN,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_PRINT,
    AST_READ,
    AST_CALL,           /* function call used as a statement */

    /* expressions */
    AST_EXPR_CALL,       /* function call used as an expression */
    AST_EXPR_INDEX,      /* array indexing: v[i] */
    AST_EXPR_ARROW,       /* pointer field access: p->field */
    AST_EXPR_UNARY,
    AST_EXPR_BINARY,

    /* leaves */
    AST_SYMBOL,
    AST_LIT_INT,
    AST_LIT_FLOAT,
    AST_LIT_CHAR,
    AST_LIT_STRING,
    AST_LIT_BOOL,

    AST_NODE_TYPES_COUNT  /* sentinel — not a valid node type */
} ast_node_type_t;

typedef struct ast_node {
    ast_node_type_t     type;
    char               *value;                      /* lexeme (leaves) or operator (BINARY/UNARY); NULL otherwise */
    int                 lineno;
    struct ast_node    *children[AST_MAX_CHILDREN];
    struct ast_node    *next;                        /* next sibling in a list */
} ast_node_t;

/*
 * ast_new — allocates a node (calloc: children/next start NULL). `value` is
 * strdup'd if non-NULL. Returns NULL on allocation failure.
 */
ast_node_t *ast_new(ast_node_type_t type, const char *value, int lineno);

/* ast_add_child — no-op if parent is NULL or index is out of range. */
void ast_add_child(ast_node_t *parent, int index, ast_node_t *child);

/*
 * ast_append — links `node` at the tail of `list`. Returns `list` (unchanged
 * head) if list != NULL, otherwise returns `node` as the new head.
 */
ast_node_t *ast_append(ast_node_t *list, ast_node_t *node);

/* ast_print — indented recursive dump; pass indent=0 at the root call. */
void ast_print(const ast_node_t *root, int indent, FILE *out);

/* ast_free — recursively frees children, next-chain, lexemes, and the node itself. */
void ast_free(ast_node_t *root);

const char *ast_type_name(ast_node_type_t type);

#endif /* AST_H */
