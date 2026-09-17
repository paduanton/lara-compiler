/*
 * tac.c — TAC allocation, formatting and list operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tac.h"

static int temp_counter  = 0;
static int label_counter = 0;

tac_instr_t *tac_new(tac_op_t op,
                     const char *result,
                     const char *arg1,
                     const char *arg2)
{
    tac_instr_t *t = (tac_instr_t *) calloc(1, sizeof(tac_instr_t));
    if (!t) { fprintf(stderr, "[TAC] ERRO: falha de alocação.\n"); return NULL; }
    t->op     = op;
    t->result = result ? strdup(result) : NULL;
    t->arg1   = arg1   ? strdup(arg1)   : NULL;
    t->arg2   = arg2   ? strdup(arg2)   : NULL;
    t->next   = NULL;
    return t;
}

tac_instr_t *tac_append(tac_instr_t *list, tac_instr_t *instr)
{
    if (!instr) return list;
    if (!list)  return instr;
    tac_instr_t *cur = list;
    while (cur->next) cur = cur->next;
    cur->next = instr;
    return list;
}

tac_instr_t *tac_concat(tac_instr_t *a, tac_instr_t *b)
{
    if (!a) return b;
    if (!b) return a;
    tac_instr_t *cur = a;
    while (cur->next) cur = cur->next;
    cur->next = b;
    return a;
}

void tac_print(const tac_instr_t *list, FILE *out)
{
    if (!out) out = stdout;
    for (const tac_instr_t *t = list; t; t = t->next) {

        if (t->op != TAC_LABEL)
            fprintf(out, "    ");

        switch (t->op) {
            case TAC_COPY:
                fprintf(out, "%s = %s", t->result, t->arg1); break;
            case TAC_ADD: case TAC_SUB: case TAC_MUL:
            case TAC_DIV: case TAC_MOD:
            case TAC_LT:  case TAC_GT:  case TAC_LE:
            case TAC_GE:  case TAC_EQ:  case TAC_NE:
            case TAC_AND: case TAC_OR:
                fprintf(out, "%s = %s %s %s",
                    t->result, t->arg1, tac_op_name(t->op), t->arg2); break;
            case TAC_NEG:
                fprintf(out, "%s = -%s", t->result, t->arg1); break;
            case TAC_NOT:
                fprintf(out, "%s = !%s", t->result, t->arg1); break;
            case TAC_LABEL:
                fprintf(out, "%s:", t->result); break;
            case TAC_JUMP:
                fprintf(out, "goto %s", t->result); break;
            case TAC_JUMPF:
                fprintf(out, "ifFalse %s goto %s", t->arg1, t->result); break;
            case TAC_JUMPT:
                fprintf(out, "ifTrue %s goto %s", t->arg1, t->result); break;
            case TAC_LOAD:
                fprintf(out, "%s = %s[%s]", t->result, t->arg1, t->arg2); break;
            case TAC_STORE:
                fprintf(out, "%s[%s] = %s", t->result, t->arg1, t->arg2); break;
            case TAC_BEGINFUNC:
                fprintf(out, "beginFunc %s", t->result); break;
            case TAC_ENDFUNC:
                fprintf(out, "endFunc %s", t->result); break;
            case TAC_PARAM:
                fprintf(out, "param %s", t->arg1); break;
            case TAC_CALL:
                if (t->result)
                    fprintf(out, "%s = call %s %s", t->result, t->arg1, t->arg2);
                else
                    fprintf(out, "call %s %s", t->arg1, t->arg2);
                break;
            case TAC_RETURN:
                fprintf(out, "return %s", t->arg1); break;
            case TAC_RETURN_VOID:
                fprintf(out, "return"); break;
            case TAC_DECL_GLOBAL:
                fprintf(out, "global %s, %s", t->result, t->arg1); break;
            case TAC_DECL_LOCAL:
                fprintf(out, "local %s, %s", t->result, t->arg1); break;
            case TAC_PRINT:
                fprintf(out, "print %s", t->arg1); break;
            case TAC_READ:
                fprintf(out, "read %s", t->result); break;
            case TAC_NOP:
                fprintf(out, "nop"); break;
            default:
                fprintf(out, "??? op=%d", t->op); break;
        }
        fprintf(out, "\n");
    }
}

void tac_free(tac_instr_t *list)
{
    while (list) {
        tac_instr_t *next = list->next;
        free(list->result);
        free(list->arg1);
        free(list->arg2);
        free(list);
        list = next;
    }
}

char *tac_new_temp(void)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "_t%d", ++temp_counter);
    return strdup(buf);
}

char *tac_new_label(void)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "_L%d", ++label_counter);
    return strdup(buf);
}

void tac_reset_counters(void)
{
    temp_counter  = 0;
    label_counter = 0;
}

const char *tac_op_name(tac_op_t op)
{
    switch (op) {
        case TAC_ADD: return "+";   case TAC_SUB: return "-";
        case TAC_MUL: return "*";   case TAC_DIV: return "/";
        case TAC_MOD: return "%";   case TAC_LT:  return "<";
        case TAC_GT:  return ">";   case TAC_LE:  return "<=";
        case TAC_GE:  return ">=";  case TAC_EQ:  return "==";
        case TAC_NE:  return "!=";  case TAC_AND: return "&&";
        case TAC_OR:  return "||";
        default:      return "?";
    }
}
