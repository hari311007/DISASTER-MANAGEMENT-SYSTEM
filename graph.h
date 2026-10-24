#ifndef GRAPH_H
#define GRAPH_H

/* ROAD NETWORK: Weighted Directed Graph + Dijkstra
 * Nodes = relief hubs / checkpoints / disaster zones
 * Edges = roads, weight = travel time (minutes)
 * A road can be BLOCKED (e.g. by flood grid) -> Dijkstra skips it.
 */

#define MAX_NODES 50
#define NODE_NAME 24
#define INF 1000000000

typedef enum { HUB, CHECKPOINT, ZONE } NodeType;

typedef struct Edge {
    int dest;
    int weight;                /* travel time in minutes */
    int blocked;               /* 1 = flooded / blocked   */
    struct Edge *next;
} Edge;

typedef struct {
    char     name[NODE_NAME];
    NodeType type;
    Edge    *head;             /* adjacency list */
} Node;

typedef struct {
    Node nodes[MAX_NODES];
    int  n;
} Graph;

void graph_init(Graph *g);
int  graph_add_node(Graph *g, const char *name, NodeType type);   /* returns id */
int  graph_add_road(Graph *g, int src, int dest, int weight);     /* one-way    */
void graph_add_two_way(Graph *g, int a, int b, int weight);
int  graph_set_blocked(Graph *g, int src, int dest, int blocked); /* used by flood grid / undo */
void graph_display(Graph *g);

/* Dijkstra: fills dist[] and parent[], skips blocked roads */
void dijkstra(Graph *g, int src, int dist[], int parent[]);

/* Prints the fastest safe route; returns total time or -1 if unreachable.
 * path[] / path_len give the node sequence (for convoy waypoints). */
int  graph_shortest_route(Graph *g, int src, int dest, int path[], int *path_len);

/* Finds the nearest hub to a zone (for emergency dispatch) */
int  graph_nearest_hub(Graph *g, int zone, int *time_out);

void graph_free(Graph *g);

#endif
