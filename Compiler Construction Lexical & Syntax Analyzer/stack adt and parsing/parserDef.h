/*  ============================================================
 *  CS F363 – Compiler Construction | BITS Pilani 2026
 *  FILE : parserDef.h
 *  DESC : Core type definitions shared by the lexer, stack ADT
 *         and the LL(1) predictive parser.
 *  ============================================================ */

#ifndef PARSER_DEF_H
#define PARSER_DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────
 * 1.  TERMINAL TOKENS
 *     Enum value doubles as the column index in the parse table.
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    TK_ASSIGNOP   =  0,   /* <---                           */
    TK_FIELDID    =  1,   /* [a-z]+                         */
    TK_ID         =  2,   /* [b-d][2-7][b-d]*[2-7]*         */
    TK_NUM        =  3,   /* [0-9]+                         */
    TK_RNUM       =  4,   /* [0-9]+.[0-9][0-9] (& exponent) */
    TK_FUNID      =  5,   /* _[a-zA-Z]+[0-9]*               */
    TK_RUID       =  6,   /* #[a-z]+                        */
    TK_WITH       =  7,   /* with                           */
    TK_PARAMETERS =  8,   /* parameters                     */
    TK_END        =  9,   /* end                            */
    TK_WHILE      = 10,   /* while                          */
    TK_UNION      = 11,   /* union                          */
    TK_ENDUNION   = 12,   /* endunion                       */
    TK_DEFINETYPE = 13,   /* definetype                     */
    TK_AS         = 14,   /* as                             */
    TK_TYPE       = 15,   /* type                           */
    TK_MAIN       = 16,   /* _main                          */
    TK_GLOBAL     = 17,   /* global                         */
    TK_PARAMETER  = 18,   /* parameter                      */
    TK_LIST       = 19,   /* list                           */
    TK_SQL        = 20,   /* [                              */
    TK_SQR        = 21,   /* ]                              */
    TK_INPUT      = 22,   /* input                          */
    TK_OUTPUT     = 23,   /* output                         */
    TK_INT        = 24,   /* int                            */
    TK_REAL       = 25,   /* real                           */
    TK_COMMA      = 26,   /* ,                              */
    TK_SEM        = 27,   /* ;                              */
    TK_COLON      = 28,   /* :                              */
    TK_DOT        = 29,   /* .                              */
    TK_ENDWHILE   = 30,   /* endwhile                       */
    TK_OP         = 31,   /* (                              */
    TK_CL         = 32,   /* )                              */
    TK_IF         = 33,   /* if                             */
    TK_THEN       = 34,   /* then                           */
    TK_ENDIF      = 35,   /* endif                          */
    TK_READ       = 36,   /* read                           */
    TK_WRITE      = 37,   /* write                          */
    TK_RETURN     = 38,   /* return                         */
    TK_PLUS       = 39,   /* +                              */
    TK_MINUS      = 40,   /* -                              */
    TK_MUL        = 41,   /* *                              */
    TK_DIV        = 42,   /* /                              */
    TK_CALL       = 43,   /* call                           */
    TK_RECORD     = 44,   /* record                         */
    TK_ENDRECORD  = 45,   /* endrecord                      */
    TK_ELSE       = 46,   /* else                           */
    TK_AND        = 47,   /* &&&                            */
    TK_OR         = 48,   /* @@@                            */
    TK_NOT        = 49,   /* ~                              */
    TK_LT         = 50,   /* <                              */
    TK_LE         = 51,   /* <=                             */
    TK_EQ         = 52,   /* ==                             */
    TK_GT         = 53,   /* >                              */
    TK_GE         = 54,   /* >=                             */
    TK_NE         = 55,   /* !=                             */
    TK_EOF        = 56,   /* $ – end of input               */
    NUM_TERMINALS = 57
} Terminal;

/* ─────────────────────────────────────────────────────────────
 * 2.  NON-TERMINALS
 *     Enum value doubles as the row index in the parse table.
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    NT_PROGRAM              =  0,
    NT_MAIN_FUNCTION        =  1,
    NT_OTHER_FUNCTIONS      =  2,
    NT_FUNCTION             =  3,
    NT_INPUT_PAR            =  4,
    NT_OUTPUT_PAR           =  5,
    NT_PARAMETER_LIST       =  6,
    NT_DATA_TYPE            =  7,
    NT_PRIMITIVE_DATATYPE   =  8,
    NT_CONSTRUCTED_DATATYPE =  9,
    NT_REMAINING_LIST       = 10,
    NT_STMTS                = 11,
    NT_TYPE_DEFINITIONS     = 12,
    NT_TYPE_DEFINITION      = 13,
    NT_FIELD_DEFINITIONS    = 14,
    NT_FIELD_DEFINITION     = 15,
    NT_FIELD_TYPE           = 16,
    NT_MORE_FIELDS          = 17,
    NT_DECLARATIONS         = 18,
    NT_DECLARATION          = 19,
    NT_GLOBAL_OR_NOT        = 20,
    NT_OTHER_STMTS          = 21,
    NT_STMT                 = 22,
    NT_ASSIGNMENT_STMT      = 23,
    NT_SINGLE_OR_REC_ID     = 24,
    NT_SOREC_SUFFIX         = 25,
    NT_FUN_CALL_STMT        = 26,
    NT_OUTPUT_PARAMS        = 27,
    NT_INPUT_PARAMS         = 28,
    NT_ITERATIVE_STMT       = 29,
    NT_CONDITIONAL_STMT     = 30,
    NT_ELSE_PART            = 31,
    NT_IO_STMT              = 32,
    NT_ARITH_EXPR           = 33,
    NT_ARITH_EXPR_PRIME     = 34,
    NT_TERM                 = 35,
    NT_TERM_PRIME           = 36,
    NT_FACTOR               = 37,
    NT_BOOL_EXPR            = 38,
    NT_VAR                  = 39,
    NT_LOGICAL_OP           = 40,
    NT_RELATIONAL_OP        = 41,
    NT_RETURN_STMT          = 42,
    NT_OPTIONAL_RETURN      = 43,
    NT_ID_LIST              = 44,
    NT_MORE_IDS             = 45,
    NT_DEFINETYPE_STMT      = 46,
    NT_TYPE_DEF_KW          = 47,
    NUM_NON_TERMINALS       = 48
} NonTerminal;

/* ─────────────────────────────────────────────────────────────
 * 3.  GRAMMAR SYMBOL  (terminal  OR  non-terminal)
 * ───────────────────────────────────────────────────────────── */
#define SYM_TERMINAL     0
#define SYM_NON_TERMINAL 1
#define SYM_EPSILON      2   /* ε – used only in rule RHS     */

typedef struct {
    int type;    /* SYM_TERMINAL | SYM_NON_TERMINAL | SYM_EPSILON */
    int value;   /* Terminal or NonTerminal enum value             */
} Symbol;

/* Convenience constructors */
#define T(tok)   { SYM_TERMINAL,     (tok) }
#define NT(nt)   { SYM_NON_TERMINAL, (nt)  }
#define EPS      { SYM_EPSILON,       0    }

/* ─────────────────────────────────────────────────────────────
 * 4.  GRAMMAR RULE
 * ───────────────────────────────────────────────────────────── */
#define MAX_RHS 10   /* Maximum symbols on RHS of any rule */

typedef struct {
    int        ruleNumber;
    NonTerminal lhs;
    Symbol      rhs[MAX_RHS];
    int         rhsLen;       /* 0 if epsilon production      */
} GrammarRule;

/* ─────────────────────────────────────────────────────────────
 * 5.  TOKEN  (produced by the lexer, consumed by the parser)
 * ───────────────────────────────────────────────────────────── */
#define MAX_LEXEME 128

typedef struct {
    Terminal    type;
    char        lexeme[MAX_LEXEME];
    int         lineNumber;
} Token;

/* ─────────────────────────────────────────────────────────────
 * 6.  MISC CONSTANTS
 * ───────────────────────────────────────────────────────────── */
#define NUM_RULES       89
#define PARSE_ERROR     -1   /* parse table "error" sentinel   */
#define STACK_MAX_SIZE  512

/* Human-readable names for diagnostics */
extern const char *terminalNames[NUM_TERMINALS];
extern const char *nonTerminalNames[NUM_NON_TERMINALS];

#endif /* PARSER_DEF_H */
