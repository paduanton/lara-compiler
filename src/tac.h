/*
 * tac.h — TAC instruction and list interface
 */

#ifndef TAC_H
#define TAC_H

#include <stdio.h>

typedef enum {

    TAC_COPY,

    TAC_ADD,
    TAC_SUB,
    TAC_MUL,
    TAC_DIV,
    TAC_MOD,
    TAC_NEG,

    TAC_LT,
    TAC_GT,
    TAC_LE,
    TAC_GE,
    TAC_EQ,
    TAC_NE,

    TAC_NOT,
    TAC_AND,
    TAC_OR,

    TAC_LABEL,
    TAC_JUMP,
    TAC_JUMPF,
    TAC_JUMPT,

    TAC_LOAD,
    TAC_STORE,          /* result[arg1] = arg2 */

    TAC_BEGINFUNC,
    TAC_ENDFUNC,
    TAC_PARAM,
    TAC_CALL,
    TAC_RETURN,
    TAC_RETURN_VOID,

    TAC_DECL_GLOBAL,
    TAC_DECL_LOCAL,

    TAC_PRINT,
    TAC_READ,

    TAC_NOP,
    TAC_OP_COUNT
} tac_op_t;

typedef struct tac_instr {
    tac_op_t          op;
    char             *result;
    char             *arg1;
    char             *arg2;
    struct tac_instr *next;
} tac_instr_t;

/* Copies all non-NULL address strings; the list owns those copies. */
tac_instr_t *tac_new(tac_op_t op,
                     const char *result,
                     const char *arg1,
                     const char *arg2);

tac_instr_t *tac_append(tac_instr_t *list, tac_instr_t *instr);

tac_instr_t *tac_concat(tac_instr_t *a, tac_instr_t *b);

void tac_print(const tac_instr_t *list, FILE *out);

void tac_free(tac_instr_t *list);

/* Returned names are owned by the caller. Counters span the whole program. */
char *tac_new_temp(void);

char *tac_new_label(void);

void tac_reset_counters(void);

const char *tac_op_name(tac_op_t op);

#endif
