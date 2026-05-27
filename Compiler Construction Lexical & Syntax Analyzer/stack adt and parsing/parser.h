/*  ============================================================
 *  CS F363 – Compiler Construction | BITS Pilani 2026
 *  FILE : parser.h
 *  DESC : LL(1) predictive parser interface.
 *
 *  Modules provided:
 *    1. Grammar rule table  (89 corrected productions)
 *    2. Predictive parse table  M[NT][Terminal] → rule number
 *    3. Parse tree node / Tree ADT
 *    4. Parser driver  (stack-based LL(1) algorithm)
 *  ============================================================ */

#ifndef PARSER_H
#define PARSER_H

#include "parserDef.h"
#include "stack.h"

/* ─────────────────────────────────────────────────────────────
 * PARSE TREE  –  Tree ADT
 *
 *  Every node represents either a terminal leaf or a
 *  non-terminal internal node.  Children are stored as a
 *  singly-linked list (left-child / right-sibling).
 * ───────────────────────────────────────────────────────────── */
#define MAX_CHILDREN 10

typedef struct ParseTreeNode {
    Symbol  symbol;               /* what this node represents   */
    char    lexeme[MAX_LEXEME];   /* filled for terminal leaves  */
    int     lineNumber;           /* source line (terminals)     */
    int     ruleApplied;          /* rule# used to expand (NTs) */

    struct ParseTreeNode *children[MAX_CHILDREN];
    int                   numChildren;

    struct ParseTreeNode *parent;  /* back-pointer (optional)   */
} ParseTreeNode;

/* ─────────────────────────────────────────────────────────────
 * PARSER STATE
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    Stack          stack;
    ParseTreeNode *root;          /* root of the parse tree      */
    int            errorCount;    /* total syntax errors found   */
    int            lineNumber;    /* current token's line        */
} Parser;

/* ─────────────────────────────────────────────────────────────
 * PUBLIC API
 * ───────────────────────────────────────────────────────────── */

/* ── Initialisation ── */

/* Build the grammar rule table (called once at startup) */
void initGrammarRules(void);

/* Build the predictive parse table (called once at startup) */
void initParseTable(void);

/* ── Parse table query ──
 *   Returns rule number (1-89), or PARSE_ERROR (-1) if no entry. */
int getParseTableEntry(NonTerminal nt, Terminal t);

/* ── Parse tree helpers ── */
ParseTreeNode *newParseTreeNode(Symbol sym);
void           addChild(ParseTreeNode *parent, ParseTreeNode *child);
void           printParseTree(const ParseTreeNode *node, int depth);
void           freeParseTree(ParseTreeNode *node);

/* ── Core parser driver ──
 *
 *  tokenStream : NULL-terminated array of Token pointers (from lexer)
 *  Returns     : pointer to the root ParseTreeNode, or NULL on failure.
 *
 *  Algorithm (LL(1) stack-based):
 *    1. Push $ then start symbol (program) onto the stack.
 *    2. Repeat:
 *       a. X  ← top of stack
 *       b. a  ← current input token
 *       c. If X == a == $  → accept.
 *       d. If X is terminal:
 *            If X == a  → match, pop, advance input.
 *            Else       → syntax error (unexpected token).
 *       e. If X is non-terminal:
 *            If M[X][a] = rule r  → pop X, push RHS(r) in reverse.
 *            Else                 → syntax error (no production).
 */
ParseTreeNode *parse(Token **tokenStream, int tokenCount);

/* ── Print the parse table (diagnostic) ── */
void printParseTable(void);

/* ── Convenience: parse directly from a source file ──
 *   This stub assumes the caller provides a getNextToken() function. */
ParseTreeNode *parseFromFile(const char *filename);

/* ─────────────────────────────────────────────────────────────
 * GRAMMAR RULE TABLE  (extern – defined in parser.c)
 * ───────────────────────────────────────────────────────────── */
extern GrammarRule grammarRules[NUM_RULES];

/* ─────────────────────────────────────────────────────────────
 * PARSE TABLE  (extern – defined in parser.c)
 *   parseTable[NT][Terminal] = rule number, or PARSE_ERROR
 * ───────────────────────────────────────────────────────────── */
extern int parseTable[NUM_NON_TERMINALS][NUM_TERMINALS];

#endif /* PARSER_H */
