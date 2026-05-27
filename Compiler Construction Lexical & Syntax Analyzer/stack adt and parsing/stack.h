/*  ============================================================
 *  CS F363 – Compiler Construction | BITS Pilani 2026
 *  FILE : stack.h
 *  DESC : Stack ADT interface used by the LL(1) parser.
 *         The stack stores grammar symbols (terminals and
 *         non-terminals). Each entry also carries a pointer
 *         to the parse-tree node created during parsing.
 *  ============================================================ */

#ifndef STACK_H
#define STACK_H

#include "parserDef.h"

/* Forward declaration (ParseTreeNode defined in parser.h) */
struct ParseTreeNode;

/* ─────────────────────────────────────────────────────────────
 * Stack entry: one grammar symbol + its parse-tree node
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    Symbol              symbol;
    struct ParseTreeNode *treeNode;  /* NULL until node is created */
} StackEntry;

/* ─────────────────────────────────────────────────────────────
 * Stack structure  (array-based, fixed capacity)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    StackEntry data[STACK_MAX_SIZE];
    int        top;     /* index of top element; -1 if empty */
} Stack;

/* ─────────────────────────────────────────────────────────────
 * Public API
 * ───────────────────────────────────────────────────────────── */

/* Initialise (or reset) a stack */
void  stackInit(Stack *s);

/* Returns 1 if the stack is empty, 0 otherwise */
int   stackIsEmpty(const Stack *s);

/* Returns 1 if the stack is full (overflow guard), 0 otherwise */
int   stackIsFull(const Stack *s);

/* Push a symbol onto the stack.
 * treeNode may be NULL when the node is not yet allocated.
 * Returns 0 on success, -1 on overflow.                      */
int   stackPush(Stack *s, Symbol sym, struct ParseTreeNode *treeNode);

/* Pop the top entry.  Returns 0 on success, -1 if empty.     */
int   stackPop(Stack *s);

/* Peek at the top entry without removing it.
 * Returns NULL if the stack is empty.                        */
StackEntry *stackTop(Stack *s);

/* Print stack contents for debugging (top → bottom).        */
void  stackPrint(const Stack *s);

/* Return the current depth (number of elements).            */
int   stackDepth(const Stack *s);

#endif /* STACK_H */
