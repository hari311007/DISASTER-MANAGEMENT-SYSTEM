/* Stack module - linked-list undo/redo stacks */
#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

static Operation *undoTop = NULL, *redoTop = NULL;
static size_t undoCount = 0, redoCount = 0;
static unsigned long nextId = 1;

static int push(Operation **top, size_t *count, Operation op)
{
    Operation *n = malloc(sizeof *n);
    if (!n) { puts("Memory allocation failed."); return 0; }
    *n = op;
    n->next = *top;
    *top = n;
    (*count)++;
    return 1;
}

static int pop(Operation **top, size_t *count, Operation *out)
{
    if (!*top) return 0;
    Operation *t = *top;
    *out = *t;
    out->next = NULL;
    *top = t->next;
    free(t);
    (*count)--;
    return 1;
}

static void clear(Operation **top, size_t *count)
{
    Operation tmp;
    while (pop(top, count, &tmp)) ;
}

const char *stack_op_text(OpType t)
{
    return t == OP_ALLOCATE ? "ALLOCATE" : t == OP_BLOCK_ROAD ? "BLOCK ROAD" : "UNBLOCK ROAD";
}

void stack_push_undo(Operation op)
{
    op.id = nextId++;
    if (push(&undoTop, &undoCount, op))
        clear(&redoTop, &redoCount);        /* new action invalidates redo */
}

/* redo: put op back on undo, keep its id, do NOT clear redo */
void stack_restore_undo(Operation op) { push(&undoTop, &undoCount, op); }

int  stack_pop_undo(Operation *out)  { return pop(&undoTop, &undoCount, out); }
void stack_push_redo(Operation op)   { push(&redoTop, &redoCount, op); }
int  stack_pop_redo(Operation *out)  { return pop(&redoTop, &redoCount, out); }

int stack_peek_undo(Operation *out)
{
    if (!undoTop) return 0;
    *out = *undoTop;
    return 1;
}

size_t stack_undo_count(void) { return undoCount; }
size_t stack_redo_count(void) { return redoCount; }

static void print_op(const Operation *o)
{
    if (o->type == OP_ALLOCATE)
        printf("  #%-3lu %-12s %d x %s -> %s\n", o->id, stack_op_text(o->type),
               o->quantity, o->sku, o->zone);
    else
        printf("  #%-3lu %-12s road %d -> %d\n", o->id, stack_op_text(o->type),
               o->src, o->dest);
}

void stack_display(void)
{
    printf("\n--- UNDO stack (top first): %zu ---\n", undoCount);
    if (!undoTop) puts("  (empty)");
    for (Operation *o = undoTop; o; o = o->next) print_op(o);
    printf("--- REDO stack (top first): %zu ---\n", redoCount);
    if (!redoTop) puts("  (empty)");
    for (Operation *o = redoTop; o; o = o->next) print_op(o);
}

void stack_free(void)
{
    clear(&undoTop, &undoCount);
    clear(&redoTop, &redoCount);
}
