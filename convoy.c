/* Convoy module - doubly linked list of waypoints */
#include <stdio.h>
#include <stdlib.h>
#include "convoy.h"

static Waypoint *head = NULL, *tail = NULL;
static size_t count = 0;

static Waypoint *make(int id, const char *loc, const char *status)
{
    Waypoint *n = malloc(sizeof *n);
    if (!n) return NULL;
    n->id = id;
    snprintf(n->location, WP_NAME, "%s", loc);
    snprintf(n->roadStatus, WP_STATUS, "%s", status);
    n->prev = n->next = NULL;
    return n;
}

Waypoint *convoy_find(int id)
{
    for (Waypoint *c = head; c; c = c->next)
        if (c->id == id) return c;
    return NULL;
}

/* O(1) append using tail pointer */
int convoy_add_end(int id, const char *loc, const char *status)
{
    if (convoy_find(id)) return 0;                  /* duplicate */
    Waypoint *n = make(id, loc, status);
    if (!n) return 0;
    n->prev = tail;
    if (!head) head = n; else tail->next = n;
    tail = n;
    count++;
    return 1;
}

/* real-time insertion: detour point after an existing waypoint */
int convoy_insert_after(int afterId, int id, const char *loc, const char *status)
{
    Waypoint *a = convoy_find(afterId);
    if (!a || convoy_find(id)) return 0;
    Waypoint *n = make(id, loc, status);
    if (!n) return 0;
    n->prev = a;
    n->next = a->next;
    if (a->next) a->next->prev = n; else tail = n;
    a->next = n;
    count++;
    return 1;
}

int convoy_delete(int id)
{
    Waypoint *n = convoy_find(id);
    if (!n) return 0;
    if (n->prev) n->prev->next = n->next; else head = n->next;
    if (n->next) n->next->prev = n->prev; else tail = n->prev;
    free(n);
    count--;
    return 1;
}

int convoy_update_status(int id, const char *status)
{
    Waypoint *n = convoy_find(id);
    if (!n) return 0;
    snprintf(n->roadStatus, WP_STATUS, "%s", status);
    return 1;
}

static void print_wp(const Waypoint *n)
{
    printf("  [node %d] %-16s road: %s\n", n->id, n->location, n->roadStatus);
}

void convoy_display_forward(void)
{
    if (!head) { puts("Convoy route is empty."); return; }
    puts("\n--- Convoy route (outbound) ---");
    for (Waypoint *c = head; c; c = c->next) print_wp(c);
}

void convoy_display_backward(void)
{
    if (!tail) { puts("Convoy route is empty."); return; }
    puts("\n--- Convoy route (return trip) ---");
    for (Waypoint *c = tail; c; c = c->prev) print_wp(c);
}

size_t convoy_count(void) { return count; }

void convoy_clear(void)
{
    Waypoint *c = head;
    while (c) { Waypoint *n = c->next; free(c); c = n; }
    head = tail = NULL;
    count = 0;
}
