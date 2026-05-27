# Compiler-Construction-Lexical-and-Syntax-Analyzer
A custom compiler frontend built from scratch in C. It features a fast twin-buffer lexer, a robust LL(1) parser with smart error recovery and full parse tree generation


Overview

This repository contains the complete Stage 1 frontend pipeline of a custom compiler written entirely in C. It implements a fully functional Lexical Analyzer and a Table-Driven LL(1) Predictive Parser for a custom, statically-typed programming language. The language features specific lexical patterns for variables, functions, and records, alongside unique operators (e.g., <--- for assignment, &&& for logical AND).
The compiler successfully tokenizes source code, builds an N-ary Parse Tree, and features robust Panic-Mode error recovery to handle and report syntax errors gracefully.  


Key Features

1. Lexical Analyzer (Scanner)
DFA-Based Tokenization: Custom deterministic finite automaton (DFA) built from scratch to recognize complex language-specific regex patterns (e.g., variables like [b-d][2-7][b-d]*[2-7]* and functions like _[a-zA-Z]+[0-9]*).
Twin-Buffer System: Optimized disk I/O using a 4KB chunked twin-buffer architecture, ensuring highly efficient character reading and double-retraction handling for overlapping operator prefixes.  
Symbol Table: Integrated fast hash-table to manage language keywords and user-defined identifiers.

3. Syntax Analyzer (Parser)
LL(1) Predictive Parsing: A fast, table-driven pushdown automaton (PDA) that verifies the grammatical structure of the token stream.  
Grammar Engineering: The provided language grammar was manually restructured (89 rules) to eliminate left-recursion and apply left-factoring, making it 100% LL(1) compliant.  
Macro-Driven Architecture: Grammar rules are cleanly mapped into the parsing engine using custom C macros, bridging the gap between theoretical FIRST/FOLLOW sets and codebase implementation.  
Panic-Mode Error Recovery: Instead of crashing on syntax errors, the parser uses synchronization sets (e.g., TK_END, TK_ENDWHILE, TK_SEM) to skip malformed tokens, recover the stack, and continue parsing to report multiple errors in a single pass.

4. Parse Tree ADT (Abstract Syntax Tree Foundation)
N-ary Tree Implementation: A robust tree data structure to represent the parsed grammar.  
Advanced Traversals: Built-in support for Pre-order, Post-order, and Level-order (BFS) traversals.  
Serialization & Visualization: Features the ability to serialize trees to disk, and print highly readable ASCII/Unicode box-drawn representations directly to the console for debugging.


Project Structure

driver.c: The main orchestrator featuring an interactive CLI menu.  
lexer.c / lexer.h: The DFA engine, twin-buffer logic, and comment-stripping utility.  
parser.c / parser.h: The LL(1) parsing table, stack logic, and panic-mode recovery heuristics.  
tree.c / tree.h: The N-ary parse tree ADT and visualization tools.  
stack.c: Array-based Stack ADT to power the LL(1) derivation state machine. 
predictive_parsing_table.html: A custom web-based UI to explore the LL(1) grammar, FIRST/FOLLOW sets, and the parse table matrix.
