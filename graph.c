#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"

static const char *type_str(NodeType t) {
    return t == HUB ? "HUB" : t == CHECKPOINT ? "CHECKPOINT" : "ZONE";
}

void graph_init(Graph *g) {
    g->n = 0;
    for (int i = 0; i < MAX_NODES; i++) g->nodes[i].head = NULL;
}

int graph_add_node(Graph *g, const char *name, NodeType type) {
    if (g->n >= MAX_NODES) return -1;
    int id = g->n++;
    strncpy(g->nodes[id].name, name, NODE_NAME - 1);
    g->nodes[id].name[NODE_NAME - 1] = '\0';
    g->nodes[id].type = type;
    g->nodes[id].head = NULL;
    return id;
}

int graph_add_road(Graph *g, int src, int dest, int weight) {
    if (src < 0 || dest < 0 || src >= g->n || dest >= g->n || weight < 0) return 0;
    Edge *e = (Edge *)malloc(sizeof(Edge));
    if (!e) return 0;
    e->dest = dest;
    e->weight = weight;
    e->blocked = 0;
    e->next = g->nodes[src].head;
    g->nodes[src].head = e;
    return 1;
}

void graph_add_two_way(Graph *g, int a, int b, int weight) {
    graph_add_road(g, a, b, weight);
    graph_add_road(g, b, a, weight);
}

int graph_set_blocked(Graph *g, int src, int dest, int blocked) {
    for (Edge *e = g->nodes[src].head; e; e = e->next)
        if (e->dest == dest) { e->blocked = blocked; return 1; }
    return 0;
}

void graph_display(Graph *g) {
    printf("\n=== Road Network ===\n");
    for (int i = 0; i < g->n; i++) {
        printf("[%d] %-14s (%s) ->", i, g->nodes[i].name, type_str(g->nodes[i].type));
        for (Edge *e = g->nodes[i].head; e; e = e->next)
            printf(" %d(%dm%s)", e->dest, e->weight, e->blocked ? ",BLOCKED" : "");
        printf("\n");
    }
}

//Dijkstra
void dijkstra(Graph *g, int src, int dist[], int parent[]) {
    int visited[MAX_NODES] = {0};
    for (int i = 0; i < g->n; i++) { dist[i] = INF; parent[i] = -1; }
    dist[src] = 0;

    for (int count = 0; count < g->n; count++) {
        int u = -1;
        for (int i = 0; i < g->n; i++)                 /* pick closest unvisited */
            if (!visited[i] && (u == -1 || dist[i] < dist[u])) u = i;
        if (u == -1 || dist[u] == INF) break;          /* rest unreachable */
        visited[u] = 1;

        for (Edge *e = g->nodes[u].head; e; e = e->next) {
            if (e->blocked) continue;                  /* avoid flooded road */
            int v = e->dest;
            if (!visited[v] && dist[u] + e->weight < dist[v]) {
                dist[v] = dist[u] + e->weight;         /* relaxation */
                parent[v] = u;
            }
        }
    }
}

int graph_shortest_route(Graph *g, int src, int dest, int path[], int *path_len) {
    int dist[MAX_NODES], parent[MAX_NODES], tmp[MAX_NODES], k = 0;
    dijkstra(g, src, dist, parent);

    *path_len = 0;
    if (dist[dest] == INF) {
        printf("No safe route from %s to %s!\n", g->nodes[src].name, g->nodes[dest].name);
        return -1;
    }
    for (int v = dest; v != -1; v = parent[v]) tmp[k++] = v;   /* backtrack */
    for (int i = 0; i < k; i++) path[i] = tmp[k - 1 - i];        /* reverse   */
    *path_len = k;

    printf("Route: ");
    for (int i = 0; i < k; i++)
        printf("%s%s", g->nodes[path[i]].name, i < k - 1 ? " -> " : "");
    printf("  | Time: %d min\n", dist[dest]);
    return dist[dest];
}

int graph_nearest_hub(Graph *g, int zone, int *time_out) {
    int best = -1, best_t = INF, dist[MAX_NODES], parent[MAX_NODES];
    for (int h = 0; h < g->n; h++) {
        if (g->nodes[h].type != HUB) continue;
        dijkstra(g, h, dist, parent);
        if (dist[zone] < best_t) { best_t = dist[zone]; best = h; }
    }
    *time_out = (best == -1) ? -1 : best_t;
    return best;
}

void graph_free(Graph *g) {
    for (int i = 0; i < g->n; i++) {
        Edge *e = g->nodes[i].head;
        while (e) { Edge *n = e->next; free(e); e = n; }
        g->nodes[i].head = NULL;
    }
    g->n = 0;
}
