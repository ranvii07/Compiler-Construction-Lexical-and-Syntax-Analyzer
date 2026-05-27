/*  ============================================================
 *  CS F363 – Compiler Construction | BITS Pilani 2026
 *  FILE : parser.c
 *  DESC : Full LL(1) predictive parser implementation.
 *
 *  Sections:
 *    A. Human-readable name tables (for error messages)
 *    B. Grammar rule table  (89 corrected productions)
 *    C. Parse table initialisation  M[NT][Terminal]
 *    D. Parse tree ADT
 *    E. LL(1) stack-based parser driver
 *  ============================================================ */

#include "parser.h"
#include "parserDef.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ════════════════════════════════════════════════════════════
 * A.  HUMAN-READABLE NAME TABLES
 * ════════════════════════════════════════════════════════════ */

const char *terminalNames[NUM_TERMINALS] = {
    "TK_ASSIGNOP",   /* 0  */
    "TK_FIELDID",    /* 1  */
    "TK_ID",         /* 2  */
    "TK_NUM",        /* 3  */
    "TK_RNUM",       /* 4  */
    "TK_FUNID",      /* 5  */
    "TK_RUID",       /* 6  */
    "TK_WITH",       /* 7  */
    "TK_PARAMETERS", /* 8  */
    "TK_END",        /* 9  */
    "TK_WHILE",      /* 10 */
    "TK_UNION",      /* 11 */
    "TK_ENDUNION",   /* 12 */
    "TK_DEFINETYPE", /* 13 */
    "TK_AS",         /* 14 */
    "TK_TYPE",       /* 15 */
    "TK_MAIN",       /* 16 */
    "TK_GLOBAL",     /* 17 */
    "TK_PARAMETER",  /* 18 */
    "TK_LIST",       /* 19 */
    "TK_SQL",        /* 20 */
    "TK_SQR",        /* 21 */
    "TK_INPUT",      /* 22 */
    "TK_OUTPUT",     /* 23 */
    "TK_INT",        /* 24 */
    "TK_REAL",       /* 25 */
    "TK_COMMA",      /* 26 */
    "TK_SEM",        /* 27 */
    "TK_COLON",      /* 28 */
    "TK_DOT",        /* 29 */
    "TK_ENDWHILE",   /* 30 */
    "TK_OP",         /* 31 */
    "TK_CL",         /* 32 */
    "TK_IF",         /* 33 */
    "TK_THEN",       /* 34 */
    "TK_ENDIF",      /* 35 */
    "TK_READ",       /* 36 */
    "TK_WRITE",      /* 37 */
    "TK_RETURN",     /* 38 */
    "TK_PLUS",       /* 39 */
    "TK_MINUS",      /* 40 */
    "TK_MUL",        /* 41 */
    "TK_DIV",        /* 42 */
    "TK_CALL",       /* 43 */
    "TK_RECORD",     /* 44 */
    "TK_ENDRECORD",  /* 45 */
    "TK_ELSE",       /* 46 */
    "TK_AND",        /* 47 */
    "TK_OR",         /* 48 */
    "TK_NOT",        /* 49 */
    "TK_LT",         /* 50 */
    "TK_LE",         /* 51 */
    "TK_EQ",         /* 52 */
    "TK_GT",         /* 53 */
    "TK_GE",         /* 54 */
    "TK_NE",         /* 55 */
    "TK_EOF"         /* 56 */
};

const char *nonTerminalNames[NUM_NON_TERMINALS] = {
    "program",              /* 0  */
    "mainFunction",         /* 1  */
    "otherFunctions",       /* 2  */
    "function",             /* 3  */
    "input_par",            /* 4  */
    "output_par",           /* 5  */
    "parameter_list",       /* 6  */
    "dataType",             /* 7  */
    "primitiveDatatype",    /* 8  */
    "constructedDatatype",  /* 9  */
    "remaining_list",       /* 10 */
    "stmts",                /* 11 */
    "typeDefinitions",      /* 12 */
    "typeDefinition",       /* 13 */
    "fieldDefinitions",     /* 14 */
    "fieldDefinition",      /* 15 */
    "fieldType",            /* 16 */
    "moreFields",           /* 17 */
    "declarations",         /* 18 */
    "declaration",          /* 19 */
    "global_or_not",        /* 20 */
    "otherStmts",           /* 21 */
    "stmt",                 /* 22 */
    "assignmentStmt",       /* 23 */
    "singleOrRecId",        /* 24 */
    "singleOrRecIdSuffix",  /* 25 */
    "funCallStmt",          /* 26 */
    "outputParameters",     /* 27 */
    "inputParameters",      /* 28 */
    "iterativeStmt",        /* 29 */
    "conditionalStmt",      /* 30 */
    "elsePart",             /* 31 */
    "ioStmt",               /* 32 */
    "arithmeticExpression", /* 33 */
    "arithExpPrime",        /* 34 */
    "term",                 /* 35 */
    "termPrime",            /* 36 */
    "factor",               /* 37 */
    "booleanExpression",    /* 38 */
    "var",                  /* 39 */
    "logicalOp",            /* 40 */
    "relationalOp",         /* 41 */
    "returnStmt",           /* 42 */
    "optionalReturn",       /* 43 */
    "idList",               /* 44 */
    "more_ids",             /* 45 */
    "definetypestmt",       /* 46 */
    "typeDefKw"             /* 47 */
};

/* ════════════════════════════════════════════════════════════
 * B.  GRAMMAR RULE TABLE
 *
 *  Each entry: { ruleNumber, LHS (NonTerminal),
 *                rhs[] array of Symbol, rhsLen }
 *
 *  Convention:
 *    T(token)   → terminal symbol
 *    NT(ntEnum) → non-terminal symbol
 *    EPS        → epsilon (rhsLen = 0 for such rules)
 *
 *  89 rules in total.
 * ════════════════════════════════════════════════════════════ */

GrammarRule grammarRules[NUM_RULES];

/* Helper macro to fill a rule entry.
 * _LHS is used instead of lhs to avoid name collision with struct field. */
#define RULE(n, _LHS, len, ...) do {                        \
    grammarRules[(n)-1].ruleNumber = (n);                   \
    grammarRules[(n)-1].lhs        = (_LHS);                \
    grammarRules[(n)-1].rhsLen     = (len);                 \
    if ((len) > 0) {                                        \
        Symbol _tmp[] = { __VA_ARGS__ };                    \
        memcpy(grammarRules[(n)-1].rhs, _tmp,               \
               (len) * sizeof(Symbol));                     \
    }                                                       \
} while(0)

void initGrammarRules(void) {
    memset(grammarRules, 0, sizeof(grammarRules));

    /* ── Structure ─────────────────────────────────────────── */
    /*  1 */ RULE( 1, NT_PROGRAM,              2, NT(NT_OTHER_FUNCTIONS), NT(NT_MAIN_FUNCTION));
    /*  2 */ RULE( 2, NT_MAIN_FUNCTION,        3, T(TK_MAIN), NT(NT_STMTS), T(TK_END));
    /*  3 */ RULE( 3, NT_OTHER_FUNCTIONS,      2, NT(NT_FUNCTION), NT(NT_OTHER_FUNCTIONS));
    /*  4 */ RULE( 4, NT_OTHER_FUNCTIONS,      0);   /* ε */
    /*  5 */ RULE( 5, NT_FUNCTION,             6, T(TK_FUNID), NT(NT_INPUT_PAR),
                                                       NT(NT_OUTPUT_PAR), T(TK_SEM),
                                                       NT(NT_STMTS), T(TK_END));
    /*  6 */ RULE( 6, NT_INPUT_PAR,            6, T(TK_INPUT), T(TK_PARAMETER), T(TK_LIST),
                                                       T(TK_SQL), NT(NT_PARAMETER_LIST), T(TK_SQR));
    /*  7 */ RULE( 7, NT_OUTPUT_PAR,           6, T(TK_OUTPUT), T(TK_PARAMETER), T(TK_LIST),
                                                       T(TK_SQL), NT(NT_PARAMETER_LIST), T(TK_SQR));
    /*  8 */ RULE( 8, NT_OUTPUT_PAR,           0);   /* ε */
    /*  9 */ RULE( 9, NT_PARAMETER_LIST,       3, NT(NT_DATA_TYPE), T(TK_ID), NT(NT_REMAINING_LIST));
    /* 10 */ RULE(10, NT_DATA_TYPE,            1, NT(NT_PRIMITIVE_DATATYPE));
    /* 11 */ RULE(11, NT_DATA_TYPE,            1, NT(NT_CONSTRUCTED_DATATYPE));

    /* ── Primitive & constructed types ─────────────────────── */
    /* 12 */ RULE(12, NT_PRIMITIVE_DATATYPE,   1, T(TK_INT));
    /* 13 */ RULE(13, NT_PRIMITIVE_DATATYPE,   1, T(TK_REAL));
    /* 14 */ RULE(14, NT_CONSTRUCTED_DATATYPE, 2, T(TK_RECORD), T(TK_RUID));
    /* 15 */ RULE(15, NT_CONSTRUCTED_DATATYPE, 2, T(TK_UNION),  T(TK_RUID));
    /* 16 */ RULE(16, NT_CONSTRUCTED_DATATYPE, 1, T(TK_RUID));          /* alias */
    /* 17 */ RULE(17, NT_REMAINING_LIST,       2, T(TK_COMMA), NT(NT_PARAMETER_LIST));
    /* 18 */ RULE(18, NT_REMAINING_LIST,       0);   /* ε */

    /* ── Statements block ───────────────────────────────────── */
    /* 19 */ RULE(19, NT_STMTS,               4, NT(NT_TYPE_DEFINITIONS), NT(NT_DECLARATIONS),
                                                       NT(NT_OTHER_STMTS), NT(NT_RETURN_STMT));
    /* 20 */ RULE(20, NT_TYPE_DEFINITIONS,    2, NT(NT_TYPE_DEFINITION), NT(NT_TYPE_DEFINITIONS));
    /* 21 */ RULE(21, NT_TYPE_DEFINITIONS,    0);   /* ε */
    /* 22 */ RULE(22, NT_TYPE_DEFINITION,     4, T(TK_RECORD), T(TK_RUID),
                                                       NT(NT_FIELD_DEFINITIONS), T(TK_ENDRECORD));
    /* 23 */ RULE(23, NT_TYPE_DEFINITION,     4, T(TK_UNION),  T(TK_RUID),
                                                       NT(NT_FIELD_DEFINITIONS), T(TK_ENDUNION));
    /* 24 */ RULE(24, NT_TYPE_DEFINITION,     1, NT(NT_DEFINETYPE_STMT));  /* NEW */

    /* ── Field definitions ──────────────────────────────────── */
    /* 25 */ RULE(25, NT_FIELD_DEFINITIONS,   3, NT(NT_FIELD_DEFINITION),
                                                       NT(NT_FIELD_DEFINITION), NT(NT_MORE_FIELDS));
    /* 26 */ RULE(26, NT_FIELD_DEFINITION,    5, T(TK_TYPE), NT(NT_FIELD_TYPE),
                                                       T(TK_COLON), T(TK_FIELDID), T(TK_SEM));
    /* 27 */ RULE(27, NT_FIELD_TYPE,          1, NT(NT_PRIMITIVE_DATATYPE));  /* NEW */
    /* 28 */ RULE(28, NT_FIELD_TYPE,          1, NT(NT_CONSTRUCTED_DATATYPE));/* NEW */
    /* 29 */ RULE(29, NT_MORE_FIELDS,         2, NT(NT_FIELD_DEFINITION), NT(NT_MORE_FIELDS));
    /* 30 */ RULE(30, NT_MORE_FIELDS,         0);   /* ε */

    /* ── Declarations ───────────────────────────────────────── */
    /* 31 */ RULE(31, NT_DECLARATIONS,        2, NT(NT_DECLARATION), NT(NT_DECLARATIONS));
    /* 32 */ RULE(32, NT_DECLARATIONS,        0);   /* ε */
    /* 33 */ RULE(33, NT_DECLARATION,         6, T(TK_TYPE), NT(NT_DATA_TYPE),
                                                       T(TK_COLON), T(TK_ID),
                                                       NT(NT_GLOBAL_OR_NOT), T(TK_SEM));
    /* 34 */ RULE(34, NT_GLOBAL_OR_NOT,       2, T(TK_COLON), T(TK_GLOBAL));  /* NEW */
    /* 35 */ RULE(35, NT_GLOBAL_OR_NOT,       0);   /* ε */

    /* ── Other statements ───────────────────────────────────── */
    /* 36 */ RULE(36, NT_OTHER_STMTS,         2, NT(NT_STMT), NT(NT_OTHER_STMTS));
    /* 37 */ RULE(37, NT_OTHER_STMTS,         0);   /* ε */
    /* 38 */ RULE(38, NT_STMT,                1, NT(NT_ASSIGNMENT_STMT));
    /* 39 */ RULE(39, NT_STMT,                1, NT(NT_ITERATIVE_STMT));
    /* 40 */ RULE(40, NT_STMT,                1, NT(NT_CONDITIONAL_STMT));
    /* 41 */ RULE(41, NT_STMT,                1, NT(NT_IO_STMT));
    /* 42 */ RULE(42, NT_STMT,                1, NT(NT_FUN_CALL_STMT));

    /* ── Assignment ─────────────────────────────────────────── */
    /* 43 */ RULE(43, NT_ASSIGNMENT_STMT,     4, NT(NT_SINGLE_OR_REC_ID), T(TK_ASSIGNOP),
                                                       NT(NT_ARITH_EXPR), T(TK_SEM));
    /* 44 */ RULE(44, NT_SINGLE_OR_REC_ID,   2, T(TK_ID), NT(NT_SOREC_SUFFIX));  /* NEW */
    /* 45 */ RULE(45, NT_SOREC_SUFFIX,        2, T(TK_DOT), T(TK_FIELDID));      /* NEW */
    /* 46 */ RULE(46, NT_SOREC_SUFFIX,        0);   /* ε – NEW */

    /* ── Function call ──────────────────────────────────────── */
    /* 47 */ RULE(47, NT_FUN_CALL_STMT,       7, NT(NT_OUTPUT_PARAMS), T(TK_CALL),
                                                       T(TK_FUNID), T(TK_WITH), T(TK_PARAMETERS),
                                                       NT(NT_INPUT_PARAMS), T(TK_SEM));
    /* 48 */ RULE(48, NT_OUTPUT_PARAMS,       4, T(TK_SQL), NT(NT_ID_LIST),
                                                       T(TK_SQR), T(TK_ASSIGNOP));
    /* 49 */ RULE(49, NT_OUTPUT_PARAMS,       0);   /* ε */
    /* 50 */ RULE(50, NT_INPUT_PARAMS,        3, T(TK_SQL), NT(NT_ID_LIST), T(TK_SQR));

    /* ── Iterative ──────────────────────────────────────────── */
    /* 51 */ RULE(51, NT_ITERATIVE_STMT,      7, T(TK_WHILE), T(TK_OP),
                                                       NT(NT_BOOL_EXPR), T(TK_CL),
                                                       NT(NT_STMT), NT(NT_OTHER_STMTS),
                                                       T(TK_ENDWHILE));

    /* ── Conditional (left-factored) ───────────────────────── */
    /* 52 */ RULE(52, NT_CONDITIONAL_STMT,    9, T(TK_IF), T(TK_OP), NT(NT_BOOL_EXPR),
                                                       T(TK_CL), T(TK_THEN),
                                                       NT(NT_STMT), NT(NT_OTHER_STMTS),
                                                       NT(NT_ELSE_PART), T(TK_ENDIF));
    /* 53 */ RULE(53, NT_ELSE_PART,           3, T(TK_ELSE), NT(NT_STMT), NT(NT_OTHER_STMTS)); /* NEW */
    /* 54 */ RULE(54, NT_ELSE_PART,           0);   /* ε – NEW */

    /* ── I/O ────────────────────────────────────────────────── */
    /* 55 */ RULE(55, NT_IO_STMT,             5, T(TK_READ),  T(TK_OP), NT(NT_VAR),
                                                       T(TK_CL), T(TK_SEM));
    /* 56 */ RULE(56, NT_IO_STMT,             5, T(TK_WRITE), T(TK_OP), NT(NT_VAR),
                                                       T(TK_CL), T(TK_SEM));

    /* ── Arithmetic (left-recursion eliminated) ─────────────── */
    /* 57 */ RULE(57, NT_ARITH_EXPR,          2, NT(NT_TERM), NT(NT_ARITH_EXPR_PRIME));   /* NEW */
    /* 58 */ RULE(58, NT_ARITH_EXPR_PRIME,    3, T(TK_PLUS),  NT(NT_TERM), NT(NT_ARITH_EXPR_PRIME)); /* NEW */
    /* 59 */ RULE(59, NT_ARITH_EXPR_PRIME,    3, T(TK_MINUS), NT(NT_TERM), NT(NT_ARITH_EXPR_PRIME)); /* NEW */
    /* 60 */ RULE(60, NT_ARITH_EXPR_PRIME,    0);   /* ε – NEW */
    /* 61 */ RULE(61, NT_TERM,                2, NT(NT_FACTOR), NT(NT_TERM_PRIME));       /* NEW */
    /* 62 */ RULE(62, NT_TERM_PRIME,          3, T(TK_MUL), NT(NT_FACTOR), NT(NT_TERM_PRIME)); /* NEW */
    /* 63 */ RULE(63, NT_TERM_PRIME,          3, T(TK_DIV), NT(NT_FACTOR), NT(NT_TERM_PRIME)); /* NEW */
    /* 64 */ RULE(64, NT_TERM_PRIME,          0);   /* ε – NEW */
    /* 65 */ RULE(65, NT_FACTOR,              3, T(TK_OP), NT(NT_ARITH_EXPR), T(TK_CL));
    /* 66 */ RULE(66, NT_FACTOR,              1, NT(NT_VAR));

    /* ── var ────────────────────────────────────────────────── */
    /* 67 */ RULE(67, NT_VAR, 1, T(TK_ID));
    /* 68 */ RULE(68, NT_VAR, 1, T(TK_NUM));
    /* 69 */ RULE(69, NT_VAR, 1, T(TK_RNUM));

    /* ── Boolean expression ─────────────────────────────────── */
    /* 70 */ RULE(70, NT_BOOL_EXPR, 7, T(TK_OP), NT(NT_BOOL_EXPR), T(TK_CL),
                                          NT(NT_LOGICAL_OP),
                                          T(TK_OP), NT(NT_BOOL_EXPR), T(TK_CL));
    /* 71 */ RULE(71, NT_BOOL_EXPR, 3, NT(NT_VAR), NT(NT_RELATIONAL_OP), NT(NT_VAR));
    /* 72 */ RULE(72, NT_BOOL_EXPR, 4, T(TK_NOT), T(TK_OP), NT(NT_BOOL_EXPR), T(TK_CL));

    /* ── Logical & relational operators ─────────────────────── */
    /* 73 */ RULE(73, NT_LOGICAL_OP,   1, T(TK_AND));
    /* 74 */ RULE(74, NT_LOGICAL_OP,   1, T(TK_OR));
    /* 75 */ RULE(75, NT_RELATIONAL_OP,1, T(TK_LT));
    /* 76 */ RULE(76, NT_RELATIONAL_OP,1, T(TK_LE));
    /* 77 */ RULE(77, NT_RELATIONAL_OP,1, T(TK_EQ));
    /* 78 */ RULE(78, NT_RELATIONAL_OP,1, T(TK_GT));
    /* 79 */ RULE(79, NT_RELATIONAL_OP,1, T(TK_GE));
    /* 80 */ RULE(80, NT_RELATIONAL_OP,1, T(TK_NE));

    /* ── Return ─────────────────────────────────────────────── */
    /* 81 */ RULE(81, NT_RETURN_STMT,    3, T(TK_RETURN), NT(NT_OPTIONAL_RETURN), T(TK_SEM));
    /* 82 */ RULE(82, NT_OPTIONAL_RETURN,3, T(TK_SQL), NT(NT_ID_LIST), T(TK_SQR));
    /* 83 */ RULE(83, NT_OPTIONAL_RETURN,0);  /* ε */

    /* ── ID lists ───────────────────────────────────────────── */
    /* 84 */ RULE(84, NT_ID_LIST, 2, T(TK_ID), NT(NT_MORE_IDS));
    /* 85 */ RULE(85, NT_MORE_IDS,2, T(TK_COMMA), NT(NT_ID_LIST));
    /* 86 */ RULE(86, NT_MORE_IDS,0);  /* ε */

    /* ── definetype ─────────────────────────────────────────── */
    /* 87 */ RULE(87, NT_DEFINETYPE_STMT, 5, T(TK_DEFINETYPE), NT(NT_TYPE_DEF_KW),
                                                T(TK_RUID), T(TK_AS), T(TK_RUID));
    /* 88 */ RULE(88, NT_TYPE_DEF_KW, 1, T(TK_RECORD));
    /* 89 */ RULE(89, NT_TYPE_DEF_KW, 1, T(TK_UNION));
}

/* ════════════════════════════════════════════════════════════
 * C.  PREDICTIVE PARSE TABLE   M[NT][Terminal]
 *
 *  All cells initialised to PARSE_ERROR (-1).
 *  Then individual entries are filled from FIRST/FOLLOW analysis.
 *  Rule 0 (epsilon) is represented as 0 in the table.
 *  Positive integers are actual rule numbers (1-89).
 * ════════════════════════════════════════════════════════════ */

int parseTable[NUM_NON_TERMINALS][NUM_TERMINALS];

/* Shorthand for setting a table cell */
#define SET(nt, tok, rule) parseTable[(nt)][(tok)] = (rule)

void initParseTable(void) {
    /* Fill all cells with error */
    for (int i = 0; i < NUM_NON_TERMINALS; i++)
        for (int j = 0; j < NUM_TERMINALS; j++)
            parseTable[i][j] = PARSE_ERROR;

    /* ── <program> ───────────────────────────────────────────── */
    SET(NT_PROGRAM,         TK_FUNID, 1);
    SET(NT_PROGRAM,         TK_MAIN,  1);

    /* ── <mainFunction> ─────────────────────────────────────── */
    SET(NT_MAIN_FUNCTION,   TK_MAIN,  2);

    /* ── <otherFunctions> ───────────────────────────────────── */
    SET(NT_OTHER_FUNCTIONS, TK_FUNID, 3);
    SET(NT_OTHER_FUNCTIONS, TK_MAIN,  4);   /* ε */

    /* ── <function> ─────────────────────────────────────────── */
    SET(NT_FUNCTION,        TK_FUNID, 5);

    /* ── <input_par> ────────────────────────────────────────── */
    SET(NT_INPUT_PAR,       TK_INPUT,  6);

    /* ── <output_par> ───────────────────────────────────────── */
    SET(NT_OUTPUT_PAR,      TK_OUTPUT, 7);
    SET(NT_OUTPUT_PAR,      TK_SEM,    8);  /* ε */

    /* ── <parameter_list> ───────────────────────────────────── */
    SET(NT_PARAMETER_LIST,  TK_INT,    9);
    SET(NT_PARAMETER_LIST,  TK_REAL,   9);
    SET(NT_PARAMETER_LIST,  TK_RECORD, 9);
    SET(NT_PARAMETER_LIST,  TK_UNION,  9);
    SET(NT_PARAMETER_LIST,  TK_RUID,   9);

    /* ── <dataType> ─────────────────────────────────────────── */
    SET(NT_DATA_TYPE,  TK_INT,    10);
    SET(NT_DATA_TYPE,  TK_REAL,   10);
    SET(NT_DATA_TYPE,  TK_RECORD, 11);
    SET(NT_DATA_TYPE,  TK_UNION,  11);
    SET(NT_DATA_TYPE,  TK_RUID,   11);

    /* ── <primitiveDatatype> ────────────────────────────────── */
    SET(NT_PRIMITIVE_DATATYPE,   TK_INT,  12);
    SET(NT_PRIMITIVE_DATATYPE,   TK_REAL, 13);

    /* ── <constructedDatatype> ──────────────────────────────── */
    SET(NT_CONSTRUCTED_DATATYPE, TK_RECORD, 14);
    SET(NT_CONSTRUCTED_DATATYPE, TK_UNION,  15);
    SET(NT_CONSTRUCTED_DATATYPE, TK_RUID,   16);

    /* ── <remaining_list> ───────────────────────────────────── */
    SET(NT_REMAINING_LIST, TK_COMMA, 17);
    SET(NT_REMAINING_LIST, TK_SQR,   18);  /* ε */

    /* ── <stmts>  (all tokens in FIRST(stmts)) ──────────────── */
    {
        Terminal stmtFirst[] = {
            TK_RECORD, TK_UNION, TK_DEFINETYPE, TK_TYPE,
            TK_ID, TK_WHILE, TK_IF, TK_READ, TK_WRITE,
            TK_SQL, TK_CALL, TK_RETURN
        };
        for (int i = 0; i < 12; i++)
            SET(NT_STMTS, stmtFirst[i], 19);
    }

    /* ── <typeDefinitions> ──────────────────────────────────── */
    SET(NT_TYPE_DEFINITIONS, TK_RECORD,     20);
    SET(NT_TYPE_DEFINITIONS, TK_UNION,      20);
    SET(NT_TYPE_DEFINITIONS, TK_DEFINETYPE, 20);
    /* ε via FOLLOW: TYPE, ID, WHILE, IF, READ, WRITE, SQL, CALL, RETURN */
    {
        Terminal epsToks[] = {
            TK_TYPE, TK_ID, TK_WHILE, TK_IF,
            TK_READ, TK_WRITE, TK_SQL, TK_CALL, TK_RETURN
        };
        for (int i = 0; i < 9; i++)
            SET(NT_TYPE_DEFINITIONS, epsToks[i], 21);
    }

    /* ── <typeDefinition> ───────────────────────────────────── */
    SET(NT_TYPE_DEFINITION,  TK_RECORD,     22);
    SET(NT_TYPE_DEFINITION,  TK_UNION,      23);
    SET(NT_TYPE_DEFINITION,  TK_DEFINETYPE, 24);

    /* ── <fieldDefinitions> ─────────────────────────────────── */
    SET(NT_FIELD_DEFINITIONS, TK_TYPE, 25);

    /* ── <fieldDefinition> ──────────────────────────────────── */
    SET(NT_FIELD_DEFINITION, TK_TYPE, 26);

    /* ── <fieldType> ────────────────────────────────────────── */
    SET(NT_FIELD_TYPE, TK_INT,    27);
    SET(NT_FIELD_TYPE, TK_REAL,   27);
    SET(NT_FIELD_TYPE, TK_RECORD, 28);
    SET(NT_FIELD_TYPE, TK_UNION,  28);
    SET(NT_FIELD_TYPE, TK_RUID,   28);

    /* ── <moreFields> ───────────────────────────────────────── */
    SET(NT_MORE_FIELDS, TK_TYPE,      29);
    SET(NT_MORE_FIELDS, TK_ENDRECORD, 30);  /* ε */
    SET(NT_MORE_FIELDS, TK_ENDUNION,  30);  /* ε */

    /* ── <declarations> ─────────────────────────────────────── */
    SET(NT_DECLARATIONS, TK_TYPE, 31);
    {
        Terminal epsToks[] = {
            TK_ID, TK_WHILE, TK_IF, TK_READ,
            TK_WRITE, TK_SQL, TK_CALL, TK_RETURN
        };
        for (int i = 0; i < 8; i++)
            SET(NT_DECLARATIONS, epsToks[i], 32);
    }

    /* ── <declaration> ──────────────────────────────────────── */
    SET(NT_DECLARATION, TK_TYPE, 33);

    /* ── <global_or_not> ────────────────────────────────────── */
    SET(NT_GLOBAL_OR_NOT, TK_COLON, 34);
    SET(NT_GLOBAL_OR_NOT, TK_SEM,   35);  /* ε */

    /* ── <otherStmts> ───────────────────────────────────────── */
    {
        Terminal stmtStarters[] = {
            TK_ID, TK_WHILE, TK_IF, TK_READ, TK_WRITE, TK_SQL, TK_CALL
        };
        for (int i = 0; i < 7; i++)
            SET(NT_OTHER_STMTS, stmtStarters[i], 36);
    }
    {
        Terminal epsToks[] = {
            TK_RETURN, TK_ENDWHILE, TK_ELSE, TK_ENDIF
        };
        for (int i = 0; i < 4; i++)
            SET(NT_OTHER_STMTS, epsToks[i], 37);
    }

    /* ── <stmt> ──────────────────────────────────────────────── */
    SET(NT_STMT, TK_ID,    38);
    SET(NT_STMT, TK_WHILE, 39);
    SET(NT_STMT, TK_IF,    40);
    SET(NT_STMT, TK_READ,  41);
    SET(NT_STMT, TK_WRITE, 41);
    SET(NT_STMT, TK_SQL,   42);
    SET(NT_STMT, TK_CALL,  42);

    /* ── <assignmentStmt> ───────────────────────────────────── */
    SET(NT_ASSIGNMENT_STMT, TK_ID, 43);

    /* ── <singleOrRecId> ────────────────────────────────────── */
    SET(NT_SINGLE_OR_REC_ID, TK_ID, 44);

    /* ── <singleOrRecIdSuffix> ──────────────────────────────── */
    SET(NT_SOREC_SUFFIX, TK_DOT,      45);
    SET(NT_SOREC_SUFFIX, TK_ASSIGNOP, 46);  /* ε */

    /* ── <funCallStmt> ──────────────────────────────────────── */
    SET(NT_FUN_CALL_STMT, TK_SQL,  47);
    SET(NT_FUN_CALL_STMT, TK_CALL, 47);

    /* ── <outputParameters> ─────────────────────────────────── */
    SET(NT_OUTPUT_PARAMS, TK_SQL,  48);
    SET(NT_OUTPUT_PARAMS, TK_CALL, 49);  /* ε */

    /* ── <inputParameters> ──────────────────────────────────── */
    SET(NT_INPUT_PARAMS, TK_SQL, 50);

    /* ── <iterativeStmt> ────────────────────────────────────── */
    SET(NT_ITERATIVE_STMT, TK_WHILE, 51);

    /* ── <conditionalStmt> ──────────────────────────────────── */
    SET(NT_CONDITIONAL_STMT, TK_IF, 52);

    /* ── <elsePart> ─────────────────────────────────────────── */
    SET(NT_ELSE_PART, TK_ELSE,  53);
    SET(NT_ELSE_PART, TK_ENDIF, 54);  /* ε */

    /* ── <ioStmt> ────────────────────────────────────────────── */
    SET(NT_IO_STMT, TK_READ,  55);
    SET(NT_IO_STMT, TK_WRITE, 56);

    /* ── <arithmeticExpression> ─────────────────────────────── */
    {
        Terminal ae[] = { TK_OP, TK_ID, TK_NUM, TK_RNUM };
        for (int i = 0; i < 4; i++) SET(NT_ARITH_EXPR, ae[i], 57);
    }

    /* ── <arithExpPrime> ────────────────────────────────────── */
    SET(NT_ARITH_EXPR_PRIME, TK_PLUS,  58);
    SET(NT_ARITH_EXPR_PRIME, TK_MINUS, 59);
    SET(NT_ARITH_EXPR_PRIME, TK_SEM,   60);  /* ε */
    SET(NT_ARITH_EXPR_PRIME, TK_CL,    60);  /* ε */

    /* ── <term> ──────────────────────────────────────────────── */
    {
        Terminal t[] = { TK_OP, TK_ID, TK_NUM, TK_RNUM };
        for (int i = 0; i < 4; i++) SET(NT_TERM, t[i], 61);
    }

    /* ── <termPrime> ────────────────────────────────────────── */
    SET(NT_TERM_PRIME, TK_MUL,   62);
    SET(NT_TERM_PRIME, TK_DIV,   63);
    SET(NT_TERM_PRIME, TK_PLUS,  64);  /* ε */
    SET(NT_TERM_PRIME, TK_MINUS, 64);  /* ε */
    SET(NT_TERM_PRIME, TK_SEM,   64);  /* ε */
    SET(NT_TERM_PRIME, TK_CL,    64);  /* ε */

    /* ── <factor> ────────────────────────────────────────────── */
    SET(NT_FACTOR, TK_OP,   65);
    SET(NT_FACTOR, TK_ID,   66);
    SET(NT_FACTOR, TK_NUM,  66);
    SET(NT_FACTOR, TK_RNUM, 66);

    /* ── <booleanExpression> ────────────────────────────────── */
    SET(NT_BOOL_EXPR, TK_OP,   70);
    SET(NT_BOOL_EXPR, TK_ID,   71);
    SET(NT_BOOL_EXPR, TK_NUM,  71);
    SET(NT_BOOL_EXPR, TK_RNUM, 71);
    SET(NT_BOOL_EXPR, TK_NOT,  72);

    /* ── <var> ───────────────────────────────────────────────── */
    SET(NT_VAR, TK_ID,   67);
    SET(NT_VAR, TK_NUM,  68);
    SET(NT_VAR, TK_RNUM, 69);

    /* ── <logicalOp> ────────────────────────────────────────── */
    SET(NT_LOGICAL_OP,   TK_AND, 73);
    SET(NT_LOGICAL_OP,   TK_OR,  74);

    /* ── <relationalOp> ─────────────────────────────────────── */
    SET(NT_RELATIONAL_OP, TK_LT, 75);
    SET(NT_RELATIONAL_OP, TK_LE, 76);
    SET(NT_RELATIONAL_OP, TK_EQ, 77);
    SET(NT_RELATIONAL_OP, TK_GT, 78);
    SET(NT_RELATIONAL_OP, TK_GE, 79);
    SET(NT_RELATIONAL_OP, TK_NE, 80);

    /* ── <returnStmt> ───────────────────────────────────────── */
    SET(NT_RETURN_STMT, TK_RETURN, 81);

    /* ── <optionalReturn> ───────────────────────────────────── */
    SET(NT_OPTIONAL_RETURN, TK_SQL, 82);
    SET(NT_OPTIONAL_RETURN, TK_SEM, 83);  /* ε */

    /* ── <idList> ────────────────────────────────────────────── */
    SET(NT_ID_LIST,  TK_ID,    84);

    /* ── <more_ids> ─────────────────────────────────────────── */
    SET(NT_MORE_IDS, TK_COMMA, 85);
    SET(NT_MORE_IDS, TK_SQR,   86);  /* ε */

    /* ── <definetypestmt> ───────────────────────────────────── */
    SET(NT_DEFINETYPE_STMT, TK_DEFINETYPE, 87);

    /* ── <typeDefKw> ────────────────────────────────────────── */
    SET(NT_TYPE_DEF_KW, TK_RECORD, 88);
    SET(NT_TYPE_DEF_KW, TK_UNION,  89);
}

/* ════════════════════════════════════════════════════════════
 * D.  PARSE TREE ADT
 * ════════════════════════════════════════════════════════════ */

ParseTreeNode *newParseTreeNode(Symbol sym) {
    ParseTreeNode *node = (ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
    if (!node) {
        fprintf(stderr, "[TREE] Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    node->symbol      = sym;
    node->numChildren = 0;
    node->parent      = NULL;
    node->ruleApplied = 0;
    node->lineNumber  = -1;
    node->lexeme[0]   = '\0';
    return node;
}

void addChild(ParseTreeNode *parent, ParseTreeNode *child) {
    if (parent->numChildren >= MAX_CHILDREN) {
        fprintf(stderr, "[TREE] Max children exceeded for node <%s>.\n",
                nonTerminalNames[parent->symbol.value]);
        return;
    }
    parent->children[parent->numChildren++] = child;
    child->parent = parent;
}

/* Recursive pretty-printer */
void printParseTree(const ParseTreeNode *node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth * 3; i++) printf(" ");
    if (depth > 0) printf("|--");

    if (node->symbol.type == SYM_NON_TERMINAL) {
        printf("<%s>", nonTerminalNames[node->symbol.value]);
        if (node->ruleApplied > 0)
            printf("  [rule %d]", node->ruleApplied);
    } else if (node->symbol.type == SYM_TERMINAL) {
        printf("%s", terminalNames[node->symbol.value]);
        if (node->lexeme[0] != '\0')
            printf("  \"%s\"", node->lexeme);
        if (node->lineNumber > 0)
            printf("  (line %d)", node->lineNumber);
    } else {
        printf("ε");
    }
    printf("\n");

    for (int c = 0; c < node->numChildren; c++)
        printParseTree(node->children[c], depth + 1);
}

void freeParseTree(ParseTreeNode *node) {
    if (!node) return;
    for (int c = 0; c < node->numChildren; c++)
        freeParseTree(node->children[c]);
    free(node);
}

/* ════════════════════════════════════════════════════════════
 * E.  LL(1) STACK-BASED PARSER DRIVER
 * ════════════════════════════════════════════════════════════ */

int getParseTableEntry(NonTerminal nt, Terminal t) {
    return parseTable[nt][t];
}

/*  Error recovery: panic-mode — skip tokens until a synchronisation
 *  token (e.g., ';' or 'end') is found.                             */
static int panicRecover(Token **stream, int *pos, int total, NonTerminal nt) {
    /* Sync set: FOLLOW of the current NT  */
    /* For simplicity we sync on ; , end , endif , endwhile */
    Terminal syncSet[] = { TK_SEM, TK_END, TK_ENDIF, TK_ENDWHILE, TK_EOF };
    int syncLen = 5;

    fprintf(stderr, "    [RECOVERY] Skipping tokens: ");
    while (*pos < total) {
        Terminal cur = stream[*pos]->type;
        for (int s = 0; s < syncLen; s++) {
            if (cur == syncSet[s]) {
                fprintf(stderr, "\n    [RECOVERY] Stopped at %s\n",
                        terminalNames[cur]);
                return 0;
            }
        }
        fprintf(stderr, "%s ", terminalNames[cur]);
        (*pos)++;
    }
    fprintf(stderr, "\n");
    return -1;
}

/*  Main parser function
 *    tokenStream : array of Token* (from lexer), last entry has type TK_EOF
 *    tokenCount  : number of tokens INCLUDING the TK_EOF sentinel
 *    Returns     : root of parse tree (caller must free), or NULL on fatal error
 */
ParseTreeNode *parse(Token **tokenStream, int tokenCount) {
    if (!tokenStream || tokenCount < 1) return NULL;

    Stack     stack;
    stackInit(&stack);

    int errorCount = 0;
    int pos        = 0;   /* current index into tokenStream */

    /* ─── Push $ (EOF sentinel) ── */
    Symbol dollarSym = { SYM_TERMINAL, TK_EOF };
    stackPush(&stack, dollarSym, NULL);

    /* ─── Push start symbol: <program> ── */
    Symbol startSym = { SYM_NON_TERMINAL, NT_PROGRAM };
    ParseTreeNode *root = newParseTreeNode(startSym);
    stackPush(&stack, startSym, root);

    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║      CS F363 – LL(1) Parser  |  BITS Pilani 2026     ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    /* ─── Main parsing loop ── */
    while (!stackIsEmpty(&stack)) {
        StackEntry *top  = stackTop(&stack);
        Token      *curr = tokenStream[pos];

        /* ── ACCEPT condition ── */
        if (top->symbol.type   == SYM_TERMINAL &&
            top->symbol.value  == TK_EOF       &&
            curr->type         == TK_EOF) {
            stackPop(&stack);
            printf("[PARSER] ✓ Input accepted successfully. Errors: %d\n\n",
                   errorCount);
            break;
        }

        /* ── Epsilon node: just pop silently ──
         * The epsilon node was already added to the parse tree
         * during the NT expansion step.                          */
        if (top->symbol.type == SYM_EPSILON) {
            stackPop(&stack);
            continue;
        }

        /* ── Terminal on stack ── */
        if (top->symbol.type == SYM_TERMINAL) {
            Terminal expected = (Terminal)top->symbol.value;

            if (expected == curr->type) {
                /* Match! Fill in the tree node */
                if (top->treeNode) {
                    strncpy(top->treeNode->lexeme,
                            curr->lexeme, MAX_LEXEME - 1);
                    top->treeNode->lineNumber = curr->lineNumber;
                }
                printf("[MATCH]  %-20s  lexeme=%-15s  line=%d\n",
                       terminalNames[expected],
                       curr->lexeme,
                       curr->lineNumber);
                stackPop(&stack);
                if (pos < tokenCount - 1) pos++;
            } else {
                errorCount++;
                fprintf(stderr,
                    "[ERROR]  Line %d: Expected %-20s got %-20s (\"%s\")\n",
                    curr->lineNumber,
                    terminalNames[expected],
                    terminalNames[curr->type],
                    curr->lexeme);
                /* Skip the terminal silently (insertion/deletion) */
                stackPop(&stack);
            }
            continue;
        }

        /* ── Non-terminal on stack ── */
        NonTerminal   nt        = (NonTerminal)top->symbol.value;
        Terminal      lookahead = curr->type;
        int           rule      = getParseTableEntry(nt, lookahead);
        ParseTreeNode *ntNode   = top->treeNode;

        if (rule == PARSE_ERROR) {
            errorCount++;
            fprintf(stderr,
                "[ERROR]  Line %d: No rule for <%s> on token %-20s (\"%s\")\n",
                curr->lineNumber,
                nonTerminalNames[nt],
                terminalNames[lookahead],
                curr->lexeme);
            stackPop(&stack);   /* discard NT, try to continue */
            panicRecover(tokenStream, &pos, tokenCount, nt);
            continue;
        }

        /* Valid rule found — pop NT, push RHS in REVERSE order */
        GrammarRule *r = &grammarRules[rule - 1];
        if (ntNode) ntNode->ruleApplied = rule;

        stackPop(&stack);

        if (r->rhsLen == 0) {
            /* Epsilon production: push the ε symbol */
            Symbol epsSym = { SYM_EPSILON, 0 };
            ParseTreeNode *epsNode = NULL;
            if (ntNode) {
                epsNode = newParseTreeNode(epsSym);
                addChild(ntNode, epsNode);
            }
            stackPush(&stack, epsSym, epsNode);

            printf("[EXPAND] %-24s → ε            (rule %d)\n",
                   nonTerminalNames[nt], rule);
        } else {
            /* Push RHS in reverse so leftmost symbol is on top */
            ParseTreeNode *childNodes[MAX_RHS];
            for (int i = 0; i < r->rhsLen; i++) {
                childNodes[i] = ntNode ? newParseTreeNode(r->rhs[i]) : NULL;
                if (ntNode) addChild(ntNode, childNodes[i]);
            }
            for (int i = r->rhsLen - 1; i >= 0; i--)
                stackPush(&stack, r->rhs[i], childNodes[i]);

            /* Print expansion (abbreviated) */
            printf("[EXPAND] %-24s → ", nonTerminalNames[nt]);
            for (int i = 0; i < r->rhsLen; i++) {
                if (r->rhs[i].type == SYM_TERMINAL)
                    printf("%s ", terminalNames[r->rhs[i].value]);
                else
                    printf("<%s> ", nonTerminalNames[r->rhs[i].value]);
            }
            printf("  (rule %d)\n", rule);
        }
    }

    if (errorCount > 0)
        fprintf(stderr,
            "\n[PARSER] Parsing completed with %d syntax error(s).\n",
            errorCount);

    return root;
}

/* ════════════════════════════════════════════════════════════
 * F.  PARSE TABLE PRINTER  (diagnostic / verification)
 * ════════════════════════════════════════════════════════════ */

void printParseTable(void) {
    printf("\n── PREDICTIVE PARSE TABLE  M[NT][Terminal] ──\n");
    printf("   Rule 0 = ε production shown as 'eps'\n\n");

    /* Print only filled cells to keep output manageable */
    for (int nt = 0; nt < NUM_NON_TERMINALS; nt++) {
        printf("  %-26s :", nonTerminalNames[nt]);
        int any = 0;
        for (int t = 0; t < NUM_TERMINALS; t++) {
            int v = parseTable[nt][t];
            if (v != PARSE_ERROR) {
                printf("  %s→%d", terminalNames[t], v);
                any = 1;
            }
        }
        if (!any) printf("  (all error)");
        printf("\n");
    }
    printf("\n");
}

/* ════════════════════════════════════════════════════════════
 * G.  parseFromFile  –  stub / integration point
 *
 *  Integrates with the lexical analyser already written.
 *  Replace getNextToken() and the token array build-up below
 *  with calls to your actual lexer.
 * ════════════════════════════════════════════════════════════ */

/* Extern declaration – provided by the lexer module */
/* extern Token *getNextToken(FILE *src); */

ParseTreeNode *parseFromFile(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[PARSER] Cannot open file: %s\n", filename);
        return NULL;
    }

    /* ── Build token array using the lexer ── */
    /*
     * Uncomment and adapt to your lexer's API:
     *
     *   int   capacity = 1024, count = 0;
     *   Token **tokens = malloc(capacity * sizeof(Token*));
     *   Token *tok;
     *   while ((tok = getNextToken(fp))->type != TK_EOF) {
     *       if (count >= capacity - 1) {
     *           capacity *= 2;
     *           tokens = realloc(tokens, capacity * sizeof(Token*));
     *       }
     *       tokens[count++] = tok;
     *   }
     *   tokens[count++] = tok; // TK_EOF sentinel
     *
     *   ParseTreeNode *root = parse(tokens, count);
     *   for (int i = 0; i < count; i++) free(tokens[i]);
     *   free(tokens);
     *   fclose(fp);
     *   return root;
     */

    fclose(fp);
    fprintf(stderr, "[PARSER] parseFromFile: lexer integration needed.\n");
    return NULL;
}
