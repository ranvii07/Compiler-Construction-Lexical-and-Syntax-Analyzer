/*  ============================================================
 *  CS F363 – Compiler Construction | BITS Pilani 2026
 *  FILE : test_tree.c
 *  DESC : Test suite for the Parse Tree ADT (tree.h / tree.c).
 *
 *  Compile & run:
 *    gcc -Wall -Wextra -Wno-unused-parameter \
 *        -o test_tree test_tree.c tree.c stack.c parser.c
 *    ./test_tree
 *    ./test_tree quiet       (suppress traversal output)
 *    ./test_tree <source>    (parse a file, print its tree)
 *  ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parserDef.h"
#include "parser.h"
#include "stack.h"
#include "tree.h"

/* ── test infrastructure ── */
static int  _pass = 0, _fail = 0;
static int  _quiet = 0;

#define ASSERT(cond, msg) do {                                      \
    if (cond) { printf("  [PASS] %s\n", msg); _pass++; }           \
    else      { printf("  [FAIL] %s\n", msg); _fail++; }           \
} while(0)

/* ── Helpers ── */
static Token *mkTok(Terminal t, const char *lex, int ln) {
    Token *tok = (Token *)calloc(1, sizeof(Token));
    tok->type = t; tok->lineNumber = ln;
    strncpy(tok->lexeme, lex, MAX_LEXEME - 1);
    return tok;
}

/*  Build a full parse tree for:
 *    _main
 *    type int : b2 ;
 *    type real : c3 ;
 *    b2 <--- b2 + c3 * b2 ;
 *    write(b2) ;
 *    return ;
 *    end
 */
static ParseTreeNode *buildTestTree(void) {
    Token *ts[32]; int i = 0;
    ts[i++] = mkTok(TK_MAIN,   "_main",   1);
    /* decl 1 */
    ts[i++] = mkTok(TK_TYPE,   "type",    2);
    ts[i++] = mkTok(TK_INT,    "int",     2);
    ts[i++] = mkTok(TK_COLON,  ":",       2);
    ts[i++] = mkTok(TK_ID,     "b2",      2);
    ts[i++] = mkTok(TK_SEM,    ";",       2);
    /* decl 2 */
    ts[i++] = mkTok(TK_TYPE,   "type",    3);
    ts[i++] = mkTok(TK_REAL,   "real",    3);
    ts[i++] = mkTok(TK_COLON,  ":",       3);
    ts[i++] = mkTok(TK_ID,     "c3",      3);
    ts[i++] = mkTok(TK_SEM,    ";",       3);
    /* assign: b2 <--- b2 + c3 * b2 ; */
    ts[i++] = mkTok(TK_ID,     "b2",      4);
    ts[i++] = mkTok(TK_ASSIGNOP,"<---",   4);
    ts[i++] = mkTok(TK_ID,     "b2",      4);
    ts[i++] = mkTok(TK_PLUS,   "+",       4);
    ts[i++] = mkTok(TK_ID,     "c3",      4);
    ts[i++] = mkTok(TK_MUL,    "*",       4);
    ts[i++] = mkTok(TK_ID,     "b2",      4);
    ts[i++] = mkTok(TK_SEM,    ";",       4);
    /* write */
    ts[i++] = mkTok(TK_WRITE,  "write",   5);
    ts[i++] = mkTok(TK_OP,     "(",       5);
    ts[i++] = mkTok(TK_ID,     "b2",      5);
    ts[i++] = mkTok(TK_CL,     ")",       5);
    ts[i++] = mkTok(TK_SEM,    ";",       5);
    /* return; end */
    ts[i++] = mkTok(TK_RETURN, "return",  6);
    ts[i++] = mkTok(TK_SEM,    ";",       6);
    ts[i++] = mkTok(TK_END,    "end",     7);
    ts[i++] = mkTok(TK_EOF,    "$",       7);
    int count = i;

    /* Suppress parser output during tree tests */
    FILE *devnull = fopen("/dev/null", "w");
    int saved = dup(1);
    dup2(fileno(devnull), 1);

    ParseTreeNode *root = parse(ts, count);

    dup2(saved, 1);
    fclose(devnull);
    close(saved);

    for (int j = 0; j < count; j++) free(ts[j]);
    return root;
}

/* ═══════════════════════════════════════════════════════════════
 * TEST GROUPS
 * ═══════════════════════════════════════════════════════════════ */

/* ─── TEST 1: Node creation & lifecycle ─── */
static void testNodeLifecycle(void) {
    printf("\n══════════════════════════════════════════\n");
    printf("  TEST 1 – Node Lifecycle (create / clone / free)\n");
    printf("══════════════════════════════════════════\n");

    /* treeNewTerminal */
    ParseTreeNode *tn = treeNewTerminal(TK_ID, "b2", 5);
    ASSERT(tn != NULL,                               "treeNewTerminal returns non-NULL");
    ASSERT(tn->symbol.type  == SYM_TERMINAL,         "symbol type is SYM_TERMINAL");
    ASSERT(tn->symbol.value == TK_ID,                "symbol value is TK_ID");
    ASSERT(strcmp(tn->lexeme, "b2") == 0,            "lexeme is 'b2'");
    ASSERT(tn->lineNumber == 5,                       "lineNumber is 5");
    ASSERT(tn->numChildren == 0,                     "numChildren is 0");
    ASSERT(tn->parent == NULL,                       "parent is NULL");

    /* treeNewNonTerminal */
    ParseTreeNode *nn = treeNewNonTerminal(NT_STMTS);
    ASSERT(nn != NULL,                               "treeNewNonTerminal returns non-NULL");
    ASSERT(nn->symbol.type  == SYM_NON_TERMINAL,     "symbol type is SYM_NON_TERMINAL");
    ASSERT(nn->symbol.value == NT_STMTS,             "symbol value is NT_STMTS");

    /* treeNewEpsilon */
    ParseTreeNode *en = treeNewEpsilon();
    ASSERT(en != NULL,                               "treeNewEpsilon returns non-NULL");
    ASSERT(en->symbol.type  == SYM_EPSILON,          "symbol type is SYM_EPSILON");
    ASSERT(treeIsEpsilon(en),                        "treeIsEpsilon returns 1");

    /* treeClone */
    ParseTreeNode *cl = treeClone(tn);
    ASSERT(cl != tn,                                 "clone is a different pointer");
    ASSERT(cl->symbol.type  == tn->symbol.type,      "clone: same symbol type");
    ASSERT(cl->symbol.value == tn->symbol.value,     "clone: same symbol value");
    ASSERT(strcmp(cl->lexeme, tn->lexeme) == 0,      "clone: same lexeme");
    ASSERT(cl->parent == NULL,                       "clone: parent is NULL");

    treeFree(tn); treeFree(nn); treeFree(en); treeFree(cl);
    printf("  (All nodes freed successfully)\n");
}

/* ─── TEST 2: Tree construction helpers ─── */
static void testConstruction(void) {
    printf("\n══════════════════════════════════════════\n");
    printf("  TEST 2 – Tree Construction Helpers\n");
    printf("══════════════════════════════════════════\n");

    ParseTreeNode *root  = treeNewNonTerminal(NT_PROGRAM);
    ParseTreeNode *child1 = treeNewNonTerminal(NT_OTHER_FUNCTIONS);
    ParseTreeNode *child2 = treeNewNonTerminal(NT_MAIN_FUNCTION);
    ParseTreeNode *gc     = treeNewTerminal(TK_MAIN, "_main", 1);

    /* addChild */
    treeAddChild(root, child1);
    treeAddChild(root, child2);
    treeAddChild(child2, gc);

    ASSERT(root->numChildren == 2,    "root has 2 children after addChild");
    ASSERT(child1->parent == root,    "child1->parent == root");
    ASSERT(child2->parent == root,    "child2->parent == root");
    ASSERT(gc->parent == child2,      "grandchild->parent == child2");
    ASSERT(child2->numChildren == 1,  "child2 has 1 grandchild");

    /* treeDepthOf */
    ASSERT(treeDepthOf(root)  == 0,   "root depth == 0");
    ASSERT(treeDepthOf(child1) == 1,  "child depth == 1");
    ASSERT(treeDepthOf(gc) == 2,      "grandchild depth == 2");

    /* treeIsLeaf */
    ASSERT(!treeIsLeaf(root),         "root is NOT leaf");
    ASSERT( treeIsLeaf(child1),       "empty NT is leaf (no children)");
    ASSERT( treeIsLeaf(gc),           "terminal is leaf");

    /* treeRemoveChildAt */
    ParseTreeNode *removed = treeRemoveChildAt(root, 0);
    ASSERT(removed == child1,         "removeChildAt(0) returned child1");
    ASSERT(removed->parent == NULL,   "removed node parent is NULL");
    ASSERT(root->numChildren == 1,    "root now has 1 child");
    ASSERT(root->children[0] == child2, "child2 shifted to index 0");

    /* treeInsertChildAt */
    treeInsertChildAt(root, child1, 0);
    ASSERT(root->numChildren == 2,           "insertChildAt restored 2 children");
    ASSERT(root->children[0] == child1,      "child1 at index 0");
    ASSERT(root->children[1] == child2,      "child2 at index 1");

    /* treeDetach */
    treeDetach(gc);
    ASSERT(gc->parent == NULL,                "detached node parent == NULL");
    ASSERT(child2->numChildren == 0,          "child2 has 0 children after detach");

    /* treeReplace */
    ParseTreeNode *newChild = treeNewTerminal(TK_END, "end", 99);
    treeAddChild(root, newChild);  /* add at index 2 */
    treeReplace(newChild, gc);
    ASSERT(root->children[2] == gc,           "replacement is gc");
    ASSERT(gc->parent == root,                "gc parent updated to root");
    ASSERT(newChild->parent == NULL,          "replaced node detached");

    treeFree(newChild);
    treeFree(root);  /* recursively frees child1, child2, gc */
}

/* ─── TEST 3: Traversals ─── */
typedef struct { int *order; int count; } TraceCtx;

static int traceVisitor(ParseTreeNode *node, int depth, void *ud) {
    (void)depth;
    TraceCtx *ctx = (TraceCtx *)ud;
    if (node->symbol.type == SYM_TERMINAL)
        ctx->order[ctx->count++] = node->symbol.value;
    return 0;
}

static void testTraversals(void) {
    printf("\n══════════════════════════════════════════\n");
    printf("  TEST 3 – Traversals (pre / post / level)\n");
    printf("══════════════════════════════════════════\n");

    /* Build a tiny hand-crafted tree:
     *          program
     *         /       \
     *       INT       REAL
     *      /
     *    PLUS
     */
    ParseTreeNode *root   = treeNewNonTerminal(NT_PROGRAM);
    ParseTreeNode *left   = treeNewTerminal(TK_INT, "int", 1);
    ParseTreeNode *right  = treeNewTerminal(TK_REAL, "real", 1);
    ParseTreeNode *ll     = treeNewTerminal(TK_PLUS, "+", 1);
    treeAddChild(root,  left);
    treeAddChild(root,  right);
    treeAddChild(left,  ll);

    /* Expected pre-order terminal sequence: INT, PLUS, REAL */
    int preSeq[8]; TraceCtx preCtx = { preSeq, 0 };
    treePreorder(root, traceVisitor, &preCtx);
    ASSERT(preCtx.count == 3,         "Pre-order visits 3 terminals");
    ASSERT(preSeq[0] == TK_INT,       "Pre-order[0] == TK_INT");
    ASSERT(preSeq[1] == TK_PLUS,      "Pre-order[1] == TK_PLUS");
    ASSERT(preSeq[2] == TK_REAL,      "Pre-order[2] == TK_REAL");

    /* Expected post-order terminal sequence: PLUS, INT, REAL */
    int postSeq[8]; TraceCtx postCtx = { postSeq, 0 };
    treePostorder(root, traceVisitor, &postCtx);
    ASSERT(postCtx.count == 3,        "Post-order visits 3 terminals");
    ASSERT(postSeq[0] == TK_PLUS,     "Post-order[0] == TK_PLUS");
    ASSERT(postSeq[1] == TK_INT,      "Post-order[1] == TK_INT");
    ASSERT(postSeq[2] == TK_REAL,     "Post-order[2] == TK_REAL");

    /* Level-order: INT, REAL, PLUS */
    int lvlSeq[8]; TraceCtx lvlCtx = { lvlSeq, 0 };
    treeLevelorder(root, traceVisitor, &lvlCtx);
    ASSERT(lvlCtx.count == 3,         "Level-order visits 3 terminals");
    ASSERT(lvlSeq[0] == TK_INT,       "Level-order[0] == TK_INT");
    ASSERT(lvlSeq[1] == TK_REAL,      "Level-order[1] == TK_REAL");
    ASSERT(lvlSeq[2] == TK_PLUS,      "Level-order[2] == TK_PLUS");

    treeFree(root);
}

/* ─── TEST 4: Statistics ─── */
static void testStatistics(ParseTreeNode *root) {
    printf("\n══════════════════════════════════════════\n");
    printf("  TEST 4 – Tree Statistics\n");
    printf("══════════════════════════════════════════\n");

    ASSERT(root != NULL, "Parse tree root is not NULL");

    int h = treeHeight(root);
    int n = treeNodeCount(root);
    int l = treeLeafCount(root);
    printf("  Height      : %d\n", h);
    printf("  Node count  : %d\n", n);
    printf("  Leaf count  : %d\n", l);

    ASSERT(h > 0,     "Tree height > 0");
    ASSERT(n > 0,     "Node count > 0");
    ASSERT(l > 0,     "Leaf count > 0");
    ASSERT(l <= n,    "Leaf count ≤ total nodes");

    TreeStats s = treeGetStats(root);
    ASSERT(s.nodeCount     == n,   "getStats nodeCount matches treeNodeCount");
    ASSERT(s.height        == h,   "getStats height matches treeHeight");
    ASSERT(s.leafCount     >= l,   "getStats leafCount ≥ basic leafCount");
    ASSERT(s.internalCount + s.leafCount == s.nodeCount,
                                   "internal + leaf == total");
    ASSERT(s.epsilonCount + s.terminalCount == s.leafCount,
                                   "epsilon + terminal == leafCount");
    ASSERT(s.maxWidth >= 1,        "maxWidth ≥ 1");

    if (!_quiet) treePrintStats(&s, stdout);
}

/* ─── TEST 5: Search & query ─── */
static void testSearch(ParseTreeNode *root) {
    printf("\n══════════════════════════════════════════\n");
    printf("  TEST 5 – Search & Query\n");
    printf("══════════════════════════════════════════\n");

    /* Find first TK_ID token */
    ParseTreeNode *id = treeFindByTerminal(root, TK_ID);
    ASSERT(id != NULL,                  "treeFindByTerminal(TK_ID) found a node");
    ASSERT(id->symbol.type == SYM_TERMINAL, "found node is a terminal");
    ASSERT(id->symbol.value == TK_ID,   "found node value is TK_ID");
    printf("  First TK_ID: lexeme='%s' line=%d\n",
           id->lexeme, id->lineNumber);

    /* Find all TK_ID occurrences */
    int idCount = 0;
    ParseTreeNode **ids = treeFindAllByTerminal(root, TK_ID, &idCount);
    ASSERT(ids != NULL,                 "treeFindAllByTerminal returned array");
    ASSERT(idCount > 0,                 "Found at least 1 TK_ID");
    ASSERT(ids[idCount] == NULL,        "Array is NULL-terminated");
    printf("  Total TK_ID leaves    : %d\n", idCount);
    free(ids);

    /* Find first <stmts> non-terminal */
    ParseTreeNode *stmts = treeFindByNT(root, NT_STMTS);
    ASSERT(stmts != NULL,               "treeFindByNT(NT_STMTS) found a node");
    ASSERT(stmts->symbol.type == SYM_NON_TERMINAL, "Found node is NT");

    /* Find all NT_DECLARATIONS */
    int declCount = 0;
    ParseTreeNode **decls = treeFindAllByNT(root, NT_DECLARATIONS, &declCount);
    ASSERT(decls != NULL && declCount > 0, "treeFindAllByNT found declarations");
    printf("  NT_DECLARATIONS nodes : %d\n", declCount);
    free(decls);

    /* treeIsLeaf, treeIsTokenLeaf */
    ParseTreeNode *mainTok = treeFindByTerminal(root, TK_MAIN);
    ASSERT(mainTok != NULL,              "Found TK_MAIN leaf");
    ASSERT(treeIsLeaf(mainTok),          "TK_MAIN node is a leaf");
    ASSERT(treeIsTokenLeaf(mainTok),     "TK_MAIN is a token leaf");
    ASSERT(!treeIsTokenLeaf(root),       "root is NOT a token leaf");

    /* treeEqual: clone should be equal */
    ParseTreeNode *clone = treeClone(stmts);
    ASSERT(treeEqual(stmts, clone),      "treeEqual: clone equals original");
    /* Modify clone and check not-equal */
    if (clone && clone->numChildren > 0) {
        ParseTreeNode *leaf = clone;
        while (leaf->numChildren > 0) leaf = leaf->children[0];
        if (leaf->symbol.type == SYM_TERMINAL) {
            leaf->symbol.value = (leaf->symbol.value + 1) % NUM_TERMINALS;
            ASSERT(!treeEqual(stmts, clone),
                   "treeEqual: modified clone ≠ original");
        }
    }
    treeFree(clone);
}

/* ─── TEST 6: All print styles ─── */
static void testPrinting(ParseTreeNode *root) {
    printf("\n══════════════════════════════════════════\n");
    printf("  TEST 6 – Print Styles\n");
    printf("══════════════════════════════════════════\n");

    if (!_quiet) {
        printf("\n─── PRINT_BOXED (depth ≤ 6) ───\n");
        treePrint(root, PRINT_BOXED, 6, stdout);

        printf("─── PRINT_COMPACT ───\n");
        treePrint(root, PRINT_COMPACT, -1, stdout);

        printf("─── PRINT_SIMPLE (depth ≤ 5) ───\n");
        treePrint(root, PRINT_SIMPLE, 5, stdout);

        printf("─── PRINT_LEAVES (token stream) ───\n");
        treePrint(root, PRINT_LEAVES, -1, stdout);
    }

    /* Verify printing doesn't crash (write to /dev/null) */
    FILE *nul = fopen("/dev/null", "w");
    treePrint(root, PRINT_BOXED,    -1, nul);
    treePrint(root, PRINT_COMPACT,  -1, nul);
    treePrint(root, PRINT_SIMPLE,   -1, nul);
    treePrint(root, PRINT_LEAVES,   -1, nul);
    fclose(nul);
    ASSERT(1, "All print styles complete without crash");
}

/* ─── TEST 7: Serialisation round-trip ─── */
static void testSerialization(ParseTreeNode *root) {
    printf("\n══════════════════════════════════════════\n");
    printf("  TEST 7 – Serialise → File → Deserialise\n");
    printf("══════════════════════════════════════════\n");

    const char *filename = "/tmp/cs363_parsetree.txt";

    int r = treeSerialize(root, filename);
    ASSERT(r == 0,   "treeSerialize returned 0 (success)");

    /* Read it back */
    ParseTreeNode *loaded = treeDeserialize(filename);
    ASSERT(loaded != NULL,  "treeDeserialize returned non-NULL root");

    /* Compare node counts */
    int origCount   = treeNodeCount(root);
    int loadedCount = treeNodeCount(loaded);
    printf("  Original node count : %d\n", origCount);
    printf("  Loaded   node count : %d\n", loadedCount);
    ASSERT(origCount == loadedCount, "Node count preserved through serialize→load");

    /* Compare heights */
    ASSERT(treeHeight(root) == treeHeight(loaded),
           "Tree height preserved through serialize→load");

    /* Structural equality check */
    ASSERT(treeEqual(root, loaded),
           "treeEqual: original == loaded");

    treeFree(loaded);

    /* Show the file contents (first 10 lines) */
    if (!_quiet) {
        printf("\n  First 10 lines of '%s':\n", filename);
        FILE *fp = fopen(filename, "r");
        char buf[256]; int line = 0;
        while (fgets(buf, sizeof buf, fp) && line < 10) {
            printf("    %s", buf); line++;
        }
        fclose(fp);
    }
}

/* ═══════════════════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    initGrammarRules();
    initParseTable();

    if (argc > 1 && strcmp(argv[1], "quiet") == 0) _quiet = 1;

    printf("╔═════════════════════════════════════════════╗\n");
    printf("║   CS F363 – Parse Tree ADT Test Suite       ║\n");
    printf("║   BITS Pilani 2026                          ║\n");
    printf("╚═════════════════════════════════════════════╝\n");

    /* Tests 1-3: don't need a parser-built tree */
    testNodeLifecycle();
    testConstruction();
    testTraversals();

    /* Build a full tree for tests 4-7 */
    printf("\n[Building test parse tree from source program...]\n");
    ParseTreeNode *root = buildTestTree();

    if (!root) {
        printf("[ERROR] Could not build parse tree. Aborting.\n");
        return 1;
    }
    printf("[OK] Parse tree built successfully.\n");

    testStatistics(root);
    testSearch(root);
    testPrinting(root);
    testSerialization(root);

    treeFree(root);

    /* ── Summary ── */
    printf("\n╔═════════════════════════════════════════════╗\n");
    printf("║   Results: %3d passed   %3d failed          ║\n",
           _pass, _fail);
    printf("╚═════════════════════════════════════════════╝\n\n");

    return _fail > 0 ? 1 : 0;
}
