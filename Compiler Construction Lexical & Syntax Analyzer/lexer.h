// CS F363: Compiler Construction
// Group no. 37
// 2022B4A70496P Garvit Singhal
// 2023A7PS0577P Raunit Raj
// 2023A7PS0613P Rishita Sachan
// 2023A7PS0622P Ranvijay Tanwar
// 2023A7PS0655P Rishabh Sharma
// 2023A7PS0605P Aayush Garg

/***********************************************************
 * lexer.h
 * * Function prototypes for the Lexical Analyzer.
 * This interface exposes the DFA and buffer initialization
 * to the parser and the main driver.
 ***********************************************************/

#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "lexerDef.h"

// Exposing the token names array for the driver to print
extern const char* tokenNames[];

// Global line number tracker, updated as the lexer consumes newlines
extern int lineNum;

// --- Core Lexer Functions ---

// Initializes the keyword array/symbol table
void populate_keywords();

// Sets up the twin buffer system and loads the first chunk of the file
void init_buffers(FILE *fp);

// The main DFA engine: reads the buffer and returns the next token
Token get_next_token();

// Utility for Driver Option 1: strips comments and outputs a clean file
void removeComments(char *testcaseFile, char *cleanFile);

#endif
