// CS F363: Compiler Construction
// Group no. 37
// 2022B4A70496P Garvit Singhal
// 2023A7PS0577P Raunit Raj
// 2023A7PS0613P Rishita Sachan
// 2023A7PS0622P Ranvijay Tanwar
// 2023A7PS0655P Rishabh Sharma
// 2023A7PS0605P Aayush Garg

/***********************************************************
 * parser.h
 *
 * Function prototypes and global exposures for the
 * Syntax Analyzer (Parser). This interface allows the driver
 * to initialize the LL(1) parse table and trigger the
 * parsing of the source code.
 ***********************************************************/

#ifndef PARSER_H
#define PARSER_H

#include "parserDef.h"

// ==========================================
// Error Tracking
// ==========================================
// We expose the error count so the driver can check it after parsing.
// If errorCount > 0, the driver will skip printing the tree.

extern int errorCount;

// Writes all collected lexical/syntax errors to the required listoferrors file
void printErrorsToFile(char *inputFilename);

// Dumps the errors directly to the terminal
void printErrorsToConsole(void);


// ==========================================
// Grammar and Parse Table
// ==========================================

extern GrammarRule ruleTable[100];
extern const char *NT_NAMES[];

// Hardcodes the manually computed FIRST/FOLLOW sets into our LL(1) mapping
void initParseTable(void);

// Helper function to look up the correct rule number in the matrix
int getParseTableEntry(int nt, int terminal);

// sets up the stack, calls the lexer, and builds the N-ary tree
parseTreeNode* parseInputSourceCode(char *filename);

// Traverses the completed tree inorder and prints the required table format to a file
void printParseTree(parseTreeNode *root, char *outfile);

#endif
