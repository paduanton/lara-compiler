/*
 * backpatch.c — Pending jump resolution
 */
#include <stdlib.h>
#include <string.h>
#include "backpatch.h"

patch_list_t *patch_list_make(tac_instr_t *instr)
{
    patch_list_t *p = calloc(1, sizeof(patch_list_t));
    if (!p) return NULL;
    p->instr = instr;
    p->next = NULL;
    return p;
}

patch_list_t *patch_list_merge(patch_list_t *l1, patch_list_t *l2)
{
    if (!l1) return l2;
    patch_list_t *tail = l1;
    while (tail->next) tail = tail->next;
    tail->next = l2;
    return l1;
}

void patch_list_backpatch(patch_list_t *list, const char *label)
{
    for (patch_list_t *p = list; p; p = p->next) {
        char *target = strdup(label);
        if (!target) return;
        free(p->instr->result);
        p->instr->result = target;
    }
}

void patch_list_free(patch_list_t *list)
{
    while (list) {
        patch_list_t *next = list->next;
        free(list);
        list = next;
    }
}
