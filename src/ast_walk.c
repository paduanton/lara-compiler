/*
 * ast_walk.c — AST statistics: node count, leaf count, max depth.
 */

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

int ast_count_nodes(const ast_node_t *node)
{
    if (node == NULL)
        return 0;

    int count = 1;

    for (int i = 0; i < AST_MAX_CHILDREN; i++)
        count += ast_count_nodes(node->children[i]);

    count += ast_count_nodes(node->next);

    return count;
}

int ast_count_leaves(const ast_node_t *node)
{
    if (node == NULL)
        return 0;

    int count = 0;
    for (int i = 0; i < AST_MAX_CHILDREN; i++)
        count += ast_count_leaves(node->children[i]);
    count += ast_count_leaves(node->next);

    return (count == 0) ? 1 : count;
}

static int max(int a, int b) { return (a > b) ? a : b; }

/* TODO: implement — children add +1 depth, next stays at the same depth */
int ast_max_depth(const ast_node_t *node)
{
    (void)node;
    (void)max;
    return 0;
}

void main_walk(const ast_node_t *root)
{
    if (root == NULL) {
        printf("[WALK] AST vazia — nada a percorrer.\n");
        return;
    }

    printf("\n=== Estatísticas da AST ===\n");
    printf("  Nós totais  : %d\n", ast_count_nodes(root));
    printf("  Folhas      : %d\n", ast_count_leaves(root));
    printf("  Profund. max: %d\n", ast_max_depth(root));
    printf("===========================\n\n");
}
