/*
 * backpatch.h — Pending jump lists
 */
#ifndef BACKPATCH_H
#define BACKPATCH_H

#include "tac.h"

typedef struct patch_list {
    tac_instr_t *instr;
    struct patch_list *next;
} patch_list_t;

patch_list_t *patch_list_make(tac_instr_t *instr);

/* Transfers both lists into the returned list. */
patch_list_t *patch_list_merge(patch_list_t *l1, patch_list_t *l2);
void patch_list_backpatch(patch_list_t *list, const char *label);

/* Frees list nodes; the TAC list owns the instructions. */
void patch_list_free(patch_list_t *list);

typedef struct {
    patch_list_t *true_list;
    patch_list_t *false_list;
} bool_result_t;

#endif
