#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

/* CIRCULAR QUEUE: Round-robin scheduling of field medical teams.
 * The team at the front takes the next medical emergency, then
 * rotates to the rear, so every team gets work in turn.
 */

#define CQ_MAX 5

typedef struct {
    int  teamId;
    char teamName[30];
    char specialty[20];      /* Trauma, General, Pediatric ... */
    int  missions;           /* missions completed */
} MedTeam;

int  cq_is_empty(void);
int  cq_is_full(void);
int  cq_count(void);
int  cq_enqueue(const char *name, const char *specialty);   /* returns team id, 0 if full */
int  cq_dequeue(MedTeam *out);                              /* remove team (off duty) */
int  cq_peek(MedTeam *out);
int  cq_assign_next(MedTeam *out);                          /* front -> mission -> rear */
void cq_display(void);
void cq_status(void);

#endif
