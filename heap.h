#ifndef HEAP_H
#define HEAP_H

#define MAX_EMERGENCIES 100

typedef struct {
    int  id;
    int  severity;          /* 1 (low) to 10 (critical) - the heap key */
    char location[30];
    char type[20];          /* Medical, Rescue, Food, Shelter */
    int  peopleAffected;
} Emergency;

int  heap_insert(Emergency e);              /* 1 = ok, 0 = full/invalid */
int  heap_extract_max(Emergency *out);      /* 1 = ok, 0 = empty */
int  heap_peek(Emergency *out);             /* look at top without removing */
int  heap_update_severity(int id, int newSeverity);
int  heap_delete(int id);
int  heap_is_empty(void);
int  heap_is_full(void);
int  heap_size(void);
void heap_display(void);                    /* prints in priority order */

#endif
