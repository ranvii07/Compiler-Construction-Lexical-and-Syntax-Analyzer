/*  ============================================================
 *  CS F363 – Compiler Construction | BITS Pilani 2026
 *  FILE : main.c
 *  DESC : Driver to demonstrate and test the LL(1) parser.
 *
 *  Compiling:
 *    gcc -Wall -Wextra -o parser main.c stack.c parser.c
 *
 *  Usage:
 *    ./parser            – runs built-in test suites
 *    ./parser table      – prints the full parse table
 *    ./parser tree       – parses a demo token stream & prints tree
 *  ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parserDef.h"
#include "stack.h"
#include "parser.h"

/* ─────────────────────────────────────────────────────────────
 * Utility: build a Token on the heap
 * ───────────────────────────────────────────────────────────── */
static Token *makeToken(Terminal type, const char *lexeme, int line) {
    Token *t = (Token *)calloc(1, sizeof(Token));
    t->type       = type;
    t->lineNumber = line;
    strncpy(t->lexeme, lexeme, MAX_LEXEME - 1);
    return t;
}

/* Free a NULL-terminated array of Token* */
static void freeTokens(Token **arr, int count) {
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

/* ─────────────────────────────────────────────────────────────
 * TEST 1 – Stack ADT unit tests
 * ───────────────────────────────────────────────────────────── */
static void testStackADT(void) {
    printf("═══════════════════════════════════\n");
    printf("  TEST 1 – Stack ADT\n");
    printf("═══════════════════════════════════\n");

    Stack s;
    stackInit(&s);

    printf("Empty? %s  (expect yes)\n", stackIsEmpty(&s) ? "yes" : "no");

    Symbol sym1 = { SYM_TERMINAL,     TK_ID     };
    Symbol sym2 = { SYM_NON_TERMINAL, NT_PROGRAM };
    Symbol sym3 = { SYM_TERMINAL,     TK_SEM    };

    stackPush(&s, sym1, NULL);
    stackPush(&s, sym2, NULL);
    stackPush(&s, sym3, NULL);

    printf("Depth after 3 pushes: %d  (expect 3)\n", stackDepth(&s));
    stackPrint(&s);

    /* Pop one and inspect top */
    stackPop(&s);
    StackEntry *top = stackTop(&s);
    printf("Top after 1 pop: %s  (expect NT program)\n",
           top->symbol.type == SYM_NON_TERMINAL
               ? nonTerminalNames[top->symbol.value]
               : terminalNames[top->symbol.value]);

    stackPop(&s);
    stackPop(&s);
    printf("Empty after 3 pops? %s  (expect yes)\n",
           stackIsEmpty(&s) ? "yes" : "no");

    /* Underflow guard */
    int rc = stackPop(&s);
    printf("Pop from empty returns: %d  (expect -1)\n", rc);
    printf("\n");
}

/* ─────────────────────────────────────────────────────────────
 * TEST 2 – Parse table spot checks
 * ───────────────────────────────────────────────────────────── */
static void testParseTable(void) {
    printf("═══════════════════════════════════\n");
    printf("  TEST 2 – Parse Table Spot Checks\n");
    printf("═══════════════════════════════════\n");

    typedef struct { NonTerminal nt; Terminal t; int expected; const char *desc; } Check;

    Check checks[] = {
        { NT_PROGRAM,          TK_FUNID,    1,  "program on FUNID → rule 1"          },
        { NT_PROGRAM,          TK_MAIN,     1,  "program on MAIN  → rule 1"          },
        { NT_OTHER_FUNCTIONS,  TK_FUNID,    3,  "otherFunctions on FUNID → rule 3"   },
        { NT_OTHER_FUNCTIONS,  TK_MAIN,     4,  "otherFunctions on MAIN  → ε(4)"     },
        { NT_PRIMITIVE_DATATYPE,TK_INT,    12,  "primitiveDatatype on INT  → rule 12" },
        { NT_PRIMITIVE_DATATYPE,TK_REAL,   13,  "primitiveDatatype on REAL → rule 13" },
        { NT_CONSTRUCTED_DATATYPE, TK_RECORD, 14, "constructedDatatype on RECORD → 14"},
        { NT_CONSTRUCTED_DATATYPE, TK_RUID,   16, "constructedDatatype on RUID  → 16 (alias)"},
        { NT_ARITH_EXPR,       TK_OP,      57,  "arithExpr on '(' → rule 57"         },
        { NT_ARITH_EXPR_PRIME, TK_PLUS,    58,  "arithExpPrime on '+' → rule 58"     },
        { NT_ARITH_EXPR_PRIME, TK_MINUS,   59,  "arithExpPrime on '-' → rule 59"     },
        { NT_ARITH_EXPR_PRIME, TK_SEM,     60,  "arithExpPrime on ';' → ε(60)"       },
        { NT_TERM_PRIME,       TK_MUL,     62,  "termPrime on '*' → rule 62"         },
        { NT_TERM_PRIME,       TK_DIV,     63,  "termPrime on '/' → rule 63"         },
        { NT_TERM_PRIME,       TK_PLUS,    64,  "termPrime on '+' → ε(64)"           },
        { NT_CONDITIONAL_STMT, TK_IF,      52,  "conditionalStmt on IF → rule 52"    },
        { NT_ELSE_PART,        TK_ELSE,    53,  "elsePart on ELSE  → rule 53"        },
        { NT_ELSE_PART,        TK_ENDIF,   54,  "elsePart on ENDIF → ε(54)"          },
        { NT_BOOL_EXPR,        TK_OP,      70,  "boolExpr on '(' → rule 70"          },
        { NT_BOOL_EXPR,        TK_ID,      71,  "boolExpr on ID  → rule 71"          },
        { NT_BOOL_EXPR,        TK_NOT,     72,  "boolExpr on NOT → rule 72"          },
        { NT_SOREC_SUFFIX,     TK_DOT,     45,  "singleOrRecIdSuffix on '.' → 45"    },
        { NT_SOREC_SUFFIX,     TK_ASSIGNOP,46,  "singleOrRecIdSuffix on ASSIGN → ε(46)"},
        { NT_GLOBAL_OR_NOT,    TK_COLON,   34,  "global_or_not on ':' → 34"          },
        { NT_GLOBAL_OR_NOT,    TK_SEM,     35,  "global_or_not on ';' → ε(35)"       },
        { NT_DEFINETYPE_STMT,  TK_DEFINETYPE,87,"definetypestmt on DEFINETYPE → 87" },
        /* Error cell */
        { NT_PROGRAM,          TK_PLUS,    -1,  "program on '+' → ERROR"             },
        { NT_ARITH_EXPR_PRIME, TK_MUL,     -1,  "arithExpPrime on '*' → ERROR"       },
    };

    int pass = 0, fail = 0;
    int total = (int)(sizeof(checks) / sizeof(checks[0]));
    for (int i = 0; i < total; i++) {
        int got = getParseTableEntry(checks[i].nt, checks[i].t);
        int ok  = (got == checks[i].expected);
        printf("  [%s] %s  (got %d, expected %d)\n",
               ok ? "PASS" : "FAIL",
               checks[i].desc, got, checks[i].expected);
        ok ? pass++ : fail++;
    }
    printf("\n  Results: %d/%d passed\n\n", pass, total);
}

/* ─────────────────────────────────────────────────────────────
 * TEST 3 – Parse a minimal valid program
 *
 *  Source equivalent:
 *    _main
 *    type int : b2 ;
 *    read(b2) ;
 *    return ;
 *    end
 * ───────────────────────────────────────────────────────────── */
static void testParseMinimal(void) {
    printf("═══════════════════════════════════════════════════════\n");
    printf("  TEST 3 – Parse minimal valid program\n");
    printf("    _main  type int: b2;  read(b2);  return;  end\n");
    printf("═══════════════════════════════════════════════════════\n");

    Token **ts = (Token **)malloc(20 * sizeof(Token *));
    int i = 0;
    ts[i++] = makeToken(TK_MAIN,    "_main",  1);
    ts[i++] = makeToken(TK_TYPE,    "type",   2);
    ts[i++] = makeToken(TK_INT,     "int",    2);
    ts[i++] = makeToken(TK_COLON,   ":",      2);
    ts[i++] = makeToken(TK_ID,      "b2",     2);
    ts[i++] = makeToken(TK_SEM,     ";",      2);
    ts[i++] = makeToken(TK_READ,    "read",   3);
    ts[i++] = makeToken(TK_OP,      "(",      3);
    ts[i++] = makeToken(TK_ID,      "b2",     3);
    ts[i++] = makeToken(TK_CL,      ")",      3);
    ts[i++] = makeToken(TK_SEM,     ";",      3);
    ts[i++] = makeToken(TK_RETURN,  "return", 4);
    ts[i++] = makeToken(TK_SEM,     ";",      4);
    ts[i++] = makeToken(TK_END,     "end",    5);
    ts[i++] = makeToken(TK_EOF,     "$",      5);
    int count = i;

    ParseTreeNode *root = parse(ts, count);

    printf("\n── Parse Tree ──\n");
    printParseTree(root, 0);

    freeParseTree(root);
    freeTokens(ts, count);
    printf("\n");
}

/* ─────────────────────────────────────────────────────────────
 * TEST 4 – Parse a function + main with arithmetic
 *
 *  Equivalent source:
 *    _add input parameter list [int b2, int c3] output parameter list [int d4] ;
 *    d4 <--- b2 + c3 ;
 *    return [d4] ;
 *    end
 *    _main
 *    type int : b2 ;
 *    type int : c3 ;
 *    type int : d4 ;
 *    [d4] <--- call _add with parameters [b2, c3] ;
 *    write(d4) ;
 *    return ;
 *    end
 * ───────────────────────────────────────────────────────────── */
static void testParseFunctionAndMain(void) {
    printf("═══════════════════════════════════════════════════════\n");
    printf("  TEST 4 – Function + main with arithmetic\n");
    printf("═══════════════════════════════════════════════════════\n");

    Token **ts = (Token **)malloc(80 * sizeof(Token *));
    int i = 0;

    /* ── function _add ── */
    ts[i++] = makeToken(TK_FUNID,      "_add",       1);
    ts[i++] = makeToken(TK_INPUT,      "input",      1);
    ts[i++] = makeToken(TK_PARAMETER,  "parameter",  1);
    ts[i++] = makeToken(TK_LIST,       "list",       1);
    ts[i++] = makeToken(TK_SQL,        "[",          1);
    ts[i++] = makeToken(TK_INT,        "int",        1);
    ts[i++] = makeToken(TK_ID,         "b2",         1);
    ts[i++] = makeToken(TK_COMMA,      ",",          1);
    ts[i++] = makeToken(TK_INT,        "int",        1);
    ts[i++] = makeToken(TK_ID,         "c3",         1);  /* wait – use valid IDs */
    ts[i++] = makeToken(TK_SQR,        "]",          1);
    ts[i++] = makeToken(TK_OUTPUT,     "output",     1);
    ts[i++] = makeToken(TK_PARAMETER,  "parameter",  1);
    ts[i++] = makeToken(TK_LIST,       "list",       1);
    ts[i++] = makeToken(TK_SQL,        "[",          1);
    ts[i++] = makeToken(TK_INT,        "int",        1);
    ts[i++] = makeToken(TK_ID,         "d4",         1);
    ts[i++] = makeToken(TK_SQR,        "]",          1);
    ts[i++] = makeToken(TK_SEM,        ";",          1);
    /* body: d4 <--- b2 + c3 ; */
    ts[i++] = makeToken(TK_ID,         "d4",         2);
    ts[i++] = makeToken(TK_ASSIGNOP,   "<---",       2);
    ts[i++] = makeToken(TK_ID,         "b2",         2);
    ts[i++] = makeToken(TK_PLUS,       "+",          2);
    ts[i++] = makeToken(TK_ID,         "c3",         2);
    ts[i++] = makeToken(TK_SEM,        ";",          2);
    /* return [d4] ; */
    ts[i++] = makeToken(TK_RETURN,     "return",     3);
    ts[i++] = makeToken(TK_SQL,        "[",          3);
    ts[i++] = makeToken(TK_ID,         "d4",         3);
    ts[i++] = makeToken(TK_SQR,        "]",          3);
    ts[i++] = makeToken(TK_SEM,        ";",          3);
    ts[i++] = makeToken(TK_END,        "end",        4);

    /* ── _main ── */
    ts[i++] = makeToken(TK_MAIN,       "_main",      6);
    /* type int : b2 ; */
    ts[i++] = makeToken(TK_TYPE,       "type",       7);
    ts[i++] = makeToken(TK_INT,        "int",        7);
    ts[i++] = makeToken(TK_COLON,      ":",          7);
    ts[i++] = makeToken(TK_ID,         "b2",         7);
    ts[i++] = makeToken(TK_SEM,        ";",          7);
    /* type int : c3 ; */
    ts[i++] = makeToken(TK_TYPE,       "type",       8);
    ts[i++] = makeToken(TK_INT,        "int",        8);
    ts[i++] = makeToken(TK_COLON,      ":",          8);
    ts[i++] = makeToken(TK_ID,         "c3",         8);
    ts[i++] = makeToken(TK_SEM,        ";",          8);
    /* type int : d4 ; */
    ts[i++] = makeToken(TK_TYPE,       "type",       9);
    ts[i++] = makeToken(TK_INT,        "int",        9);
    ts[i++] = makeToken(TK_COLON,      ":",          9);
    ts[i++] = makeToken(TK_ID,         "d4",         9);
    ts[i++] = makeToken(TK_SEM,        ";",          9);
    /* [d4] <--- call _add with parameters [b2, c3] ; */
    ts[i++] = makeToken(TK_SQL,        "[",          10);
    ts[i++] = makeToken(TK_ID,         "d4",         10);
    ts[i++] = makeToken(TK_SQR,        "]",          10);
    ts[i++] = makeToken(TK_ASSIGNOP,   "<---",       10);
    ts[i++] = makeToken(TK_CALL,       "call",       10);
    ts[i++] = makeToken(TK_FUNID,      "_add",       10);
    ts[i++] = makeToken(TK_WITH,       "with",       10);
    ts[i++] = makeToken(TK_PARAMETERS, "parameters", 10);
    ts[i++] = makeToken(TK_SQL,        "[",          10);
    ts[i++] = makeToken(TK_ID,         "b2",         10);
    ts[i++] = makeToken(TK_COMMA,      ",",          10);
    ts[i++] = makeToken(TK_ID,         "c3",         10);
    ts[i++] = makeToken(TK_SQR,        "]",          10);
    ts[i++] = makeToken(TK_SEM,        ";",          10);
    /* write(d4) ; */
    ts[i++] = makeToken(TK_WRITE,      "write",      11);
    ts[i++] = makeToken(TK_OP,         "(",          11);
    ts[i++] = makeToken(TK_ID,         "d4",         11);
    ts[i++] = makeToken(TK_CL,         ")",          11);
    ts[i++] = makeToken(TK_SEM,        ";",          11);
    /* return ; end */
    ts[i++] = makeToken(TK_RETURN,     "return",     12);
    ts[i++] = makeToken(TK_SEM,        ";",          12);
    ts[i++] = makeToken(TK_END,        "end",        13);
    ts[i++] = makeToken(TK_EOF,        "$",          13);

    int count = i;
    ParseTreeNode *root = parse(ts, count);

    printf("\n── Parse Tree (abbreviated depth-5) ──\n");
    /* Print only first 5 levels to keep output readable */
    void printTree5(const ParseTreeNode *n, int d);
    printTree5(root, 0);

    freeParseTree(root);
    freeTokens(ts, count);
    printf("\n");
}

void printTree5(const ParseTreeNode *node, int d) {
    if (!node || d > 5) return;
    for (int i = 0; i < d * 3; i++) printf(" ");
    if (d > 0) printf("|--");
    if (node->symbol.type == SYM_NON_TERMINAL) {
        printf("<%s>", nonTerminalNames[node->symbol.value]);
        if (node->ruleApplied) printf(" [r%d]", node->ruleApplied);
    } else if (node->symbol.type == SYM_TERMINAL) {
        printf("%s \"%s\"", terminalNames[node->symbol.value], node->lexeme);
    } else {
        printf("ε");
    }
    printf("\n");
    for (int c = 0; c < node->numChildren; c++)
        printTree5(node->children[c], d + 1);
}

/* ─────────────────────────────────────────────────────────────
 * MAIN
 * ───────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    /* Initialise grammar rules and parse table */
    initGrammarRules();
    initParseTable();

    if (argc > 1 && strcmp(argv[1], "table") == 0) {
        printParseTable();
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "rules") == 0) {
        printf("─── Grammar Rules ───\n");
        for (int i = 0; i < NUM_RULES; i++) {
            GrammarRule *r = &grammarRules[i];
            printf("  G%-3d  <%s>  →  ", r->ruleNumber,
                   nonTerminalNames[r->lhs]);
            if (r->rhsLen == 0) {
                printf("ε");
            } else {
                for (int s = 0; s < r->rhsLen; s++) {
                    if (r->rhs[s].type == SYM_TERMINAL)
                        printf("%s ", terminalNames[r->rhs[s].value]);
                    else if (r->rhs[s].type == SYM_NON_TERMINAL)
                        printf("<%s> ", nonTerminalNames[r->rhs[s].value]);
                    else printf("ε ");
                }
            }
            printf("\n");
        }
        return 0;
    }

    /* Default: run all tests */
    testStackADT();
    testParseTable();
    testParseMinimal();
    testParseFunctionAndMain();

    return 0;
}
