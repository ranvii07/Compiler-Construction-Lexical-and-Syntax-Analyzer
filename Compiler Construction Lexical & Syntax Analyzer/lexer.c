// CS F363: Compiler Construction
// Group no. 37
// 2022B4A70496P Garvit Singhal
// 2023A7PS0577P Raunit Raj
// 2023A7PS0613P Rishita Sachan
// 2023A7PS0622P Ranvijay Tanwar
// 2023A7PS0655P Rishabh Sharma
// 2023A7PS0605P Aayush Garg

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "lexerDef.h"

// String mappings for tokens
const char* tokenNames[] = {
    "TK_NUM", "TK_RNUM", "TK_ID", "TK_FIELDID", "TK_FUNID", "TK_RUID", "TK_MAIN",
    "TK_ASSIGNOP", "TK_AND", "TK_OR", "TK_NOT", "TK_LT", "TK_LE", "TK_EQ", "TK_GT", "TK_GE", "TK_NE",
    "TK_PLUS", "TK_MINUS", "TK_MUL", "TK_DIV", "TK_COMMA", "TK_SEM", "TK_COLON", "TK_DOT",
    "TK_OP", "TK_CL", "TK_SQL", "TK_SQR", "TK_WITH", "TK_PARAMETERS", "TK_END", "TK_WHILE",
    "TK_UNION", "TK_ENDUNION", "TK_DEFINETYPE", "TK_AS", "TK_TYPE", "TK_GLOBAL",
    "TK_PARAMETER", "TK_LIST", "TK_INPUT", "TK_OUTPUT", "TK_INT", "TK_REAL",
    "TK_ENDWHILE", "TK_IF", "TK_THEN", "TK_ENDIF", "TK_READ", "TK_WRITE", "TK_RETURN",
    "TK_CALL", "TK_RECORD", "TK_ENDRECORD", "TK_ELSE", "TK_ERROR", "TK_EOF", "TK_COMMENT"
};

// ==========================================
// Symbol Table Implementation
// ==========================================
typedef struct SymbolNode {
    char name[256];
    TokenType type;
    struct SymbolNode* next;
} SymbolNode;

#define SYM_TABLE_SIZE 211
SymbolNode* symbolTable[SYM_TABLE_SIZE];

// Simple djb2 hash algorithm for decent distribution
unsigned long get_hash(char* str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) {
        h = ((h << 5) + h) + c;
    }
    return h % SYM_TABLE_SIZE;
}

// Inserts an identifier or keyword into the table
void insert_symbol(char* name, TokenType type) {
    unsigned long h = get_hash(name);
    SymbolNode* curr = symbolTable[h];

    // Check for duplicates before inserting
    while (curr) {
        if (strcmp(curr->name, name) == 0) return;
        curr = curr->next;
    }

    SymbolNode* newNode = (SymbolNode*)malloc(sizeof(SymbolNode));
    strcpy(newNode->name, name);
    newNode->type = type;
    newNode->next = symbolTable[h];
    symbolTable[h] = newNode;
}

// ==========================================
// Comment Removal
// ==========================================
void removeComments(char *testcaseFile, char *cleanFile) {
    FILE *src = fopen(testcaseFile, "r");
    FILE *dest = fopen(cleanFile, "w");

    if (!src || !dest) {
        perror("Error opening files for comment removal");
        return;
    }

    char c;
    int inComment = 0;

    // Read character by character. If we see a '%', toggle comment mode until newline.
    while ((c = fgetc(src)) != EOF) {
        if (!inComment && c == '%') {
            inComment = 1;
        } else if (inComment && c == '\n') {
            inComment = 0;
            fputc(c, dest); // Keep the newline to maintain correct line numbers
        } else if (!inComment) {
            fputc(c, dest);
        }
    }

    fclose(src);
    fclose(dest);

    // Print the cleaned code to console as required by driver option 1
    printf("\n--- Comment-Free Code ---\n");
    dest = fopen(cleanFile, "r");
    while ((c = fgetc(dest)) != EOF) {
        putchar(c);
    }
    fclose(dest);
    printf("\n-------------------------\n");
}

// ==========================================
// Keyword Management
// ==========================================
typedef struct {
    char* key;
    TokenType t;
} KeyEntry;

KeyEntry keywords[30];
int keyCount = 0;

void add_keyword(char* k, TokenType t) {
    keywords[keyCount].key = k;
    keywords[keyCount++].t = t;
}

// Loads all language keywords into our array
void populate_keywords() {
    add_keyword("with", TK_WITH); add_keyword("parameters", TK_PARAMETERS);
    add_keyword("end", TK_END); add_keyword("while", TK_WHILE);
    add_keyword("union", TK_UNION); add_keyword("int", TK_INT);
    add_keyword("real", TK_REAL); add_keyword("if", TK_IF);
    add_keyword("then", TK_THEN); add_keyword("return", TK_RETURN);
    add_keyword("_main", TK_MAIN); add_keyword("else", TK_ELSE);
    add_keyword("read", TK_READ); add_keyword("write", TK_WRITE);
    add_keyword("input", TK_INPUT); add_keyword("output", TK_OUTPUT);
    add_keyword("record", TK_RECORD); add_keyword("endrecord", TK_ENDRECORD);
    add_keyword("endunion", TK_ENDUNION); add_keyword("definetype", TK_DEFINETYPE);
    add_keyword("as", TK_AS); add_keyword("type", TK_TYPE); add_keyword("global", TK_GLOBAL);
    add_keyword("parameter", TK_PARAMETER); add_keyword("list", TK_LIST);
    add_keyword("endwhile", TK_ENDWHILE); add_keyword("endif", TK_ENDIF);
    add_keyword("call", TK_CALL);
}

// Checks if a scanned lexeme is a keyword. If not, adds it to the symbol table as an ID.
TokenType check_keyword_or_id(char* lexeme, TokenType defaultType) {
    for (int i = 0; i < keyCount; i++) {
        if (strcmp(lexeme, keywords[i].key) == 0) {
            return keywords[i].t;
        }
    }
    insert_symbol(lexeme, defaultType);
    return defaultType;
}

// ==========================================
// Twin Buffer System
// ==========================================
char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];
char *forward;
int activeBuffer = 1;
FILE *sourceFile;
int lineNum = 1;

void init_buffers(FILE *fp) {
    sourceFile = fp;
    size_t n = fread(buffer1, 1, BUFFER_SIZE - 1, sourceFile);
    buffer1[n] = EOF;
    forward = buffer1;
    activeBuffer = 1;
    lineNum = 1;
}

// Fetches the next char from the active buffer, reloading chunks from disk when needed.
char get_next_char() {
    char c = *forward;

    // If we hit the end of the current buffer, swap to the other one
    if (c == EOF && !feof(sourceFile)) {
        if (activeBuffer == 1) {
            size_t n = fread(buffer2, 1, BUFFER_SIZE - 1, sourceFile);
            buffer2[n] = EOF;
            forward = buffer2;
            activeBuffer = 2;
        } else {
            size_t n = fread(buffer1, 1, BUFFER_SIZE - 1, sourceFile);
            buffer1[n] = EOF;
            forward = buffer1;
            activeBuffer = 1;
        }
        c = *forward;
    }

    if (c == '\n') lineNum++;
    forward++;
    return c;
}

// Moves the pointer back one step if the DFA looks ahead too far
void retract() {
    forward--;
    if (*forward == '\n') lineNum--;
}

// ==========================================
// Lexer DFA Engine
// ==========================================
Token get_next_token() {
    int state = 0;
    char c;
    int i = 0;

    Token T;
    T.line = lineNum;
    T.err = ERR_NONE;
    memset(T.lexeme, 0, 256); // Zero out the lexeme to prevent strcat garbage

    while (1) {
        c = get_next_char();

        switch (state) {
            case 0:
                // Base state: determine the path based on the starting character
                if (isspace(c)) {
                    T.line = lineNum;
                    continue;
                }

                T.lexeme[i++] = c;

                if (c == '%') { state = 50; continue; }

                if (isdigit(c)) state = 1;
                else if (c >= 'b' && c <= 'd') state = 12;
                else if (islower(c)) state = 15;
                else if (c == '_') state = 17;
                else if (c == '#') state = 21;
                else if (c == '<') state = 24;
                else if (c == '&') state = 33;
                else if (c == '@') state = 36;
                else if (c == '=') state = 30;
                else if (c == '!') state = 42;
                else if (c == '>') state = 45;
                else if (c == EOF) {
                    T.type = TK_EOF;
                    return T;
                }
                else {
                    // Single character operators
                    if (c == '+') T.type = TK_PLUS;
                    else if (c == '-') T.type = TK_MINUS;
                    else if (c == '*') T.type = TK_MUL;
                    else if (c == '/') T.type = TK_DIV;
                    else if (c == '.') T.type = TK_DOT;
                    else if (c == '~') T.type = TK_NOT;
                    else if (c == ',') T.type = TK_COMMA;
                    else if (c == ';') T.type = TK_SEM;
                    else if (c == ':') T.type = TK_COLON;
                    else if (c == '(') T.type = TK_OP;
                    else if (c == ')') T.type = TK_CL;
                    else if (c == '[') T.type = TK_SQL;
                    else if (c == ']') T.type = TK_SQR;
                    else {
                        T.type = TK_ERROR;
                        T.err = ERR_UNKNOWN_SYMBOL;
                    }
                    return T;
                }
                break;

            // --- Numbers (Integers and Reals) ---
            case 1:
                if (isdigit(c)) {
                    T.lexeme[i++] = c;
                    state = 1;
                } else if (c == '.') {
                    T.lexeme[i++] = c;
                    state = 3;
                } else {
                    retract();
                    T.type = TK_NUM;
                    T.attr.intVal = atoi(T.lexeme);
                    return T;
                }
                break;

            case 3:
                if (isdigit(c)) { T.lexeme[i++] = c; state = 4; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 4:
                if (isdigit(c)) { T.lexeme[i++] = c; state = 5; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 5:
                if (c == 'E') { T.lexeme[i++] = c; state = 7; }
                else {
                    retract();
                    T.type = TK_RNUM;
                    T.attr.realVal = atof(T.lexeme);
                    return T;
                }
                break;

            case 7:
                if (c == '+' || c == '-') { T.lexeme[i++] = c; state = 8; }
                else if (isdigit(c)) { T.lexeme[i++] = c; state = 9; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 8:
                if (isdigit(c)) { T.lexeme[i++] = c; state = 9; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 9:
                if (isdigit(c)) { T.lexeme[i++] = c; state = 10; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 10:
                T.type = TK_RNUM;
                T.attr.realVal = atof(T.lexeme);
                return T;

            // --- Identifiers (Variables and Fields) ---
            case 12:
                if (c >= '2' && c <= '7') {
                    T.lexeme[i++] = c;
                    state = 13;
                } else if (islower(c)) {
                    T.lexeme[i++] = c;
                    state = 15;
                } else {
                    retract();
                    // Single 'b', 'c', or 'd' is a valid field ID
                    T.type = check_keyword_or_id(T.lexeme, TK_FIELDID);
                    return T;
                }
                break;

            case 13:
                if ((c >= 'b' && c <= 'd') || (c >= '2' && c <= '7')) {
                    T.lexeme[i++] = c;
                    state = 13;
                } else {
                    retract();
                    if (strlen(T.lexeme) < 2 || strlen(T.lexeme) > 20) {
                        T.type = TK_ERROR;
                        T.err = ERR_ID_LONG;
                        return T;
                    }
                    T.type = check_keyword_or_id(T.lexeme, TK_ID);
                    return T;
                }
                break;

            case 15:
                if (islower(c)) {
                    T.lexeme[i++] = c;
                    state = 15;
                } else {
                    retract();
                    T.type = check_keyword_or_id(T.lexeme, TK_FIELDID);
                    return T;
                }
                break;

            // --- Function Identifiers ---
            case 17:
                if (isalpha(c)) { T.lexeme[i++] = c; state = 18; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_SYMBOL; return T; }
                break;

            case 18:
                if (isalpha(c)) { T.lexeme[i++] = c; state = 18; }
                else if (isdigit(c)) { T.lexeme[i++] = c; state = 19; }
                else {
                    retract();
                    if (strlen(T.lexeme) > 30) { T.type = TK_ERROR; T.err = ERR_FUNID_LONG; return T; }
                    T.type = check_keyword_or_id(T.lexeme, TK_FUNID);
                    return T;
                }
                break;

            case 19:
                if (isdigit(c)) { T.lexeme[i++] = c; state = 19; }
                else {
                    retract();
                    if (strlen(T.lexeme) > 30) { T.type = TK_ERROR; T.err = ERR_FUNID_LONG; return T; }
                    T.type = TK_FUNID;
                    return T;
                }
                break;

            // --- Record Identifiers ---
            case 21:
                if (islower(c)) { T.lexeme[i++] = c; state = 22; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_SYMBOL; return T; }
                break;

            case 22:
                if (islower(c)) { T.lexeme[i++] = c; state = 22; }
                else { retract(); T.type = TK_RUID; return T; }
                break;

            // --- Relational & Assignment Operators ---
            case 24:
                if (c == '-') { T.lexeme[i++] = c; state = 25; }
                else if (c == '=') { strcat(T.lexeme, "="); T.type = TK_LE; return T; }
                else { retract(); T.type = TK_LT; return T; }
                break;

            case 25:
                if (c == '-') { T.lexeme[i++] = c; state = 26; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 26:
                if (c == '-') { strcat(T.lexeme, "-"); T.type = TK_ASSIGNOP; return T; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 33:
                if (c == '&') { T.lexeme[i++] = c; state = 34; }
                else { T.type = TK_ERROR; T.err = ERR_UNKNOWN_SYMBOL; return T; }
                break;

            case 34:
                if (c == '&') { strcat(T.lexeme, "&"); T.type = TK_AND; return T; }
                else { T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 36:
                if (c == '@') { T.lexeme[i++] = c; state = 37; }
                else { T.type = TK_ERROR; T.err = ERR_UNKNOWN_SYMBOL; return T; }
                break;

            case 37:
                if (c == '@') { strcat(T.lexeme, "@"); T.type = TK_OR; return T; }
                else { T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 30:
                if (c == '=') { strcat(T.lexeme, "="); T.type = TK_EQ; return T; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 42:
                if (c == '=') { strcat(T.lexeme, "="); T.type = TK_NE; return T; }
                else { retract(); T.type = TK_ERROR; T.err = ERR_UNKNOWN_PATTERN; return T; }
                break;

            case 45:
                if (c == '=') { strcat(T.lexeme, "="); T.type = TK_GE; return T; }
                else { retract(); T.type = TK_GT; return T; }
                break;

            // --- Comments ---
            case 50:
                if (c == '\n' || c == EOF) {
                    retract();
                    T.type = TK_COMMENT;
                    return T;
                } else {
                    state = 50; // Keep consuming comment chars
                }
                break;
        }
    }
}
