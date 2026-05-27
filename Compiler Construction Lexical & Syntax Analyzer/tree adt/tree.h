/*  ============================================================
 *  CS F363 – Compiler Construction | BITS Pilani 2026
 *  FILE : tree.h
 *  DESC : Parse Tree ADT – complete interface.
 *
 *  Builds on top of ParseTreeNode (defined in parser.h) and
 *  provides all higher-level tree operations needed by the
 *  compiler pipeline:
 *
 *    Section A – Node lifecycle  (create / clone / free)
 *    Section B – Tree construction helpers
 *    Section C – Traversal engine  (pre / post / in / level)
 *    Section D – Tree statistics  (height / width / counts)
 *    Section E – Search & query
 *    Section F – Printing  (compact / boxed / rule-only)
 *    Section G – Serialisation  (write to / read from file)
 *
 *  All functions operate on   ParseTreeNode *   pointers so
 *  the parser can hand its root directly to this module with
 *  no conversion step.
 *  ============================================================ */

#ifndef TREE_H
#define TREE_H

#include "parser.h"   /* ParseTreeNode, Symbol, Terminal, NonTerminal */
#include <stdio.h>

/* ═══════════════════════════════════════════════════════════════
 * TRAVERSAL ORDER CONSTANTS
 * ═══════════════════════════════════════════════════════════════ */
typedef enum {
    TRAVERSAL_PREORDER,    /* root → children left→right         */
    TRAVERSAL_POSTORDER,   /* children left→right → root         */
    TRAVERSAL_LEVELORDER   /* BFS level by level                 */
} TraversalOrder;

/* ═══════════════════════════════════════════════════════════════
 * VISITOR CALLBACK
 *   Passed to traversal functions.  Return 0 to continue, non-0
 *   to stop early (like a short-circuit).
 * ═══════════════════════════════════════════════════════════════ */
typedef int (*TreeVisitor)(ParseTreeNode *node, int depth, void *userdata);

/* ═══════════════════════════════════════════════════════════════
 * TREE STATISTICS
 * ═══════════════════════════════════════════════════════════════ */
typedef struct {
    int height;            /* longest root-to-leaf path (edges)  */
    int nodeCount;         /* total nodes (including ε nodes)    */
    int internalCount;     /* non-terminal internal nodes        */
    int leafCount;         /* terminal + ε leaf nodes            */
    int epsilonCount;      /* ε nodes specifically               */
    int terminalCount;     /* terminal leaf nodes                */
    int maxWidth;          /* maximum nodes at any single level  */
    int ruleCount[NUM_RULES + 1]; /* how many times each rule fired */
} TreeStats;

/* ═══════════════════════════════════════════════════════════════
 * PRINT STYLE
 * ═══════════════════════════════════════════════════════════════ */
typedef enum {
    PRINT_SIMPLE,    /* indented text, | -- connectors           */
    PRINT_BOXED,     /* Unicode box-drawing, coloured labels     */
    PRINT_COMPACT,   /* one line per rule: LHS → RHS symbols     */
    PRINT_LEAVES     /* only leaf nodes (token stream view)      */
} PrintStyle;

/* ═══════════════════════════════════════════════════════════════
 * SECTION A – NODE LIFECYCLE
 * ═══════════════════════════════════════════════════════════════ */

/*  Allocate a new node for the given symbol.
 *  Sets parent = NULL, numChildren = 0, ruleApplied = 0.
 *  Aborts on allocation failure.                                */
ParseTreeNode *treeNewNode(Symbol sym);

/*  Convenience wrappers                                         */
ParseTreeNode *treeNewTerminal(Terminal t, const char *lexeme, int line);
ParseTreeNode *treeNewNonTerminal(NonTerminal nt);
ParseTreeNode *treeNewEpsilon(void);

/*  Deep-copy a subtree.  Parent pointer of the copy's root = NULL. */
ParseTreeNode *treeClone(const ParseTreeNode *node);

/*  Free an entire subtree recursively (post-order).            */
void treeFree(ParseTreeNode *root);

/* ═══════════════════════════════════════════════════════════════
 * SECTION B – TREE CONSTRUCTION HELPERS
 * ═══════════════════════════════════════════════════════════════ */

/*  Attach child to parent.  Sets child->parent.
 *  Prints a warning and discards if numChildren == MAX_CHILDREN.
 *  Returns child on success, NULL on failure.                  */
ParseTreeNode *treeAddChild(ParseTreeNode *parent, ParseTreeNode *child);

/*  Remove and return the i-th child (0-based).  Shifts the rest.
 *  Returns NULL if index is out of range.                      */
ParseTreeNode *treeRemoveChildAt(ParseTreeNode *parent, int index);

/*  Insert child at position i, shifting existing children right.
 *  Returns child on success, NULL on failure.                  */
ParseTreeNode *treeInsertChildAt(ParseTreeNode *parent,
                                  ParseTreeNode *child, int index);

/*  Detach a node from its parent (makes it a new root).        */
void treeDetach(ParseTreeNode *node);

/*  Replace node with replacement inside its parent.
 *  replacement inherits node's position; node is detached.     */
void treeReplace(ParseTreeNode *node, ParseTreeNode *replacement);

/* ═══════════════════════════════════════════════════════════════
 * SECTION C – TRAVERSAL ENGINE
 * ═══════════════════════════════════════════════════════════════ */

/*  General traversal.  Calls visitor(node, depth, userdata) for
 *  every node in the requested order.  Returns the accumulated
 *  non-zero return value from the first early-stop, or 0.      */
int treeTraverse(ParseTreeNode *root,
                 TraversalOrder order,
                 TreeVisitor     visitor,
                 void           *userdata);

/*  Convenience wrappers                                         */
int treePreorder  (ParseTreeNode *root, TreeVisitor v, void *ud);
int treePostorder (ParseTreeNode *root, TreeVisitor v, void *ud);
int treeLevelorder(ParseTreeNode *root, TreeVisitor v, void *ud);

/* ═══════════════════════════════════════════════════════════════
 * SECTION D – TREE STATISTICS
 * ═══════════════════════════════════════════════════════════════ */

/*  Compute all statistics in a single pass.                    */
TreeStats treeGetStats(const ParseTreeNode *root);

/*  Individual fast accessors (each does its own single pass)   */
int treeHeight    (const ParseTreeNode *root);   /* 0 = leaf node */
int treeNodeCount (const ParseTreeNode *root);
int treeLeafCount (const ParseTreeNode *root);
int treeDepthOf   (const ParseTreeNode *node);   /* distance from root */

/*  Print the statistics table to fp.                           */
void treePrintStats(const TreeStats *stats, FILE *fp);

/* ═══════════════════════════════════════════════════════════════
 * SECTION E – SEARCH & QUERY
 * ═══════════════════════════════════════════════════════════════ */

/*  Find the first (pre-order) node matching a predicate.
 *  Returns NULL if not found.                                  */
typedef int (*TreePredicate)(const ParseTreeNode *node, void *userdata);
ParseTreeNode *treeFind    (ParseTreeNode *root,
                            TreePredicate  pred, void *userdata);

/*  Collect all nodes matching a predicate into a NULL-terminated
 *  array.  Caller must free() the returned array (not the nodes).
 *  Returns NULL if none found.                                 */
ParseTreeNode **treeFindAll(ParseTreeNode *root,
                            TreePredicate  pred, void *userdata,
                            int           *outCount);

/*  Convenience predicates                                       */
ParseTreeNode  *treeFindByTerminal   (ParseTreeNode *root, Terminal t);
ParseTreeNode  *treeFindByNT         (ParseTreeNode *root, NonTerminal nt);
ParseTreeNode **treeFindAllByTerminal(ParseTreeNode *root, Terminal t,
                                      int *outCount);
ParseTreeNode **treeFindAllByNT      (ParseTreeNode *root, NonTerminal nt,
                                      int *outCount);

/*  Is this node a leaf (no children, or only ε child)?         */
int treeIsLeaf        (const ParseTreeNode *node);
/*  Is this node a terminal token leaf?                         */
int treeIsTokenLeaf   (const ParseTreeNode *node);
/*  Is this node an ε node?                                     */
int treeIsEpsilon     (const ParseTreeNode *node);

/* ═══════════════════════════════════════════════════════════════
 * SECTION F – PRINTING
 * ═══════════════════════════════════════════════════════════════ */

/*  Master printer.  Writes to fp using the selected style.
 *  maxDepth = -1 means unlimited.                              */
void treePrint(const ParseTreeNode *root,
               PrintStyle           style,
               int                  maxDepth,
               FILE                *fp);

/*  Convenience: print to stdout with default (BOXED) style     */
void treePrintStdout(const ParseTreeNode *root);

/*  Print only the token stream (leaf nodes in order)           */
void treePrintTokenStream(const ParseTreeNode *root, FILE *fp);

/*  Print a summary line for a single node                      */
void treePrintNode(const ParseTreeNode *node, FILE *fp);

/* ═══════════════════════════════════════════════════════════════
 * SECTION G – SERIALISATION
 * ═══════════════════════════════════════════════════════════════ */

/*  Write the tree to a text file in indented format.
 *  Returns 0 on success, -1 on error.                         */
int  treeSerialize  (const ParseTreeNode *root, const char *filename);

/*  Read back a serialised tree.
 *  Returns root node or NULL on error.                         */
ParseTreeNode *treeDeserialize(const char *filename);

/* ═══════════════════════════════════════════════════════════════
 * SECTION H – UTILITY
 * ═══════════════════════════════════════════════════════════════ */

/*  Return a human-readable label for any node (static buffer). */
const char *treeNodeLabel(const ParseTreeNode *node);

/*  Return 1 if two subtrees are structurally identical
 *  (same symbols, same lexemes for leaves).                    */
int treeEqual(const ParseTreeNode *a, const ParseTreeNode *b);

#endif /* TREE_H */
