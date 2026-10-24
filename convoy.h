#ifndef CONVOY_H
#define CONVOY_H

/* DOUBLY LINKED LIST: Convoy transit log.
 * Waypoints are loaded from the Dijkstra route; they can be
 * inserted/removed live and walked forward or backward (return trip).
 */

#define WP_NAME   50
#define WP_STATUS 20

typedef struct Waypoint {
    int  id;                     /* graph node id */
    char location[WP_NAME];
    char roadStatus[WP_STATUS];  /* OPEN / BLOCKED */
    struct Waypoint *prev, *next;
} Waypoint;

int       convoy_add_end(int id, const char *loc, const char *status);
int       convoy_insert_after(int afterId, int id, const char *loc, const char *status);
int       convoy_delete(int id);
Waypoint *convoy_find(int id);
int       convoy_update_status(int id, const char *status);
void      convoy_display_forward(void);
void      convoy_display_backward(void);
size_t    convoy_count(void);
void      convoy_clear(void);

#endif
