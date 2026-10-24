#ifndef STACK_H
#define STACK_H

/* STACK: Undo / Redo for relief operations.
 * Every allocation or road block is pushed on the UNDO stack.
 * Undo pops it, reverses it, and pushes it on the REDO stack.
 * Any new action clears the REDO stack.
 */

#define OP_TEXT 50

typedef enum { OP_ALLOCATE, OP_BLOCK_ROAD, OP_UNBLOCK_ROAD } OpType;

typedef struct Operation {
    unsigned long id;
    OpType type;
    char   sku[OP_TEXT];        /* OP_ALLOCATE: item dispatched      */
    int    quantity;
    char   zone[OP_TEXT];       /* destination zone name             */
    int    src, dest;           /* OP_*_ROAD: road endpoints         */
    struct Operation *next;
} Operation;

void       stack_push_undo(Operation op);   /* new action: also clears redo */
void       stack_restore_undo(Operation op);   /* used by redo */
int        stack_pop_undo(Operation *out);
void       stack_push_redo(Operation op);
int        stack_pop_redo(Operation *out);
int        stack_peek_undo(Operation *out);
void       stack_display(void);
size_t     stack_undo_count(void);
size_t     stack_redo_count(void);
const char *stack_op_text(OpType t);
void       stack_free(void);

#endif
