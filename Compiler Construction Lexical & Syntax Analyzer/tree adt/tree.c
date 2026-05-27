/*  ============================================================
 *  CS F363 – Compiler Construction | BITS Pilani 2026
 *  FILE : tree.c
 *  DESC : Parse Tree ADT implementation.
 *
 *  Sections:
 *    A. Node lifecycle
 *    B. Tree construction helpers
 *    C. Traversal engine
 *    D. Statistics
 *    E. Search & query
 *    F. Printing (simple / boxed / compact / leaves)
 *    G. Serialisation
 *    H. Utility
 *  ============================================================ */

#include "tree.h"
#include "parserDef.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External name tables defined in parser.c */
extern const char *terminalNames[NUM_TERMINALS];
extern const char *nonTerminalNames[NUM_NON_TERMINALS];
extern GrammarRule grammarRules[NUM_RULES];

/* ═══════════════════════════════════════════════════════════════
 * INTERNAL HELPERS
 * ═══════════════════════════════════════════════════════════════ */

static void oom(void) {
    fprintf(stderr, "[TREE] Fatal: out of memory.\n");
    exit(EXIT_FAILURE);
}

/* Simple queue node for level-order traversal */
typedef struct QNode {
    ParseTreeNode  *treeNode;
    int             depth;
    struct QNode   *next;
} QNode;

typedef struct {
    QNode *head;
    QNode *tail;
    int    size;
} Queue;

static void qEnqueue(Queue *q, ParseTreeNode *n, int depth) {
    QNode *qn = (QNode *)malloc(sizeof(QNode));
    if (!qn) oom();
    qn->treeNode = n;
    qn->depth    = depth;
    qn->next     = NULL;
    if (!q->head) { q->head = q->tail = qn; }
    else          { q->tail->next = qn; q->tail = qn; }
    q->size++;
}

static QNode *qDequeue(Queue *q) {
    if (!q->head) return NULL;
    QNode *qn = q->head;
    q->head   = qn->next;
    if (!q->head) q->tail = NULL;
    q->size--;
    return qn;
}

/* ═══════════════════════════════════════════════════════════════
 * SECTION A – NODE LIFECYCLE
 * ═══════════════════════════════════════════════════════════════ */

ParseTreeNode *treeNewNode(Symbol sym) {
    ParseTreeNode *n = (ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
    if (!n) oom();
    n->symbol      = sym;
    n->numChildren = 0;
    n->parent      = NULL;
    n->ruleApplied = 0;
    n->lineNumber  = -1;
    n->lexeme[0]   = '\0';
    return n;
}

ParseTreeNode *treeNewTerminal(Terminal t, const char *lexeme, int line) {
    Symbol sym = { SYM_TERMINAL, t };
    ParseTreeNode *n = treeNewNode(sym);
    if (lexeme) strncpy(n->lexeme, lexeme, MAX_LEXEME - 1);
    n->lineNumber = line;
    return n;
}

ParseTreeNode *treeNewNonTerminal(NonTerminal nt) {
    Symbol sym = { SYM_NON_TERMINAL, nt };
    return treeNewNode(sym);
}

ParseTreeNode *treeNewEpsilon(void) {
    Symbol sym = { SYM_EPSILON, 0 };
    ParseTreeNode *n = treeNewNode(sym);
    strncpy(n->lexeme, "ε", MAX_LEXEME - 1);
    return n;
}

ParseTreeNode *treeClone(const ParseTreeNode *node) {
    if (!node) return NULL;
    ParseTreeNode *copy = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
    if (!copy) oom();
    memcpy(copy, node, sizeof(ParseTreeNode));
    copy->parent = NULL;
    for (int i = 0; i < node->numChildren; i++) {
        copy->children[i] = treeClone(node->children[i]);
        if (copy->children[i])
            copy->children[i]->parent = copy;
    }
    return copy;
}

void treeFree(ParseTreeNode *root) {
    if (!root) return;
    for (int i = 0; i < root->numChildren; i++)
        treeFree(root->children[i]);
    free(root);
}

/* ═══════════════════════════════════════════════════════════════
 * SECTION B – TREE CONSTRUCTION HELPERS
 * ═══════════════════════════════════════════════════════════════ */

ParseTreeNode *treeAddChild(ParseTreeNode *parent, ParseTreeNode *child) {
    if (!parent || !child) return NULL;
    if (parent->numChildren >= MAX_CHILDREN) {
        fprintf(stderr,
            "[TREE] Warning: MAX_CHILDREN (%d) exceeded for <%s>. "
            "Child discarded.\n",
            MAX_CHILDREN, treeNodeLabel(parent));
        return NULL;
    }
    parent->children[parent->numChildren++] = child;
    child->parent = parent;
    return child;
}

ParseTreeNode *treeRemoveChildAt(ParseTreeNode *parent, int index) {
    if (!parent || index < 0 || index >= parent->numChildren) return NULL;
    ParseTreeNode *removed = parent->children[index];
    /* Shift remaining children left */
    for (int i = index; i < parent->numChildren - 1; i++)
        parent->children[i] = parent->children[i + 1];
    parent->children[--parent->numChildren] = NULL;
    if (removed) removed->parent = NULL;
    return removed;
}

ParseTreeNode *treeInsertChildAt(ParseTreeNode *parent,
                                   ParseTreeNode *child, int index) {
    if (!parent || !child) return NULL;
    if (parent->numChildren >= MAX_CHILDREN) {
        fprintf(stderr, "[TREE] Warning: MAX_CHILDREN exceeded on insert.\n");
        return NULL;
    }
    if (index < 0) index = 0;
    if (index > parent->numChildren) index = parent->numChildren;
    /* Shift right */
    for (int i = parent->numChildren; i > index; i--)
        parent->children[i] = parent->children[i - 1];
    parent->children[index] = child;
    parent->numChildren++;
    child->parent = parent;
    return child;
}

void treeDetach(ParseTreeNode *node) {
    if (!node || !node->parent) return;
    ParseTreeNode *parent = node->parent;
    for (int i = 0; i < parent->numChildren; i++) {
        if (parent->children[i] == node) {
            treeRemoveChildAt(parent, i);
            return;
        }
    }
}

void treeReplace(ParseTreeNode *node, ParseTreeNode *replacement) {
    if (!node || !replacement) return;
    ParseTreeNode *parent = node->parent;
    if (!parent) return;
    for (int i = 0; i < parent->numChildren; i++) {
        if (parent->children[i] == node) {
            parent->children[i] = replacement;
            replacement->parent = parent;
            node->parent        = NULL;
            return;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════
 * SECTION C – TRAVERSAL ENGINE
 * ═══════════════════════════════════════════════════════════════ */

/* Internal pre-order DFS */
static int _preorder(ParseTreeNode *node, int depth,
                      TreeVisitor visitor, void *ud) {
    if (!node) return 0;
    int r = visitor(node, depth, ud);
    if (r) return r;
    for (int i = 0; i < node->numChildren; i++) {
        r = _preorder(node->children[i], depth + 1, visitor, ud);
        if (r) return r;
    }
    return 0;
}

/* Internal post-order DFS */
static int _postorder(ParseTreeNode *node, int depth,
                       TreeVisitor visitor, void *ud) {
    if (!node) return 0;
    for (int i = 0; i < node->numChildren; i++) {
        int r = _postorder(node->children[i], depth + 1, visitor, ud);
        if (r) return r;
    }
    return visitor(node, depth, ud);
}

/* BFS level-order */
static int _levelorder(ParseTreeNode *root, TreeVisitor visitor, void *ud) {
    if (!root) return 0;
    Queue q = { NULL, NULL, 0 };
    qEnqueue(&q, root, 0);
    int result = 0;
    while (q.head) {
        QNode *qn = qDequeue(&q);
        ParseTreeNode *node  = qn->treeNode;
        int            depth = qn->depth;
        free(qn);
        result = visitor(node, depth, ud);
        if (result) break;
        for (int i = 0; i < node->numChildren; i++)
            if (node->children[i])
                qEnqueue(&q, node->children[i], depth + 1);
    }
    /* Drain queue on early exit */
    while (q.head) { QNode *qn = qDequeue(&q); free(qn); }
    return result;
}

int treeTraverse(ParseTreeNode *root, TraversalOrder order,
                 TreeVisitor visitor, void *userdata) {
    if (!root || !visitor) return 0;
    switch (order) {
        case TRAVERSAL_PREORDER:   return _preorder  (root, 0, visitor, userdata);
        case TRAVERSAL_POSTORDER:  return _postorder (root, 0, visitor, userdata);
        case TRAVERSAL_LEVELORDER: return _levelorder(root,    visitor, userdata);
    }
    return 0;
}

int treePreorder  (ParseTreeNode *root, TreeVisitor v, void *ud) {
    return treeTraverse(root, TRAVERSAL_PREORDER, v, ud); }
int treePostorder (ParseTreeNode *root, TreeVisitor v, void *ud) {
    return treeTraverse(root, TRAVERSAL_POSTORDER, v, ud); }
int treeLevelorder(ParseTreeNode *root, TreeVisitor v, void *ud) {
    return treeTraverse(root, TRAVERSAL_LEVELORDER, v, ud); }

/* ═══════════════════════════════════════════════════════════════
 * SECTION D – TREE STATISTICS
 * ═══════════════════════════════════════════════════════════════ */

/* Visitor used to gather stats */
static int _statsVisitor(ParseTreeNode *node, int depth, void *ud) {
    TreeStats *s = (TreeStats *)ud;
    s->nodeCount++;
    if (node->symbol.type == SYM_NON_TERMINAL) {
        s->internalCount++;
        int r = node->ruleApplied;
        if (r >= 1 && r <= NUM_RULES) s->ruleCount[r]++;
    } else {
        s->leafCount++;
        if (node->symbol.type == SYM_EPSILON)  s->epsilonCount++;
        else                                    s->terminalCount++;
    }
    if (depth > s->height) s->height = depth;
    return 0;
}

/* For maxWidth: count nodes per level via level-order */
typedef struct { int *counts; int size; } LevelCounts;

static int _levelWidthVisitor(ParseTreeNode *node, int depth, void *ud) {
    LevelCounts *lc = (LevelCounts *)ud;
    if (depth >= lc->size) {
        int newSize = depth + 64;
        lc->counts = (int *)realloc(lc->counts, newSize * sizeof(int));
        if (!lc->counts) oom();
        for (int i = lc->size; i < newSize; i++) lc->counts[i] = 0;
        lc->size = newSize;
    }
    lc->counts[depth]++;
    return 0;
}

TreeStats treeGetStats(const ParseTreeNode *root) {
    TreeStats s;
    memset(&s, 0, sizeof(s));
    if (!root) return s;

    /* Gather node-level stats via pre-order */
    treePreorder((ParseTreeNode *)root, _statsVisitor, &s);

    /* Compute max width via level-order */
    LevelCounts lc = { NULL, 0 };
    treeLevelorder((ParseTreeNode *)root, _levelWidthVisitor, &lc);
    for (int i = 0; i < lc.size; i++)
        if (lc.counts[i] > s.maxWidth) s.maxWidth = lc.counts[i];
    free(lc.counts);

    return s;
}

/* ─── Simple recursive helpers ─── */
int treeHeight(const ParseTreeNode *root) {
    if (!root || root->numChildren == 0) return 0;
    int max = 0;
    for (int i = 0; i < root->numChildren; i++) {
        int h = treeHeight(root->children[i]);
        if (h > max) max = h;
    }
    return max + 1;
}

int treeNodeCount(const ParseTreeNode *root) {
    if (!root) return 0;
    int c = 1;
    for (int i = 0; i < root->numChildren; i++)
        c += treeNodeCount(root->children[i]);
    return c;
}

int treeLeafCount(const ParseTreeNode *root) {
    if (!root) return 0;
    if (root->numChildren == 0) return 1;
    int c = 0;
    for (int i = 0; i < root->numChildren; i++)
        c += treeLeafCount(root->children[i]);
    return c;
}

int treeDepthOf(const ParseTreeNode *node) {
    if (!node) return -1;
    int d = 0;
    const ParseTreeNode *cur = node;
    while (cur->parent) { d++; cur = cur->parent; }
    return d;
}

void treePrintStats(const TreeStats *s, FILE *fp) {
    fprintf(fp,
        "\n┌─────────────────────────────────────────┐\n"
        "│       CS F363 – Parse Tree Statistics   │\n"
        "├─────────────────────────────────────────┤\n"
        "│  Total nodes        : %-6d             │\n"
        "│  Internal (NT)      : %-6d             │\n"
        "│  Leaf (terminal)    : %-6d             │\n"
        "│  Epsilon nodes      : %-6d             │\n"
        "│  Token leaves       : %-6d             │\n"
        "│  Tree height        : %-6d             │\n"
        "│  Max level width    : %-6d             │\n"
        "├─────────────────────────────────────────┤\n"
        "│  Top rules applied:                     │\n",
        s->nodeCount, s->internalCount, s->leafCount,
        s->epsilonCount, s->terminalCount,
        s->height, s->maxWidth);

    /* Print rules that were applied at least once */
    for (int r = 1; r <= NUM_RULES; r++) {
        if (s->ruleCount[r] > 0) {
            fprintf(fp, "│    G%-3d : %3d time%-1s                         │\n",
                    r, s->ruleCount[r],
                    s->ruleCount[r] == 1 ? " " : "s");
        }
    }
    fprintf(fp,
        "└─────────────────────────────────────────┘\n\n");
}

/* ═══════════════════════════════════════════════════════════════
 * SECTION E – SEARCH & QUERY
 * ═══════════════════════════════════════════════════════════════ */

/* Internal: pre-order search until predicate returns non-zero */
static ParseTreeNode *_findFirst(ParseTreeNode *node,
                                  TreePredicate pred, void *ud) {
    if (!node) return NULL;
    if (pred(node, ud)) return node;
    for (int i = 0; i < node->numChildren; i++) {
        ParseTreeNode *r = _findFirst(node->children[i], pred, ud);
        if (r) return r;
    }
    return NULL;
}

ParseTreeNode *treeFind(ParseTreeNode *root,
                         TreePredicate pred, void *ud) {
    return _findFirst(root, pred, ud);
}

/* Collect context for findAll */
typedef struct { ParseTreeNode **arr; int count; int cap;
                 TreePredicate pred; void *ud; } FindAllCtx;

static int _findAllVisitor(ParseTreeNode *node, int depth, void *ud) {
    (void)depth;
    FindAllCtx *ctx = (FindAllCtx *)ud;
    if (ctx->pred(node, ctx->ud)) {
        if (ctx->count >= ctx->cap) {
            ctx->cap *= 2;
            ctx->arr  = (ParseTreeNode **)realloc(
                            ctx->arr, ctx->cap * sizeof(ParseTreeNode *));
            if (!ctx->arr) oom();
        }
        ctx->arr[ctx->count++] = node;
    }
    return 0;
}

ParseTreeNode **treeFindAll(ParseTreeNode *root, TreePredicate pred,
                             void *ud, int *outCount) {
    if (!root || !pred) { if (outCount) *outCount = 0; return NULL; }
    FindAllCtx ctx;
    ctx.cap  = 32;
    ctx.count = 0;
    ctx.pred  = pred;
    ctx.ud    = ud;
    ctx.arr   = (ParseTreeNode **)malloc(ctx.cap * sizeof(ParseTreeNode *));
    if (!ctx.arr) oom();
    treePreorder(root, _findAllVisitor, &ctx);
    if (ctx.count == 0) { free(ctx.arr); if (outCount) *outCount = 0; return NULL; }
    /* NULL-terminate */
    ctx.arr = (ParseTreeNode **)realloc(
                ctx.arr, (ctx.count + 1) * sizeof(ParseTreeNode *));
    ctx.arr[ctx.count] = NULL;
    if (outCount) *outCount = ctx.count;
    return ctx.arr;
}

/* ─── Predicate implementations ─── */
static int _predTerminal(const ParseTreeNode *n, void *ud) {
    return n->symbol.type == SYM_TERMINAL &&
           n->symbol.value == *(int *)ud;
}
static int _predNT(const ParseTreeNode *n, void *ud) {
    return n->symbol.type == SYM_NON_TERMINAL &&
           n->symbol.value == *(int *)ud;
}

ParseTreeNode *treeFindByTerminal(ParseTreeNode *root, Terminal t) {
    int v = (int)t;
    return treeFind(root, _predTerminal, &v);
}
ParseTreeNode *treeFindByNT(ParseTreeNode *root, NonTerminal nt) {
    int v = (int)nt;
    return treeFind(root, _predNT, &v);
}
ParseTreeNode **treeFindAllByTerminal(ParseTreeNode *root, Terminal t,
                                       int *outCount) {
    int v = (int)t;
    return treeFindAll(root, _predTerminal, &v, outCount);
}
ParseTreeNode **treeFindAllByNT(ParseTreeNode *root, NonTerminal nt,
                                 int *outCount) {
    int v = (int)nt;
    return treeFindAll(root, _predNT, &v, outCount);
}

int treeIsLeaf(const ParseTreeNode *node) {
    if (!node) return 0;
    if (node->numChildren == 0) return 1;
    /* Treat a node with a single ε child as a leaf conceptually */
    if (node->numChildren == 1 &&
        node->children[0]->symbol.type == SYM_EPSILON) return 1;
    return 0;
}
int treeIsTokenLeaf(const ParseTreeNode *node) {
    return node && node->symbol.type == SYM_TERMINAL;
}
int treeIsEpsilon(const ParseTreeNode *node) {
    return node && node->symbol.type == SYM_EPSILON;
}

/* ═══════════════════════════════════════════════════════════════
 * SECTION F – PRINTING
 * ═══════════════════════════════════════════════════════════════ */

/* ─── ANSI colours (disabled automatically if output is not a tty,
       but we keep them unconditional here for simplicity) ─── */
#define COL_NT      "\033[36m"   /* cyan   – non-terminal       */
#define COL_TK      "\033[32m"   /* green  – terminal           */
#define COL_EPS     "\033[33m"   /* yellow – epsilon            */
#define COL_RULE    "\033[90m"   /* dark   – rule annotation    */
#define COL_LINE    "\033[35m"   /* purple – line numbers       */
#define COL_LEXEME  "\033[97m"   /* bright – lexeme value       */
#define COL_RESET   "\033[0m"

/* ─── Build the node label into a static buffer ─── */
const char *treeNodeLabel(const ParseTreeNode *node) {
    static char buf[256];
    if (!node) { snprintf(buf, sizeof buf, "(null)"); return buf; }
    switch (node->symbol.type) {
        case SYM_NON_TERMINAL:
            snprintf(buf, sizeof buf, "<%s>",
                     nonTerminalNames[node->symbol.value]);
            break;
        case SYM_TERMINAL:
            snprintf(buf, sizeof buf, "%s",
                     terminalNames[node->symbol.value]);
            break;
        case SYM_EPSILON:
            snprintf(buf, sizeof buf, "ε");
            break;
        default:
            snprintf(buf, sizeof buf, "???");
    }
    return buf;
}

/* ─────────────────────────────────────────────────────────────
 * PRINT_SIMPLE  – indented with | and -- connectors
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    FILE  *fp;
    int    maxDepth;
} SimplePrintCtx;

static int _simplePrintVisitor(ParseTreeNode *node, int depth, void *ud) {
    SimplePrintCtx *ctx = (SimplePrintCtx *)ud;
    if (ctx->maxDepth >= 0 && depth > ctx->maxDepth) return 0;
    FILE *fp = ctx->fp;

    /* Indentation */
    for (int i = 0; i < depth * 3; i++) fputc(' ', fp);
    if (depth > 0) fprintf(fp, "|--");

    switch (node->symbol.type) {
        case SYM_NON_TERMINAL:
            fprintf(fp, COL_NT "<%s>" COL_RESET,
                    nonTerminalNames[node->symbol.value]);
            if (node->ruleApplied > 0)
                fprintf(fp, COL_RULE "  [G%d]" COL_RESET,
                        node->ruleApplied);
            fprintf(fp, "\n");
            break;
        case SYM_TERMINAL:
            fprintf(fp, COL_TK "%s" COL_RESET,
                    terminalNames[node->symbol.value]);
            if (node->lexeme[0])
                fprintf(fp, "  " COL_LEXEME "\"%s\"" COL_RESET,
                        node->lexeme);
            if (node->lineNumber > 0)
                fprintf(fp, COL_LINE "  (ln %d)" COL_RESET,
                        node->lineNumber);
            fprintf(fp, "\n");
            break;
        case SYM_EPSILON:
            fprintf(fp, COL_EPS "ε" COL_RESET "\n");
            break;
    }
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * PRINT_BOXED  – Unicode box-drawing, header bar per NT
 * ───────────────────────────────────────────────────────────── */

/* Box-drawing characters */
#define BOX_VER   "│"
#define BOX_TURN  "└─"
#define BOX_FORK  "├─"
#define BOX_HORZ  "──"

/* We do a recursive print for boxed (pre-order DFS with prefix tracking) */
static void _boxedRecurse(const ParseTreeNode *node,
                           const char *prefix,
                           int         isLast,
                           int         depth,
                           int         maxDepth,
                           FILE       *fp) {
    if (!node) return;
    if (maxDepth >= 0 && depth > maxDepth) return;

    /* Print prefix + connector */
    fprintf(fp, "%s", prefix);
    fprintf(fp, "%s", isLast ? BOX_TURN BOX_HORZ " " : BOX_FORK BOX_HORZ " ");

    /* Print the node itself */
    switch (node->symbol.type) {
        case SYM_NON_TERMINAL:
            fprintf(fp, COL_NT "<%s>" COL_RESET,
                    nonTerminalNames[node->symbol.value]);
            if (node->ruleApplied > 0)
                fprintf(fp, COL_RULE " [G%d]" COL_RESET,
                        node->ruleApplied);
            fprintf(fp, "\n");
            break;
        case SYM_TERMINAL:
            fprintf(fp, COL_TK "%s" COL_RESET,
                    terminalNames[node->symbol.value]);
            if (node->lexeme[0])
                fprintf(fp, " " COL_LEXEME "\"%s\"" COL_RESET,
                        node->lexeme);
            if (node->lineNumber > 0)
                fprintf(fp, COL_LINE " (ln %d)" COL_RESET,
                        node->lineNumber);
            fprintf(fp, "\n");
            break;
        case SYM_EPSILON:
            fprintf(fp, COL_EPS "ε" COL_RESET "\n");
            break;
    }

    /* Build new prefix for children */
    char newPrefix[2048];
    snprintf(newPrefix, sizeof newPrefix, "%s%s",
             prefix, isLast ? "    " : BOX_VER "   ");

    for (int i = 0; i < node->numChildren; i++) {
        int childIsLast = (i == node->numChildren - 1);
        _boxedRecurse(node->children[i], newPrefix,
                      childIsLast, depth + 1, maxDepth, fp);
    }
}

/* ─────────────────────────────────────────────────────────────
 * PRINT_COMPACT  – one line per rule expansion
 * ───────────────────────────────────────────────────────────── */
static int _compactVisitor(ParseTreeNode *node, int depth, void *ud) {
    (void)depth;
    FILE *fp = (FILE *)ud;
    if (node->symbol.type != SYM_NON_TERMINAL) return 0;
    if (node->ruleApplied <= 0) return 0;

    fprintf(fp, COL_RULE "G%-3d  " COL_RESET, node->ruleApplied);
    fprintf(fp, COL_NT "<%s>" COL_RESET " → ",
            nonTerminalNames[node->symbol.value]);

    for (int i = 0; i < node->numChildren; i++) {
        const ParseTreeNode *c = node->children[i];
        switch (c->symbol.type) {
            case SYM_NON_TERMINAL:
                fprintf(fp, COL_NT "<%s>" COL_RESET " ",
                        nonTerminalNames[c->symbol.value]);
                break;
            case SYM_TERMINAL:
                fprintf(fp, COL_TK "%s" COL_RESET " ",
                        terminalNames[c->symbol.value]);
                break;
            case SYM_EPSILON:
                fprintf(fp, COL_EPS "ε" COL_RESET " ");
                break;
        }
    }
    fprintf(fp, "\n");
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * PRINT_LEAVES  – token stream reconstructed from leaves
 * ───────────────────────────────────────────────────────────── */
static int _leavesVisitor(ParseTreeNode *node, int depth, void *ud) {
    (void)depth;
    FILE *fp = (FILE *)ud;
    if (node->symbol.type == SYM_TERMINAL) {
        fprintf(fp, COL_TK "%-16s" COL_RESET,
                terminalNames[node->symbol.value]);
        if (node->lexeme[0])
            fprintf(fp, " " COL_LEXEME "%-12s" COL_RESET, node->lexeme);
        if (node->lineNumber > 0)
            fprintf(fp, COL_LINE " (ln %3d)" COL_RESET, node->lineNumber);
        fprintf(fp, "\n");
    }
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * MASTER PRINTER
 * ───────────────────────────────────────────────────────────── */
void treePrint(const ParseTreeNode *root, PrintStyle style,
               int maxDepth, FILE *fp) {
    if (!root || !fp) return;

    switch (style) {

        case PRINT_SIMPLE: {
            fprintf(fp, "\n── Parse Tree (simple) ──\n\n");
            SimplePrintCtx ctx = { fp, maxDepth };
            /* Print root manually (no connector) */
            fprintf(fp, COL_NT "<%s>" COL_RESET,
                    root->symbol.type == SYM_NON_TERMINAL
                        ? nonTerminalNames[root->symbol.value]
                        : treeNodeLabel(root));
            if (root->ruleApplied)
                fprintf(fp, COL_RULE "  [G%d]" COL_RESET,
                        root->ruleApplied);
            fprintf(fp, "\n");
            /* Print children via visitor */
            for (int i = 0; i < root->numChildren; i++)
                _preorder(root->children[i], 1, _simplePrintVisitor, &ctx);
            break;
        }

        case PRINT_BOXED: {
            fprintf(fp, "\n── Parse Tree ──────────────────────────────\n\n");
            /* Print root label */
            if (root->symbol.type == SYM_NON_TERMINAL)
                fprintf(fp, COL_NT "<%s>" COL_RESET,
                        nonTerminalNames[root->symbol.value]);
            else
                fprintf(fp, COL_TK "%s" COL_RESET,
                        terminalNames[root->symbol.value]);
            if (root->ruleApplied)
                fprintf(fp, COL_RULE " [G%d]" COL_RESET,
                        root->ruleApplied);
            fprintf(fp, "\n");
            for (int i = 0; i < root->numChildren; i++) {
                int isLast = (i == root->numChildren - 1);
                _boxedRecurse(root->children[i], "",
                              isLast, 1, maxDepth, fp);
            }
            fprintf(fp, "\n");
            break;
        }

        case PRINT_COMPACT: {
            fprintf(fp, "\n── Parse Trace (rule expansions) ──\n\n");
            treePreorder((ParseTreeNode *)root, _compactVisitor, fp);
            fprintf(fp, "\n");
            break;
        }

        case PRINT_LEAVES: {
            fprintf(fp, "\n── Token Stream (leaves in order) ──\n\n");
            treePreorder((ParseTreeNode *)root, _leavesVisitor, fp);
            fprintf(fp, "\n");
            break;
        }
    }
}

void treePrintStdout(const ParseTreeNode *root) {
    treePrint(root, PRINT_BOXED, -1, stdout);
}

void treePrintTokenStream(const ParseTreeNode *root, FILE *fp) {
    treePrint(root, PRINT_LEAVES, -1, fp);
}

void treePrintNode(const ParseTreeNode *node, FILE *fp) {
    if (!node) { fprintf(fp, "(null node)\n"); return; }
    fprintf(fp, "Node: %-30s", treeNodeLabel(node));
    if (node->ruleApplied > 0)
        fprintf(fp, " rule=G%d", node->ruleApplied);
    if (node->symbol.type == SYM_TERMINAL && node->lexeme[0])
        fprintf(fp, " lexeme=\"%s\"", node->lexeme);
    if (node->lineNumber > 0)
        fprintf(fp, " line=%d", node->lineNumber);
    fprintf(fp, " children=%d", node->numChildren);
    fprintf(fp, " depth=%d\n", treeDepthOf(node));
}

/* ═══════════════════════════════════════════════════════════════
 * SECTION G – SERIALISATION
 * ═══════════════════════════════════════════════════════════════ */

/* Format per line:
 *   DEPTH TYPE VALUE RULE LINE "LEXEME"
 *   TYPE:  T=terminal  N=non-terminal  E=epsilon
 */
static void _serializeNode(const ParseTreeNode *node,
                            int depth, FILE *fp) {
    if (!node) return;
    char t;
    int  v = node->symbol.value;
    switch (node->symbol.type) {
        case SYM_TERMINAL:     t = 'T'; break;
        case SYM_NON_TERMINAL: t = 'N'; break;
        default:               t = 'E'; v = 0; break;
    }
    /* Escape quotes in lexeme */
    char esc[MAX_LEXEME * 2];
    int  k = 0;
    for (int j = 0; node->lexeme[j] && k < (int)sizeof(esc) - 2; j++) {
        if (node->lexeme[j] == '"') esc[k++] = '\\';
        esc[k++] = node->lexeme[j];
    }
    esc[k] = '\0';

    fprintf(fp, "%d %c %d %d %d \"%s\"\n",
            depth, t, v,
            node->ruleApplied, node->lineNumber, esc);

    for (int i = 0; i < node->numChildren; i++)
        _serializeNode(node->children[i], depth + 1, fp);
}

int treeSerialize(const ParseTreeNode *root, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[TREE] Cannot open '%s' for writing.\n", filename);
        return -1;
    }
    fprintf(fp, "# CS F363 Parse Tree – serialised\n");
    fprintf(fp, "# depth type(T/N/E) value ruleApplied lineNumber \"lexeme\"\n");
    _serializeNode(root, 0, fp);
    fclose(fp);
    return 0;
}

/* Deserialise: rebuild the tree from the depth-annotated file */
ParseTreeNode *treeDeserialize(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[TREE] Cannot open '%s' for reading.\n", filename);
        return NULL;
    }

    /* Stack for reconstruction: stores (depth, node) pairs */
    ParseTreeNode *stack[1024];
    int            sdepth[1024];
    int            top = -1;
    ParseTreeNode *root = NULL;

    char line[512];
    while (fgets(line, sizeof line, fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        int  depth, value, rule, lineNum;
        char type;
        char lexbuf[MAX_LEXEME] = {0};

        /* Parse the line */
        int n = sscanf(line, "%d %c %d %d %d \"%[^\"]\"",
                       &depth, &type, &value, &rule, &lineNum, lexbuf);
        if (n < 5) continue;  /* skip malformed */

        /* Build node */
        Symbol sym;
        switch (type) {
            case 'T': sym.type = SYM_TERMINAL;     sym.value = value; break;
            case 'N': sym.type = SYM_NON_TERMINAL; sym.value = value; break;
            default:  sym.type = SYM_EPSILON;      sym.value = 0;     break;
        }
        ParseTreeNode *node = treeNewNode(sym);
        node->ruleApplied = rule;
        node->lineNumber  = lineNum;
        strncpy(node->lexeme, lexbuf, MAX_LEXEME - 1);

        if (depth == 0) {
            root = node;
        } else {
            /* Pop stack back to the node at depth-1 */
            while (top >= 0 && sdepth[top] >= depth) top--;
            if (top >= 0)
                treeAddChild(stack[top], node);
        }
        top++;
        stack [top] = node;
        sdepth[top] = depth;
    }
    fclose(fp);
    return root;
}

/* ═══════════════════════════════════════════════════════════════
 * SECTION H – UTILITY
 * ═══════════════════════════════════════════════════════════════ */

int treeEqual(const ParseTreeNode *a, const ParseTreeNode *b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;
    if (a->symbol.type  != b->symbol.type)  return 0;
    if (a->symbol.value != b->symbol.value) return 0;
    /* For terminals, compare lexemes */
    if (a->symbol.type == SYM_TERMINAL)
        if (strcmp(a->lexeme, b->lexeme) != 0) return 0;
    if (a->numChildren != b->numChildren) return 0;
    for (int i = 0; i < a->numChildren; i++)
        if (!treeEqual(a->children[i], b->children[i])) return 0;
    return 1;
}
