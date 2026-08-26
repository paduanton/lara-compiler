/*
 * symtab.h — Symbol table interface
 *
 * Chained hash table (SYMTAB_SIZE buckets). At this stage every identifier
 * is inserted with nature/datatype unknown; `nature`, `datatype`, `offset`,
 * and nested scoping are unused placeholders for later stages.
 */

#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>

#define SYMTAB_SIZE 509  /* prime, for a better hash distribution */

typedef enum {
    SYM_UNKNOWN  = 0,
    SYM_VAR      = 1,
    SYM_ARRAY    = 2,
    SYM_FUNCTION = 3
} sym_nature_t;

typedef enum {
    SYM_TYPE_UNKNOWN = 0,
    SYM_TYPE_INT     = 1,
    SYM_TYPE_FLOAT   = 2,
    SYM_TYPE_CHAR    = 3,
    SYM_TYPE_BOOL    = 4,
    SYM_TYPE_VOID    = 5,
    SYM_TYPE_STRING  = 6   /* literals only — not a variable type */
} sym_datatype_t;

typedef struct sym_entry {
    char           *lexeme;
    int             lineno;    /* first occurrence */
    sym_nature_t    nature;
    sym_datatype_t  datatype;
    int             offset;
    struct sym_entry *next;    /* collision chain */
} sym_entry_t;

typedef struct {
    sym_entry_t *buckets[SYMTAB_SIZE];
    int          count;
} symtab_t;

/* Returns NULL on allocation failure. */
symtab_t *symtab_new(void);

/*
 * symtab_insert — returns the existing entry if `lexeme` is already present,
 * otherwise creates one (nature/datatype unknown). Never NULL unless OOM.
 */
sym_entry_t *symtab_insert(symtab_t *tab, const char *lexeme, int lineno);

/* symtab_lookup — returns NULL if not found. */
sym_entry_t *symtab_lookup(symtab_t *tab, const char *lexeme);

void symtab_print(const symtab_t *tab, FILE *out);
void symtab_free(symtab_t *tab);

#endif /* SYMTAB_H */
