// CS F363: Compiler Construction
// Group no. 37
// 2022B4A70496P Garvit Singhal
// 2023A7PS0577P Raunit Raj
// 2023A7PS0613P Rishita Sachan
// 2023A7PS0622P Ranvijay Tanwar
// 2023A7PS0655P Rishabh Sharma
// 2023A7PS0605P Aayush Garg

/***********************************************************
 * parser.c
 *
 * LL(1) Predictive Syntax Analyzer with Panic Mode
 * Error Recovery and N-ary Parse Tree generation.
 ***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"

// ==========================================
// Parse Table Mappings and Initialization
// ==========================================

#define NUM_NON_TERMINALS 48
#define NUM_TERMINALS     65
#define PARSE_ERROR      -1

// Enums to map table indices cleanly to Grammar IDs
typedef enum {
    NT_PROGRAM = 0, NT_MAINFUNCTION, NT_OTHERFUNCTIONS, NT_FUNCTION,
    NT_INPUT_PAR, NT_OUTPUT_PAR, NT_PARAMETER_LIST, NT_DATATYPE,
    NT_PRIMITIVEDATATYPE, NT_CONSTRUCTEDDATATYPE, NT_REMAINING_LIST,
    NT_STMTS, NT_TYPEDEFINITIONS, NT_TYPEDEFINITION, NT_FIELDDEFINITIONS,
    NT_FIELDDEFINITION, NT_FIELDTYPE, NT_MOREFIELDS, NT_DECLARATIONS,
    NT_DECLARATION, NT_GLOBAL_OR_NOT, NT_OTHERSTMTS, NT_STMT,
    NT_ASSIGNMENTSTMT, NT_SINGLEORRECID, NT_SINGLEORRECIDSUFFIX,
    NT_FUNCALLSTMT, NT_OUTPUTPARAMETERS, NT_INPUTPARAMETERS,
    NT_ITERATIVESTMT, NT_CONDITIONALSTMT, NT_ELSEPART, NT_IOSTMT,
    NT_ARITHMETICEXPRESSION, NT_ARITHEXPPRIME, NT_TERM, NT_TERMPRIME,
    NT_FACTOR, NT_BOOLEANEXPRESSION, NT_VAR, NT_LOGICALOP, NT_RELATIONALOP,
    NT_RETURNSTMT, NT_OPTIONALRETURN, NT_IDLIST, NT_MORE_IDS,
    NT_DEFINETYPESTMT, NT_TYPEDEFKW
} NonTerminal;

const char *NT_NAMES[NUM_NON_TERMINALS] = {
    "program", "mainFunction", "otherFunctions", "function", "input_par",
    "output_par", "parameter_list", "dataType", "primitiveDatatype",
    "constructedDatatype", "remaining_list", "stmts", "typeDefinitions",
    "typeDefinition", "fieldDefinitions", "fieldDefinition", "fieldType",
    "moreFields", "declarations", "declaration", "global_or_not",
    "otherStmts", "stmt", "assignmentStmt", "singleOrRecId",
    "singleOrRecIdSuffix", "funCallStmt", "outputParameters", "inputParameters",
    "iterativeStmt", "conditionalStmt", "elsePart", "ioStmt",
    "arithmeticExpression", "arithExpPrime", "term", "termPrime", "factor",
    "booleanExpression", "var", "logicalOp", "relationalOp", "returnStmt",
    "optionalReturn", "idList", "more_ids", "definetypestmt", "typeDefKw"
};

// External hooks into the Parse Table logic
extern int getParseTableEntry(int nt, int terminal);
extern const char* tokenNames[];

Token lookahead;

// ==========================================
// Error Tracking & Panic Mode System
// ==========================================

CompilerError errorList[MAX_ERRORS];
int errorCount = 0;

// Acts as a shield. When 1, we are currently skipping garbage tokens
// and will try to not give the same error repeatedly
int isPanicMode = 0;

void clearErrors() {
    errorCount = 0;
    isPanicMode = 0;
}

// Logs an error safely, preventing duplicate spam
void addError(int line, const char *msg) {
    if (errorCount > 0 &&
        errorList[errorCount - 1].line == line &&
        strcmp(errorList[errorCount - 1].message, msg) == 0) {
        return;
    }

    if (errorCount < MAX_ERRORS) {
        errorList[errorCount].line = line;
        strncpy(errorList[errorCount].message, msg, 255);
        errorCount++;
    }
}

// Writes the exact required output format to the error text file
void printErrorsToFile(char *inputFilename) {
    if (errorCount == 0) return;

    // Extract filename
    char outName[256];
    char *base = strrchr(inputFilename, '/');
    if (!base) base = strrchr(inputFilename, '\\');
    base = base ? base + 1 : inputFilename;

    sprintf(outName, "listoferrors_%s", base);

    FILE *f = fopen(outName, "w");
    if (!f) return;
    for(int i = 0; i < errorCount; i++) {
        fprintf(f, "Line %d: %s\n", errorList[i].line, errorList[i].message);
    }
    fclose(f);
    printf("--> Found %d errors. Written to: %s\n", errorCount, outName);
}

// prints errors directly to the terminal
void printErrorsToConsole(void) {
    if (errorCount == 0) return;
    printf("\n--- Lexical and Syntax Errors ---\n");
    for(int i = 0; i < errorCount; i++) {
        printf("Line %d \t%s\n", errorList[i].line, errorList[i].message);
    }
    printf("---------------------------------\n");
}

// ==========================================
// Lexical Error Interception
// ==========================================
// The parser only wants valid tokens. This wrapper skips comments
// and catches lexical errors on the fly, logging them into our error system.
Token get_next_valid_token() {
    Token t = get_next_token();

    while (t.type == TK_COMMENT || t.type == TK_ERROR) {
        if (t.type == TK_ERROR) {
            char msg[256];
            if (t.err == ERR_ID_LONG)
                sprintf(msg, "Lexical Error: Variable Identifier exceeds length <%s>", t.lexeme);
            else if (t.err == ERR_FUNID_LONG)
                sprintf(msg, "Lexical Error: Function Identifier exceeds length <%s>", t.lexeme);
            else if (t.err == ERR_UNKNOWN_PATTERN)
                sprintf(msg, "Lexical Error: Unknown pattern <%s>", t.lexeme);
            else
                sprintf(msg, "Lexical Error: Unknown Symbol <%s>", t.lexeme);

            addError(t.line, msg);
        }
        t = get_next_token();
    }
    return t;
}

// ==========================================
// N-ary Parse Tree Management
// ==========================================
parseTreeNode* createNode(const char *symbol, parseTreeNode *parent) {
    parseTreeNode *node = (parseTreeNode*)malloc(sizeof(parseTreeNode));
    strcpy(node->symbol, symbol);

    node->parent = parent;
    node->childCount = 0;
    node->isLeaf = 0;

    for(int i = 0; i < MAX_CHILDREN; i++) {
        node->children[i] = NULL;
    }
    return node;
}

void addChild(parseTreeNode *parent, parseTreeNode *child) {
    if (parent->childCount < MAX_CHILDREN) {
        parent->children[parent->childCount++] = child;
    }
}

// ==========================================
// Pushdown Automaton (Stack) Implementation
// ==========================================
typedef struct {
    int isTerminal;
    int symbol;
    parseTreeNode *node;
} StackItem;

StackItem stack[MAX_STACK];
int top = -1;

void push(int isTerm, int sym, parseTreeNode *n) {
    if(top >= MAX_STACK - 1) {
        printf("\nFATAL ERROR: Stack Overflow! Expression too deep.\n");
        fflush(stdout);
        exit(1);
    }
    stack[++top] = (StackItem){isTerm, sym, n};
}

StackItem pop() {
    if(top < 0) {
        printf("\nFATAL ERROR: Stack Underflow! Malformed grammar mapping.\n");
        fflush(stdout);
        exit(1);
    }
    return stack[top--];
}

// ==========================================
// Core LL(1) Parser Engine
// ==========================================
parseTreeNode* parseInputSourceCode(char *filename) {
    top = -1;
    clearErrors();

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Could not open source file.\n");
        return NULL;
    }

    init_buffers(fp);
    lookahead = get_next_valid_token();

    // Setup the root node and initial stack state
    parseTreeNode *root = createNode("program", NULL);
    push(1, TK_EOF, NULL);
    push(0, NT_PROGRAM, root);

    while (top >= 0) {
        StackItem current = pop();

        // -----------------------------------------
        // Case 1: Stack Top is a Terminal Symbol
        // -----------------------------------------
        if (current.isTerminal) {

            if (current.symbol == TK_EOF) {
                if (lookahead.type != TK_EOF) {
                    char msg[256];
                    sprintf(msg, "Syntax Error: Extra tokens found at end of file. Lookahead: %s", tokenNames[lookahead.type]);
                    addError(lookahead.line, msg);
                }
                break; // Parsing complete
            }

            // We found a match and so we will advance the input.
            if (current.symbol == lookahead.type) {
                isPanicMode = 0; // Successfully matched, turn off the error flag
                current.node->isLeaf = 1;
                current.node->tk = lookahead;
                lookahead = get_next_valid_token();
            }
            // We have a mismatch, Missing terminal error.
            else {
                if (!isPanicMode) {
                    char msg[256];
                    sprintf(msg, "Syntax Error: Expected %s, but got %s", tokenNames[current.symbol], tokenNames[lookahead.type]);
                    addError(lookahead.line, msg);
                    isPanicMode = 1;
                }
                // Heuristic Recovery: We assume the terminal was just missing and pop the stack, but keep lookahead where it is.
            }
        }

        // -----------------------------------------
        // Case 2: Stack Top is a Non-Terminal
        // -----------------------------------------
        else {
            int ruleNum = getParseTableEntry(current.symbol, lookahead.type);

            // Table entry is empty (Syntax Error)
            if (ruleNum == -1) {
                if (!isPanicMode) {
                    char msg[256];
                    sprintf(msg, "Syntax Error: Unexpected token %s while parsing %s", tokenNames[lookahead.type], NT_NAMES[current.symbol]);
                    addError(lookahead.line, msg);
                    isPanicMode = 1;
                }

                // PANIC HEURISTIC: We check if lookahead is a structural sync token.
                // If we hit END, ENDIF, EOF, etc., we let the Non-Terminal pop without advancing.
                int isSync = (lookahead.type == TK_END || lookahead.type == TK_ENDIF ||
                              lookahead.type == TK_ENDWHILE || lookahead.type == TK_ENDRECORD ||
                              lookahead.type == TK_ENDUNION || lookahead.type == TK_EOF);

                if (!isSync) {
                    // It's garbage. We discard the token, and push the NT back onto the stack to try again.
                    push(0, current.symbol, current.node);
                    lookahead = get_next_valid_token();
                }
            }
            // Epsilon Rule Mapping
            else if (ruleNum == 0) {
                parseTreeNode *epsNode = createNode("epsilon", current.node);
                epsNode->isLeaf = 1;
                strcpy(epsNode->tk.lexeme, "eps");
                addChild(current.node, epsNode);
            }
            // Valid Rule Expansion
            else {
                GrammarRule rule = ruleTable[ruleNum];
                parseTreeNode *childrenNodes[MAX_CHILDREN];

                // Create child nodes sequentially
                for (int i = 0; i < rule.rhsCount; i++) {
                    const char* symName = rule.isTerminal[i] ? tokenNames[rule.rhs[i]] : NT_NAMES[rule.rhs[i]];
                    childrenNodes[i] = createNode(symName, current.node);
                    addChild(current.node, childrenNodes[i]);
                }

                // Push children to the stack in REVERSE order so the leftmost is processed first
                for (int i = rule.rhsCount - 1; i >= 0; i--) {
                    push(rule.isTerminal[i], rule.rhs[i], childrenNodes[i]);
                }
            }
        }
    }

    fclose(fp);
    return root;
}

// ==========================================
// Parse Tree Output formatting
// ==========================================
void printInorder(parseTreeNode *node, FILE *fp) {
    if (!node) return;

    // Visit leftmost child first
    if (node->childCount > 0) {
        printInorder(node->children[0], fp);
    }

    // Default formatting strings for non-leaf nodes
    char lexeme[30] = "----";
    char tokenName[30] = "----";
    char valueStr[30] = "----";
    char parent[50];

    // If it's a leaf, populate actual token data
    if (node->isLeaf) {
        if (strcmp(node->symbol, "epsilon") == 0) {
            strcpy(lexeme, "epsilon");
            strcpy(tokenName, "TK_EPSILON");
        } else {
            strcpy(lexeme, node->tk.lexeme);
            strcpy(tokenName, tokenNames[node->tk.type]);

            // Extract numerical values if applicable
            if (node->tk.type == TK_NUM) {
                sprintf(valueStr, "%d", node->tk.attr.intVal);
            } else if (node->tk.type == TK_RNUM) {
                sprintf(valueStr, "%.2f", node->tk.attr.realVal);
            }
        }
    }

    // Safely assign parent string
    strcpy(parent, (node->parent == NULL) ? "ROOT" : node->parent->symbol);

    // Print with column justifications
    fprintf(fp, "%-20s %-6d %-20s %-15s %-25s %-5s %-20s\n",
            lexeme,
            (node->isLeaf && strcmp(node->symbol, "epsilon") != 0) ? node->tk.line : -1,
            tokenName,
            valueStr,
            parent,
            node->isLeaf ? "yes" : "no",
            node->symbol);

    // Visit remaining siblings
    for (int i = 1; i < node->childCount; i++) {
        printInorder(node->children[i], fp);
    }
}

void printParseTree(parseTreeNode *root, char *outfile) {
    FILE *fp = fopen(outfile, "w");
    if (fp) {
        printInorder(root, fp);
        fclose(fp);
    }
}


// ==========================================
// Grammar Rules and Parse Table Mappings
// ==========================================
// We avoid hardcoding a massive 2D integer array. Instead, we use a macro-driven
// approach. This simulates a parser generator's output, making it highly
// readable and easy to debug if the FIRST/FOLLOW sets need tweaking.

GrammarRule ruleTable[100];
static int parseTable[NUM_NON_TERMINALS][NUM_TERMINALS];

// MACROS to safely populate ruleTable
// r: Rule ID | t1, t2: 1 if Terminal, 0 if Non-Terminal | s1, s2: The Symbol Enums
#define RULE_1(r, t1, s1) \
    ruleTable[r].rhsCount = 1; ruleTable[r].isTerminal[0] = t1; ruleTable[r].rhs[0] = s1;

#define RULE_2(r, t1, s1, t2, s2) \
    ruleTable[r].rhsCount = 2; ruleTable[r].isTerminal[0] = t1; ruleTable[r].rhs[0] = s1; \
    ruleTable[r].isTerminal[1] = t2; ruleTable[r].rhs[1] = s2;

#define RULE_3(r, t1, s1, t2, s2, t3, s3) \
    ruleTable[r].rhsCount = 3; ruleTable[r].isTerminal[0] = t1; ruleTable[r].rhs[0] = s1; \
    ruleTable[r].isTerminal[1] = t2; ruleTable[r].rhs[1] = s2; ruleTable[r].isTerminal[2] = t3; ruleTable[r].rhs[2] = s3;

#define RULE_4(r, t1, s1, t2, s2, t3, s3, t4, s4) \
    ruleTable[r].rhsCount = 4; ruleTable[r].isTerminal[0] = t1; ruleTable[r].rhs[0] = s1; \
    ruleTable[r].isTerminal[1] = t2; ruleTable[r].rhs[1] = s2; ruleTable[r].isTerminal[2] = t3; ruleTable[r].rhs[2] = s3; \
    ruleTable[r].isTerminal[3] = t4; ruleTable[r].rhs[3] = s4;

#define RULE_5(r, t1, s1, t2, s2, t3, s3, t4, s4, t5, s5) \
    ruleTable[r].rhsCount = 5; ruleTable[r].isTerminal[0] = t1; ruleTable[r].rhs[0] = s1; \
    ruleTable[r].isTerminal[1] = t2; ruleTable[r].rhs[1] = s2; ruleTable[r].isTerminal[2] = t3; ruleTable[r].rhs[2] = s3; \
    ruleTable[r].isTerminal[3] = t4; ruleTable[r].rhs[3] = s4; ruleTable[r].isTerminal[4] = t5; ruleTable[r].rhs[4] = s5;

#define RULE_6(r, t1, s1, t2, s2, t3, s3, t4, s4, t5, s5, t6, s6) \
    ruleTable[r].rhsCount = 6; ruleTable[r].isTerminal[0] = t1; ruleTable[r].rhs[0] = s1; \
    ruleTable[r].isTerminal[1] = t2; ruleTable[r].rhs[1] = s2; ruleTable[r].isTerminal[2] = t3; ruleTable[r].rhs[2] = s3; \
    ruleTable[r].isTerminal[3] = t4; ruleTable[r].rhs[3] = s4; ruleTable[r].isTerminal[4] = t5; ruleTable[r].rhs[4] = s5; \
    ruleTable[r].isTerminal[5] = t6; ruleTable[r].rhs[5] = s6;

#define RULE_7(r, t1, s1, t2, s2, t3, s3, t4, s4, t5, s5, t6, s6, t7, s7) \
    ruleTable[r].rhsCount = 7; ruleTable[r].isTerminal[0] = t1; ruleTable[r].rhs[0] = s1; \
    ruleTable[r].isTerminal[1] = t2; ruleTable[r].rhs[1] = s2; ruleTable[r].isTerminal[2] = t3; ruleTable[r].rhs[2] = s3; \
    ruleTable[r].isTerminal[3] = t4; ruleTable[r].rhs[3] = s4; ruleTable[r].isTerminal[4] = t5; ruleTable[r].rhs[4] = s5; \
    ruleTable[r].isTerminal[5] = t6; ruleTable[r].rhs[5] = s6; ruleTable[r].isTerminal[6] = t7; ruleTable[r].rhs[6] = s7;

#define RULE_8(r, t1, s1, t2, s2, t3, s3, t4, s4, t5, s5, t6, s6, t7, s7, t8, s8) \
    ruleTable[r].rhsCount = 8; ruleTable[r].isTerminal[0] = t1; ruleTable[r].rhs[0] = s1; \
    ruleTable[r].isTerminal[1] = t2; ruleTable[r].rhs[1] = s2; ruleTable[r].isTerminal[2] = t3; ruleTable[r].rhs[2] = s3; \
    ruleTable[r].isTerminal[3] = t4; ruleTable[r].rhs[3] = s4; ruleTable[r].isTerminal[4] = t5; ruleTable[r].rhs[4] = s5; \
    ruleTable[r].isTerminal[5] = t6; ruleTable[r].rhs[5] = s6; ruleTable[r].isTerminal[6] = t7; ruleTable[r].rhs[6] = s7; \
    ruleTable[r].isTerminal[7] = t8; ruleTable[r].rhs[7] = s8;

// Populates the array that parser.c uses to push RHS symbols onto the stack
void initGrammarRules() {
    RULE_2(1, 0,NT_OTHERFUNCTIONS, 0,NT_MAINFUNCTION);
    RULE_3(2, 1,TK_MAIN, 0,NT_STMTS, 1,TK_END);
    RULE_2(3, 0,NT_FUNCTION, 0,NT_OTHERFUNCTIONS);
    RULE_6(5, 1,TK_FUNID, 0,NT_INPUT_PAR, 0,NT_OUTPUT_PAR, 1,TK_SEM, 0,NT_STMTS, 1,TK_END);
    RULE_6(6, 1,TK_INPUT, 1,TK_PARAMETER, 1,TK_LIST, 1,TK_SQL, 0,NT_PARAMETER_LIST, 1,TK_SQR);
    RULE_6(7, 1,TK_OUTPUT, 1,TK_PARAMETER, 1,TK_LIST, 1,TK_SQL, 0,NT_PARAMETER_LIST, 1,TK_SQR);
    RULE_3(9, 0,NT_DATATYPE, 1,TK_ID, 0,NT_REMAINING_LIST);
    RULE_1(10, 0,NT_PRIMITIVEDATATYPE);
    RULE_1(11, 0,NT_CONSTRUCTEDDATATYPE);
    RULE_1(12, 1,TK_INT);
    RULE_1(13, 1,TK_REAL);
    RULE_2(14, 1,TK_RECORD, 1,TK_RUID);
    RULE_2(15, 1,TK_UNION, 1,TK_RUID);
    RULE_1(16, 1,TK_RUID);
    RULE_2(17, 1,TK_COMMA, 0,NT_PARAMETER_LIST);
    RULE_4(19, 0,NT_TYPEDEFINITIONS, 0,NT_DECLARATIONS, 0,NT_OTHERSTMTS, 0,NT_RETURNSTMT);
    RULE_2(20, 0,NT_TYPEDEFINITION, 0,NT_TYPEDEFINITIONS);
    RULE_4(22, 1,TK_RECORD, 1,TK_RUID, 0,NT_FIELDDEFINITIONS, 1,TK_ENDRECORD);
    RULE_4(23, 1,TK_UNION, 1,TK_RUID, 0,NT_FIELDDEFINITIONS, 1,TK_ENDUNION);
    RULE_1(24, 0,NT_DEFINETYPESTMT);
    RULE_3(25, 0,NT_FIELDDEFINITION, 0,NT_FIELDDEFINITION, 0,NT_MOREFIELDS);
    RULE_5(26, 1,TK_TYPE, 0,NT_FIELDTYPE, 1,TK_COLON, 1,TK_FIELDID, 1,TK_SEM);
    RULE_1(27, 0,NT_PRIMITIVEDATATYPE);
    RULE_1(28, 0,NT_CONSTRUCTEDDATATYPE);
    RULE_2(29, 0,NT_FIELDDEFINITION, 0,NT_MOREFIELDS);
    RULE_2(31, 0,NT_DECLARATION, 0,NT_DECLARATIONS);
    RULE_6(33, 1,TK_TYPE, 0,NT_DATATYPE, 1,TK_COLON, 1,TK_ID, 0,NT_GLOBAL_OR_NOT, 1,TK_SEM);
    RULE_2(34, 1,TK_COLON, 1,TK_GLOBAL);
    RULE_2(36, 0,NT_STMT, 0,NT_OTHERSTMTS);
    RULE_1(38, 0,NT_ASSIGNMENTSTMT);
    RULE_1(39, 0,NT_ITERATIVESTMT);
    RULE_1(40, 0,NT_CONDITIONALSTMT);
    RULE_1(41, 0,NT_IOSTMT);
    RULE_1(42, 0,NT_FUNCALLSTMT);
    RULE_4(43, 0,NT_SINGLEORRECID, 1,TK_ASSIGNOP, 0,NT_ARITHMETICEXPRESSION, 1,TK_SEM);
    RULE_2(44, 1,TK_ID, 0,NT_SINGLEORRECIDSUFFIX);
    RULE_3(45, 1,TK_DOT, 1,TK_FIELDID, 0,NT_SINGLEORRECIDSUFFIX);
    RULE_7(47, 0,NT_OUTPUTPARAMETERS, 1,TK_CALL, 1,TK_FUNID, 1,TK_WITH, 1,TK_PARAMETERS, 0,NT_INPUTPARAMETERS, 1,TK_SEM);
    RULE_4(48, 1,TK_SQL, 0,NT_IDLIST, 1,TK_SQR, 1,TK_ASSIGNOP);
    RULE_3(50, 1,TK_SQL, 0,NT_IDLIST, 1,TK_SQR);
    RULE_7(51, 1,TK_WHILE, 1,TK_OP, 0,NT_BOOLEANEXPRESSION, 1,TK_CL, 0,NT_STMT, 0,NT_OTHERSTMTS, 1,TK_ENDWHILE);
    RULE_8(52, 1,TK_IF, 1,TK_OP, 0,NT_BOOLEANEXPRESSION, 1,TK_CL, 1,TK_THEN, 0,NT_STMT, 0,NT_OTHERSTMTS, 0,NT_ELSEPART);
    RULE_4(53, 1,TK_ELSE, 0,NT_STMT, 0,NT_OTHERSTMTS, 1,TK_ENDIF);
    RULE_1(54, 1,TK_ENDIF); // Explicitly consume ENDIF instead of epsilon
    RULE_5(55, 1,TK_READ, 1,TK_OP, 0,NT_VAR, 1,TK_CL, 1,TK_SEM);
    RULE_5(56, 1,TK_WRITE, 1,TK_OP, 0,NT_VAR, 1,TK_CL, 1,TK_SEM);
    RULE_2(57, 0,NT_TERM, 0,NT_ARITHEXPPRIME);
    RULE_3(58, 1,TK_PLUS, 0,NT_TERM, 0,NT_ARITHEXPPRIME);
    RULE_3(59, 1,TK_MINUS, 0,NT_TERM, 0,NT_ARITHEXPPRIME);
    RULE_2(61, 0,NT_FACTOR, 0,NT_TERMPRIME);
    RULE_3(62, 1,TK_MUL, 0,NT_FACTOR, 0,NT_TERMPRIME);
    RULE_3(63, 1,TK_DIV, 0,NT_FACTOR, 0,NT_TERMPRIME);
    RULE_3(65, 1,TK_OP, 0,NT_ARITHMETICEXPRESSION, 1,TK_CL);
    RULE_1(66, 0,NT_VAR);
    RULE_1(67, 0,NT_SINGLEORRECID);
    RULE_1(68, 1,TK_NUM);
    RULE_1(69, 1,TK_RNUM);
    RULE_7(70, 1,TK_OP, 0,NT_BOOLEANEXPRESSION, 1,TK_CL, 0,NT_LOGICALOP, 1,TK_OP, 0,NT_BOOLEANEXPRESSION, 1,TK_CL);
    RULE_3(71, 0,NT_VAR, 0,NT_RELATIONALOP, 0,NT_VAR);
    RULE_4(72, 1,TK_NOT, 1,TK_OP, 0,NT_BOOLEANEXPRESSION, 1,TK_CL);
    RULE_1(73, 1,TK_AND);
    RULE_1(74, 1,TK_OR);
    RULE_1(75, 1,TK_LT);
    RULE_1(76, 1,TK_LE);
    RULE_1(77, 1,TK_EQ);
    RULE_1(78, 1,TK_GT);
    RULE_1(79, 1,TK_GE);
    RULE_1(80, 1,TK_NE);
    RULE_3(81, 1,TK_RETURN, 0,NT_OPTIONALRETURN, 1,TK_SEM);
    RULE_3(82, 1,TK_SQL, 0,NT_IDLIST, 1,TK_SQR);
    RULE_2(84, 1,TK_ID, 0,NT_MORE_IDS);
    RULE_2(85, 1,TK_COMMA, 0,NT_IDLIST);
    RULE_5(87, 1,TK_DEFINETYPE, 0,NT_TYPEDEFKW, 1,TK_RUID, 1,TK_AS, 1,TK_RUID);
    RULE_1(88, 1,TK_RECORD);
    RULE_1(89, 1,TK_UNION);
}

// Populates the LL(1) parse table matrix safely using exact Enums based on FIRST/FOLLOW sets
void initParseTable(void) {
    initGrammarRules();

    // 1. Initialize the entire matrix to PARSE_ERROR
    for (int i = 0; i < NUM_NON_TERMINALS; i++) {
        for (int j = 0; j < NUM_TERMINALS; j++) {
            parseTable[i][j] = PARSE_ERROR;
        }
    }

    // 2. Macro to cleanly map M[NonTerminal][Token] -> Rule ID (or 0 for Epsilon)
    #define SET(nt, tk, rule) parseTable[(nt)][(tk)] = (rule)

    // Program and Functions
    SET(NT_PROGRAM, TK_FUNID, 1);    SET(NT_PROGRAM, TK_MAIN, 1);
    SET(NT_MAINFUNCTION, TK_MAIN, 2);
    SET(NT_OTHERFUNCTIONS, TK_FUNID, 3);   SET(NT_OTHERFUNCTIONS, TK_MAIN, 0);
    SET(NT_FUNCTION, TK_FUNID, 5);
    SET(NT_INPUT_PAR, TK_INPUT, 6);
    SET(NT_OUTPUT_PAR, TK_OUTPUT, 7);   SET(NT_OUTPUT_PAR, TK_SEM, 0);

    // Data Types and Parameters
    SET(NT_PARAMETER_LIST, TK_INT, 9);   SET(NT_PARAMETER_LIST, TK_REAL, 9);
    SET(NT_PARAMETER_LIST, TK_RECORD, 9); SET(NT_PARAMETER_LIST, TK_UNION, 9);
    SET(NT_PARAMETER_LIST, TK_RUID, 9);

    SET(NT_DATATYPE, TK_INT, 10);   SET(NT_DATATYPE, TK_REAL, 10);
    SET(NT_DATATYPE, TK_RECORD, 11); SET(NT_DATATYPE, TK_UNION, 11);
    SET(NT_DATATYPE, TK_RUID, 11);

    SET(NT_PRIMITIVEDATATYPE, TK_INT, 12);   SET(NT_PRIMITIVEDATATYPE, TK_REAL, 13);
    SET(NT_CONSTRUCTEDDATATYPE, TK_RECORD, 14); SET(NT_CONSTRUCTEDDATATYPE, TK_UNION, 15);
    SET(NT_CONSTRUCTEDDATATYPE, TK_RUID, 16);

    SET(NT_REMAINING_LIST, TK_COMMA, 17);   SET(NT_REMAINING_LIST, TK_SQR, 0);

    // Statements and Definitions
    SET(NT_STMTS, TK_RECORD, 19);  SET(NT_STMTS, TK_UNION, 19);  SET(NT_STMTS, TK_DEFINETYPE, 19);
    SET(NT_STMTS, TK_TYPE, 19);    SET(NT_STMTS, TK_ID, 19);     SET(NT_STMTS, TK_WHILE, 19);
    SET(NT_STMTS, TK_IF, 19);      SET(NT_STMTS, TK_READ, 19);   SET(NT_STMTS, TK_WRITE, 19);
    SET(NT_STMTS, TK_SQL, 19);     SET(NT_STMTS, TK_CALL, 19);   SET(NT_STMTS, TK_RETURN, 19);

    SET(NT_TYPEDEFINITIONS, TK_RECORD, 20);  SET(NT_TYPEDEFINITIONS, TK_UNION, 20);
    SET(NT_TYPEDEFINITIONS, TK_DEFINETYPE, 20);
    SET(NT_TYPEDEFINITIONS, TK_TYPE, 0);  SET(NT_TYPEDEFINITIONS, TK_ID, 0);
    SET(NT_TYPEDEFINITIONS, TK_WHILE, 0); SET(NT_TYPEDEFINITIONS, TK_IF, 0);
    SET(NT_TYPEDEFINITIONS, TK_READ, 0);  SET(NT_TYPEDEFINITIONS, TK_WRITE, 0);
    SET(NT_TYPEDEFINITIONS, TK_SQL, 0);   SET(NT_TYPEDEFINITIONS, TK_CALL, 0);
    SET(NT_TYPEDEFINITIONS, TK_RETURN, 0);

    SET(NT_TYPEDEFINITION, TK_RECORD, 22);  SET(NT_TYPEDEFINITION, TK_UNION, 23);
    SET(NT_TYPEDEFINITION, TK_DEFINETYPE, 24);

    SET(NT_FIELDDEFINITIONS, TK_TYPE, 25);
    SET(NT_FIELDDEFINITION, TK_TYPE, 26);

    SET(NT_FIELDTYPE, TK_INT, 27);  SET(NT_FIELDTYPE, TK_REAL, 27);
    SET(NT_FIELDTYPE, TK_RECORD, 28); SET(NT_FIELDTYPE, TK_UNION, 28);
    SET(NT_FIELDTYPE, TK_RUID, 28);

    SET(NT_MOREFIELDS, TK_TYPE, 29);  SET(NT_MOREFIELDS, TK_ENDRECORD, 0);
    SET(NT_MOREFIELDS, TK_ENDUNION, 0);

    SET(NT_DECLARATIONS, TK_TYPE, 31);  SET(NT_DECLARATIONS, TK_ID, 0);
    SET(NT_DECLARATIONS, TK_WHILE, 0);  SET(NT_DECLARATIONS, TK_IF, 0);
    SET(NT_DECLARATIONS, TK_READ, 0);   SET(NT_DECLARATIONS, TK_WRITE, 0);
    SET(NT_DECLARATIONS, TK_SQL, 0);    SET(NT_DECLARATIONS, TK_CALL, 0);
    SET(NT_DECLARATIONS, TK_RETURN, 0);

    SET(NT_DECLARATION, TK_TYPE, 33);
    SET(NT_GLOBAL_OR_NOT, TK_COLON, 34);  SET(NT_GLOBAL_OR_NOT, TK_SEM, 0);

    SET(NT_OTHERSTMTS, TK_ID, 36);     SET(NT_OTHERSTMTS, TK_WHILE, 36);
    SET(NT_OTHERSTMTS, TK_IF, 36);     SET(NT_OTHERSTMTS, TK_READ, 36);
    SET(NT_OTHERSTMTS, TK_WRITE, 36);  SET(NT_OTHERSTMTS, TK_SQL, 36);
    SET(NT_OTHERSTMTS, TK_CALL, 36);   SET(NT_OTHERSTMTS, TK_RETURN, 0);
    SET(NT_OTHERSTMTS, TK_ENDWHILE, 0);SET(NT_OTHERSTMTS, TK_ELSE, 0);
    SET(NT_OTHERSTMTS, TK_ENDIF, 0);

    SET(NT_STMT, TK_ID, 38);      SET(NT_STMT, TK_WHILE, 39);
    SET(NT_STMT, TK_IF, 40);      SET(NT_STMT, TK_READ, 41);
    SET(NT_STMT, TK_WRITE, 41);   SET(NT_STMT, TK_SQL, 42);
    SET(NT_STMT, TK_CALL, 42);

    SET(NT_ASSIGNMENTSTMT, TK_ID, 43);
    SET(NT_SINGLEORRECID, TK_ID, 44);

    // Expression & Operator Epsilon Transitions (for FOLLOW sets)
    SET(NT_SINGLEORRECIDSUFFIX, TK_DOT, 45);
    SET(NT_SINGLEORRECIDSUFFIX, TK_ASSIGNOP, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_LT, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_LE, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_EQ, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_GT, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_GE, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_NE, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_PLUS, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_MINUS, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_MUL, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_DIV, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_SEM, 0);
    SET(NT_SINGLEORRECIDSUFFIX, TK_CL, 0);

    // Control Flow and I/O
    SET(NT_FUNCALLSTMT, TK_SQL, 47);  SET(NT_FUNCALLSTMT, TK_CALL, 47);
    SET(NT_OUTPUTPARAMETERS, TK_SQL, 48);  SET(NT_OUTPUTPARAMETERS, TK_CALL, 0);
    SET(NT_INPUTPARAMETERS, TK_SQL, 50);
    SET(NT_ITERATIVESTMT, TK_WHILE, 51);
    SET(NT_CONDITIONALSTMT, TK_IF, 52);
    SET(NT_ELSEPART, TK_ELSE, 53);  SET(NT_ELSEPART, TK_ENDIF, 54);
    SET(NT_IOSTMT, TK_READ, 55);  SET(NT_IOSTMT, TK_WRITE, 56);

    // Arithmetic and Boolean Expressions
    SET(NT_ARITHMETICEXPRESSION, TK_OP, 57);  SET(NT_ARITHMETICEXPRESSION, TK_ID, 57);
    SET(NT_ARITHMETICEXPRESSION, TK_NUM, 57); SET(NT_ARITHMETICEXPRESSION, TK_RNUM, 57);

    SET(NT_ARITHEXPPRIME, TK_PLUS, 58);  SET(NT_ARITHEXPPRIME, TK_MINUS, 59);
    SET(NT_ARITHEXPPRIME, TK_SEM, 0);    SET(NT_ARITHEXPPRIME, TK_CL, 0);

    SET(NT_TERM, TK_OP, 61);  SET(NT_TERM, TK_ID, 61);
    SET(NT_TERM, TK_NUM, 61); SET(NT_TERM, TK_RNUM, 61);

    SET(NT_TERMPRIME, TK_MUL, 62);  SET(NT_TERMPRIME, TK_DIV, 63);
    SET(NT_TERMPRIME, TK_PLUS, 0);  SET(NT_TERMPRIME, TK_MINUS, 0);
    SET(NT_TERMPRIME, TK_SEM, 0);   SET(NT_TERMPRIME, TK_CL, 0);

    SET(NT_FACTOR, TK_OP, 65);  SET(NT_FACTOR, TK_ID, 66);
    SET(NT_FACTOR, TK_NUM, 66); SET(NT_FACTOR, TK_RNUM, 66);

    SET(NT_BOOLEANEXPRESSION, TK_OP, 70);  SET(NT_BOOLEANEXPRESSION, TK_ID, 71);
    SET(NT_BOOLEANEXPRESSION, TK_NUM, 71); SET(NT_BOOLEANEXPRESSION, TK_RNUM, 71);
    SET(NT_BOOLEANEXPRESSION, TK_NOT, 72);

    SET(NT_VAR, TK_ID, 67);  SET(NT_VAR, TK_NUM, 68);  SET(NT_VAR, TK_RNUM, 69);
    SET(NT_LOGICALOP, TK_AND, 73);  SET(NT_LOGICALOP, TK_OR, 74);

    SET(NT_RELATIONALOP, TK_LT, 75);  SET(NT_RELATIONALOP, TK_LE, 76);
    SET(NT_RELATIONALOP, TK_EQ, 77);  SET(NT_RELATIONALOP, TK_GT, 78);
    SET(NT_RELATIONALOP, TK_GE, 79);  SET(NT_RELATIONALOP, TK_NE, 80);

    SET(NT_RETURNSTMT, TK_RETURN, 81);
    SET(NT_OPTIONALRETURN, TK_SQL, 82);  SET(NT_OPTIONALRETURN, TK_SEM, 0);
    SET(NT_IDLIST, TK_ID, 84);
    SET(NT_MORE_IDS, TK_COMMA, 85);  SET(NT_MORE_IDS, TK_SQR, 0);
    SET(NT_DEFINETYPESTMT, TK_DEFINETYPE, 87);
    SET(NT_TYPEDEFKW, TK_RECORD, 88);  SET(NT_TYPEDEFKW, TK_UNION, 89);

    #undef SET
}

// fetches the rule from the initialized Parse Table
int getParseTableEntry(int nt, int terminal) {
    if (nt < 0 || nt >= NUM_NON_TERMINALS) return PARSE_ERROR;
    if (terminal < 0 || terminal >= NUM_TERMINALS) return PARSE_ERROR;
    return parseTable[nt][terminal];
}

