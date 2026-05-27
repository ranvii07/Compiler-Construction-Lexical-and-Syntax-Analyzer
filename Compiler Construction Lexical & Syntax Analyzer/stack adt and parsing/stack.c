/*  ============================================================
 *  CS F363 – Compiler Construction | BITS Pilani 2026
 *  FILE : stack.c
 *  DESC : Stack ADT implementation (array-based).
 *  ============================================================ */

#include "stack.h"
#include "parserDef.h"
#include <stdio.h>

/* Name tables live in parser.c; declared extern here for printing */
extern const char *terminalNames[NUM_TERMINALS];
extern const char *nonTerminalNames[NUM_NON_TERMINALS];

/* ─── stackInit ────────────────────────────────────────────── */
void stackInit(Stack *s) {
    s->top = -1;
}

/* ─── stackIsEmpty ─────────────────────────────────────────── */
int stackIsEmpty(const Stack *s) {
    return s->top == -1;
}

/* ─── stackIsFull ──────────────────────────────────────────── */
int stackIsFull(const Stack *s) {
    return s->top == STACK_MAX_SIZE - 1;
}

/* ─── stackPush ────────────────────────────────────────────── */
int stackPush(Stack *s, Symbol sym, struct ParseTreeNode *treeNode) {
    if (stackIsFull(s)) {
        fprintf(stderr, "[STACK] Overflow – cannot push.\n");
        return -1;
    }
    s->top++;
    s->data[s->top].symbol   = sym;
    s->data[s->top].treeNode = treeNode;
    return 0;
}

/* ─── stackPop ─────────────────────────────────────────────── */
int stackPop(Stack *s) {
    if (stackIsEmpty(s)) {
        fprintf(stderr, "[STACK] Underflow – cannot pop.\n");
        return -1;
    }
    s->top--;
    return 0;
}

/* ─── stackTop ─────────────────────────────────────────────── */
StackEntry *stackTop(Stack *s) {
    if (stackIsEmpty(s)) return NULL;
    return &s->data[s->top];
}

/* ─── stackDepth ───────────────────────────────────────────── */
int stackDepth(const Stack *s) {
    return s->top + 1;
}

/* ─── stackPrint ───────────────────────────────────────────── */
/*  Prints stack from TOP down to BOTTOM – useful for debugging */
void stackPrint(const Stack *s) {
    if (stackIsEmpty(s)) {
        printf("[STACK]  <empty>\n");
        return;
    }
    printf("[STACK]  depth=%d  (top → bottom):\n", s->top + 1);
    for (int i = s->top; i >= 0; i--) {
        const Symbol *sym = &s->data[i].symbol;
        printf("  [%3d]  ", i);
        if (sym->type == SYM_TERMINAL) {
            printf("TERM  %-20s", terminalNames[sym->value]);
        } else if (sym->type == SYM_NON_TERMINAL) {
            printf("NT    %-20s", nonTerminalNames[sym->value]);
        } else {
            printf("EPS   ε                   ");
        }
        if (s->data[i].treeNode != NULL)
            printf("  (tree-node @ %p)", (void*)s->data[i].treeNode);
        printf("\n");
    }
}
