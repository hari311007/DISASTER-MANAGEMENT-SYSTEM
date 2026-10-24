/* Circular Queue module - medical team round-robin */
#include <stdio.h>
#include <string.h>
#include "circular_queue.h"

static MedTeam queue[CQ_MAX];
static int front = -1, rear = -1;
static int nextTeamId = 101;

int cq_is_empty(void) { return front == -1; }
int cq_is_full(void)  { return (rear + 1) % CQ_MAX == front; }

int cq_count(void)
{
    if (cq_is_empty()) return 0;
    return (rear - front + CQ_MAX) % CQ_MAX + 1;
}

static int put(MedTeam t)
{
    if (cq_is_full()) return 0;
    if (cq_is_empty()) front = 0;
    rear = (rear + 1) % CQ_MAX;
    queue[rear] = t;
    return 1;
}

int cq_enqueue(const char *name, const char *specialty)
{
    MedTeam t;
    memset(&t, 0, sizeof t);
    t.teamId = nextTeamId;
    snprintf(t.teamName, sizeof t.teamName, "%s", name);
    snprintf(t.specialty, sizeof t.specialty, "%s", specialty);
    if (!put(t)) return 0;
    nextTeamId++;
    return t.teamId;
}

int cq_dequeue(MedTeam *out)
{
    if (cq_is_empty()) return 0;
    *out = queue[front];
    if (front == rear) front = rear = -1;          /* last element removed */
    else front = (front + 1) % CQ_MAX;
    return 1;
}

int cq_peek(MedTeam *out)
{
    if (cq_is_empty()) return 0;
    *out = queue[front];
    return 1;
}

/* Round robin: take front team, give it a mission, send it to the rear */
int cq_assign_next(MedTeam *out)
{
    MedTeam t;
    if (!cq_dequeue(&t)) return 0;
    t.missions++;
    put(t);                                        /* slot just freed, never fails */
    *out = t;
    return 1;
}

void cq_display(void)
{
    if (cq_is_empty()) { printf("No medical teams on duty.\n"); return; }
    printf("\n%-6s %-6s %-20s %-12s %s\n", "Slot", "ID", "Team", "Specialty", "Missions");
    printf("-------------------------------------------------------\n");
    for (int i = front;; i = (i + 1) % CQ_MAX) {
        printf("%-6d %-6d %-20s %-12s %d\n", i, queue[i].teamId,
               queue[i].teamName, queue[i].specialty, queue[i].missions);
        if (i == rear) break;
    }
}

void cq_status(void)
{
    printf("Teams: %d / %d | front = %d | rear = %d | %s\n",
           cq_count(), CQ_MAX, front, rear,
           cq_is_empty() ? "EMPTY" : cq_is_full() ? "FULL" : "AVAILABLE");
}
