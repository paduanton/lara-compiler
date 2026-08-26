/*
 * tokens.h — LARA token values
 *
 * Manually mirrors the %token values declared in parser.y, for external
 * reference. The actual build uses bison's generated parser.tab.h (included
 * by scanner.l) — keep this file in sync with parser.y if it's ever edited.
 */

#ifndef TOKENS_H
#define TOKENS_H

#define TK_PR_INT       256
#define TK_PR_FLOAT     257
#define TK_PR_BOOL      258
#define TK_PR_CHAR      259
#define TK_PR_IF        260
#define TK_PR_ELSE      261
#define TK_PR_WHILE     262
#define TK_PR_FOR       263
#define TK_PR_RETURN    264
#define TK_PR_VOID      265
#define TK_PR_ARRAY     266
#define TK_PR_OF        267
#define TK_PR_LET       268
#define TK_PR_FUN       269
#define TK_PR_DO        270
#define TK_PR_PRINT     271
#define TK_PR_READ      272

#define TK_ID           300
#define TK_LIT_INT      301
#define TK_LIT_FLOAT    302
#define TK_LIT_CHAR     303
#define TK_LIT_STRING   304
#define TK_LIT_TRUE     305
#define TK_LIT_FALSE    306

#define TK_OC_LE        320   /* <= */
#define TK_OC_GE        321   /* >= */
#define TK_OC_EQ        322   /* == */
#define TK_OC_NE        323   /* != */
#define TK_OC_AND       324   /* && */
#define TK_OC_OR        325   /* || */
#define TK_OC_ARROW     326   /* -> */
#define TK_OC_ASSIGN    327   /* := */
#define TK_OC_PLUSEQ    328   /* += */
#define TK_OC_MINUSEQ   329   /* -= */

/* Single-char tokens (+ - * / % < > ! = ( ) { } [ ] , ; : .) are returned
 * directly by the scanner as their ASCII value — no #define needed. */

#define TK_ERROR        400

#endif /* TOKENS_H */
