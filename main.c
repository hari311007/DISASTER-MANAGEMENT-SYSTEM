/*
 * DISASTER RELIEF SUPPLY CHAIN & EMERGENCY RESPONSE LOGISTICS
 * Integrated driver: connects all 8 data-structure modules.
 *
 * Compile: gcc main.c -o relief
 * main.c includes every module's .c file (each .c includes its own .h),
 * so only main.c needs to be passed to gcc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

#include "flood_grid.c"
#include "graph.c"
#include "heap.c"
#include "hash_table.c"
#include "trie.c"
#include "stack.c"
#include "circular_queue.c"
#include "convoy.c"

/* ================= SHARED STATE ================= */
static Graph g;                       /* road network                */
static int   nodeRow[MAX_NODES];      /* grid cell of each graph node */
static int   nodeCol[MAX_NODES];

#define NUM_DEPOTS 2
static HashTable depots[NUM_DEPOTS];  /* one inventory per hub       */

static int nextEmergencyId = 1;
static int nextSurvivorId  = 1;

/* ================= INPUT HELPERS ================= */
static int read_line(const char *prompt, char *buf, size_t size)
{
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, (int)size, stdin)) return 0;
    size_t len = strlen(buf);
    if (len && buf[len - 1] == '\n') buf[--len] = '\0';
    else { int ch; while ((ch = getchar()) != '\n' && ch != EOF) ; }
    if (len && buf[len - 1] == '\r') buf[--len] = '\0';
    return 1;
}

static int read_int(const char *prompt, int *out)
{
    char buf[64], *end;
    while (read_line(prompt, buf, sizeof buf)) {
        long v = strtol(buf, &end, 10);
        while (isspace((unsigned char)*end)) end++;
        if (end != buf && *end == '\0') { *out = (int)v; return 1; }
        puts("Enter a valid number.");
    }
    return 0;
}

static int read_text(const char *prompt, char *buf, size_t size)
{
    while (read_line(prompt, buf, size)) {
        if (buf[0]) return 1;
        puts("Input cannot be empty.");
    }
    return 0;
}

/* ================= LINK HELPERS ================= */
static HashTable *depot_at(int node)
{
    for (int i = 0; i < NUM_DEPOTS; i++)
        if (depots[i].depot_node_id == node) return &depots[i];
    return NULL;
}

static int node_by_name(const char *name)
{
    for (int i = 0; i < g.n; i++)
        if (strcasecmp(g.nodes[i].name, name) == 0) return i;
    return -1;
}

/* Emergency type -> which SKU the convoy carries */
static const char *sku_for_type(const char *type)
{
    if (strcasecmp(type, "Medical") == 0) return "MED-001";
    if (strcasecmp(type, "Food")    == 0) return "FOOD-010";
    if (strcasecmp(type, "Shelter") == 0) return "TENT-005";
    return "WAT-002";                             /* Rescue -> water */
}

static void list_nodes(void)
{
    for (int i = 0; i < g.n; i++)
        printf("  %d = %s\n", i, g.nodes[i].name);
}

/* ================= SAMPLE DATA ================= */
static void seed_data(void)
{
    graph_init(&g);
    struct { const char *n; NodeType t; int r, c; } nodes[] = {
        {"Chennai Hub",  HUB,        0, 0},
        {"Vellore Hub",  HUB,        7, 9},
        {"Checkpoint-1", CHECKPOINT, 2, 4},   /* low ground (2 m) */
        {"Checkpoint-2", CHECKPOINT, 5, 7},
        {"Tambaram",     ZONE,       1, 2},
        {"Velachery",    ZONE,       4, 7},
        {"Adyar",        ZONE,       6, 3},
    };
    for (int i = 0; i < 7; i++) {
        int id = graph_add_node(&g, nodes[i].n, nodes[i].t);
        nodeRow[id] = nodes[i].r;
        nodeCol[id] = nodes[i].c;
    }
    graph_add_two_way(&g, 0, 2, 20);
    graph_add_two_way(&g, 0, 4, 15);
    graph_add_two_way(&g, 4, 2, 10);
    graph_add_two_way(&g, 2, 5, 15);
    graph_add_two_way(&g, 2, 6, 25);
    graph_add_two_way(&g, 0, 3, 45);
    graph_add_two_way(&g, 3, 5, 10);
    graph_add_two_way(&g, 1, 3, 30);
    graph_add_two_way(&g, 3, 6, 35);

    ht_init(&depots[0], 0);
    ht_insert(&depots[0], "MED-001",  "Paracetamol", 500, "2027-03-15");
    ht_insert(&depots[0], "FOOD-010", "Rice 5kg",    120, "2027-01-01");
    ht_insert(&depots[0], "WAT-002",  "Water 1L",    400, "2026-12-31");
    ht_insert(&depots[0], "TENT-005", "Tent",         30, "2030-01-01");
    ht_init(&depots[1], 1);
    ht_insert(&depots[1], "MED-001",  "Paracetamol", 200, "2027-05-10");
    ht_insert(&depots[1], "FOOD-010", "Rice 5kg",    300, "2027-02-01");
    ht_insert(&depots[1], "WAT-002",  "Water 1L",    150, "2026-11-30");
    ht_insert(&depots[1], "TENT-005", "Tent",         60, "2030-01-01");

    struct { int sev; const char *loc, *type; int people; } em[] = {
        {9, "Velachery", "Medical", 30},
        {7, "Adyar",     "Rescue",  12},
        {5, "Tambaram",  "Food",    40},
        {3, "Adyar",     "Shelter", 20},
    };
    for (int i = 0; i < 4; i++) {
        Emergency e; memset(&e, 0, sizeof e);
        e.id = nextEmergencyId++; e.severity = em[i].sev; e.peopleAffected = em[i].people;
        snprintf(e.location, sizeof e.location, "%s", em[i].loc);
        snprintf(e.type, sizeof e.type, "%s", em[i].type);
        heap_insert(e);
    }

    struct { const char *n; int age; const char *loc; int st; } sv[] = {
        {"Ravi", 25, "Tambaram", STATUS_SAFE},   {"Ram", 30, "Adyar", STATUS_INJURED},
        {"Rajesh", 41, "Adyar", STATUS_MISSING}, {"Ramya", 19, "Velachery", STATUS_INJURED},
        {"Sita", 35, "Velachery", STATUS_MISSING},
    };
    for (int i = 0; i < 5; i++) {
        Survivor s; memset(&s, 0, sizeof s);
        s.id = nextSurvivorId++; s.age = sv[i].age; s.status = sv[i].st;
        snprintf(s.name, SURV_NAME_LEN, "%s", sv[i].n);
        snprintf(s.location, LOC_LEN, "%s", sv[i].loc);
        trie_insert(s);
    }

    cq_enqueue("Alpha", "Trauma");
    cq_enqueue("Bravo", "General");
    cq_enqueue("Charlie", "Pediatric");
}

/* ================= INTEGRATION: FLOOD -> GRAPH ================= */
/* Block every road leading INTO a node whose grid cell is flooded */
static void apply_flood_to_roads(void)
{
    int blocked = 0;
    for (int v = 0; v < g.n; v++) {
        if (!flood_is_flooded(nodeRow[v], nodeCol[v])) continue;
        printf("  Node %s at (%d,%d) is UNDER WATER\n", g.nodes[v].name, nodeRow[v], nodeCol[v]);
        for (int u = 0; u < g.n; u++)
            for (Edge *e = g.nodes[u].head; e; e = e->next)
                if (e->dest == v && !e->blocked) {
                    e->blocked = 1;
                    Operation op = {0}; op.type = OP_BLOCK_ROAD; op.src = u; op.dest = v;
                    stack_push_undo(op);
                    blocked++;
                }
    }
    printf("  %d road(s) blocked in the graph (undoable).\n", blocked);
}

static void unblock_all_roads(void)
{
    for (int u = 0; u < g.n; u++)
        for (Edge *e = g.nodes[u].head; e; e = e->next) e->blocked = 0;
}

/* ================= INTEGRATION: FULL DISPATCH ================= */
/* HEAP -> GRAPH(nearest hub + Dijkstra) -> HASH(stock) -> CONVOY(DLL)
 *      -> QUEUE(medical team) -> TRIE(missing list) -> STACK(undo) */
static void dispatch_top_emergency(void)
{
    Emergency e;
    if (!heap_extract_max(&e)) { puts("No pending emergencies."); return; }
    printf("\n[1] HEAP     : Top emergency #%d - %s at %s (severity %d, %d people)\n",
           e.id, e.type, e.location, e.severity, e.peopleAffected);

    int zone = node_by_name(e.location);
    if (zone < 0) { printf("    Unknown location in road network. Re-queued.\n"); heap_insert(e); return; }

    int t, hub = graph_nearest_hub(&g, zone, &t);
    if (hub < 0) { puts("[2] GRAPH    : No hub can reach this zone (roads flooded). Re-queued."); heap_insert(e); return; }
    printf("[2] GRAPH    : Nearest hub = %s (%d min)\n", g.nodes[hub].name, t);

    int path[MAX_NODES], len;
    printf("    Dijkstra ");
    graph_shortest_route(&g, hub, zone, path, &len);

    HashTable *dep = depot_at(hub);
    const char *sku = sku_for_type(e.type);
    int r = dep ? ht_update_qty(dep, sku, -e.peopleAffected) : 0;
    if (r != 1) {
        printf("[3] HASH     : %s at %s - %s. Re-queued.\n", sku, g.nodes[hub].name,
               r == -1 ? "insufficient stock" : "item not found");
        heap_insert(e);
        return;
    }
    Item *it = ht_search(dep, sku);
    printf("[3] HASH     : %d x %s (%s) loaded. Left at depot: %d\n",
           e.peopleAffected, sku, it->name, it->quantity);

    convoy_clear();
    for (int i = 0; i < len; i++)
        convoy_add_end(path[i], g.nodes[path[i]].name, "OPEN");
    printf("[4] CONVOY   : %zu waypoints loaded into transit log\n", convoy_count());
    convoy_display_forward();

    if (strcasecmp(e.type, "Medical") == 0) {
        MedTeam m;
        if (cq_assign_next(&m))
            printf("[5] QUEUE    : Medical team %s (%s) assigned, mission #%d\n",
                   m.teamName, m.specialty, m.missions);
        else
            puts("[5] QUEUE    : No medical teams on duty!");
    }
    if (strcasecmp(e.type, "Rescue") == 0) {
        puts("[5] TRIE     : Missing persons to search for:");
        trie_list_by_status(STATUS_MISSING);
    }

    Operation op = {0};
    op.type = OP_ALLOCATE; op.quantity = e.peopleAffected; op.src = hub;
    snprintf(op.sku, OP_TEXT, "%s", sku);
    snprintf(op.zone, OP_TEXT, "%s", e.location);
    stack_push_undo(op);
    puts("[6] STACK    : Allocation recorded (can be undone)");
}

/* ================= UNDO / REDO ================= */
static int apply_op(const Operation *o, int reverse)
{
    if (o->type == OP_ALLOCATE) {
        HashTable *d = depot_at(o->src);
        int change = reverse ? o->quantity : -o->quantity;
        if (!d || ht_update_qty(d, o->sku, change) != 1) return 0;
        printf("%s %d x %s at %s\n", reverse ? "Returned" : "Re-dispatched",
               o->quantity, o->sku, g.nodes[o->src].name);
        return 1;
    }
    int block = (o->type == OP_BLOCK_ROAD);
    if (reverse) block = !block;
    graph_set_blocked(&g, o->src, o->dest, block);
    printf("Road %s -> %s is now %s\n", g.nodes[o->src].name, g.nodes[o->dest].name,
           block ? "BLOCKED" : "OPEN");
    return 1;
}

static void undo(void)
{
    Operation o;
    if (!stack_pop_undo(&o)) { puts("Nothing to undo."); return; }
    printf("UNDO #%lu %s: ", o.id, stack_op_text(o.type));
    if (apply_op(&o, 1)) stack_push_redo(o);
    else { puts("failed."); stack_restore_undo(o); }
}

static void redo(void)
{
    Operation o;
    if (!stack_pop_redo(&o)) { puts("Nothing to redo."); return; }
    printf("REDO #%lu %s: ", o.id, stack_op_text(o.type));
    if (apply_op(&o, 0)) stack_restore_undo(o);
    else { puts("failed (stock changed)."); stack_push_redo(o); }
}

/* ================= SUB-MENUS ================= */
static void menu_flood(void)
{
    int ch, r, c, lvl;
    do {
        puts("\n--- FLOOD GRID (2D Array) ---");
        puts("1. Elevation map  2. Simulate flood  3. Flood report");
        puts("4. Relief camp sites  5. Reset flood (reopen roads)  0. Back");
        if (!read_int("Choice: ", &ch)) return;
        switch (ch) {
        case 1: flood_display_elevation(); break;
        case 2:
            if (read_int("Breach row: ", &r) && read_int("Breach col: ", &c) &&
                read_int("Water level (m): ", &lvl) && flood_simulate(r, c, lvl) > 0)
                apply_flood_to_roads();
            break;
        case 3: flood_report(); break;
        case 4: flood_suggest_camps(); break;
        case 5: flood_reset(); unblock_all_roads(); puts("Flood cleared, all roads open."); break;
        }
    } while (ch != 0);
}

static void menu_inventory(void)
{
    int ch, node, q;
    char sku[SKU_LEN], name[NAME_LEN], exp[DATE_LEN];
    if (!read_int("Depot hub node (0 = Chennai, 1 = Vellore): ", &node)) return;
    HashTable *d = depot_at(node);
    if (!d) { puts("No depot at that node."); return; }
    do {
        printf("\n--- INVENTORY @ %s (Hash Table) ---\n", g.nodes[node].name);
        puts("1. Display  2. Add/restock  3. Search  4. Dispatch qty  5. Delete SKU  6. Low stock  0. Back");
        if (!read_int("Choice: ", &ch)) return;
        switch (ch) {
        case 1: ht_display(d); break;
        case 2:
            if (read_text("SKU: ", sku, sizeof sku) && read_text("Name: ", name, sizeof name) &&
                read_int("Qty: ", &q) && read_text("Expiry (YYYY-MM-DD): ", exp, sizeof exp))
                puts(ht_insert(d, sku, name, q, exp) ? "Saved." : "Failed.");
            break;
        case 3:
            if (read_text("SKU: ", sku, sizeof sku)) {
                Item *it = ht_search(d, sku);
                if (it) printf("%s | %s | qty %d | exp %s | bucket %u\n",
                               it->sku, it->name, it->quantity, it->expiry, ht_hash(sku));
                else puts("Not found.");
            }
            break;
        case 4:
            if (read_text("SKU: ", sku, sizeof sku) && read_int("Qty to dispatch: ", &q) && q > 0) {
                int r = ht_update_qty(d, sku, -q);
                if (r == 1) {
                    Operation op = {0}; op.type = OP_ALLOCATE; op.quantity = q; op.src = node;
                    snprintf(op.sku, OP_TEXT, "%s", sku);
                    snprintf(op.zone, OP_TEXT, "manual");
                    stack_push_undo(op);
                    puts("Dispatched (undoable).");
                } else puts(r == -1 ? "Insufficient stock." : "Not found.");
            }
            break;
        case 5: if (read_text("SKU: ", sku, sizeof sku)) puts(ht_delete(d, sku) ? "Deleted." : "Not found."); break;
        case 6: if (read_int("Threshold: ", &q)) ht_low_stock(d, q); break;
        }
    } while (ch != 0);
}

static void menu_emergency(void)
{
    int ch, id, sev;
    Emergency e;
    do {
        puts("\n--- EMERGENCIES (Max Heap) ---");
        puts("1. Display (priority order)  2. Report new SOS  3. Peek top");
        puts("4. Update severity  5. Cancel emergency  0. Back");
        if (!read_int("Choice: ", &ch)) return;
        switch (ch) {
        case 1: heap_display(); break;
        case 2:
            memset(&e, 0, sizeof e);
            puts("Zones:"); list_nodes();
            if (read_text("Location (zone name): ", e.location, sizeof e.location) &&
                read_text("Type (Medical/Rescue/Food/Shelter): ", e.type, sizeof e.type) &&
                read_int("Severity 1-10: ", &e.severity) &&
                read_int("People affected: ", &e.peopleAffected)) {
                if (node_by_name(e.location) < 0) { puts("Location not in road network."); break; }
                e.id = nextEmergencyId;
                if (heap_insert(e)) { nextEmergencyId++; printf("SOS #%d queued.\n", e.id); }
                else puts("Rejected (full or bad severity).");
            }
            break;
        case 3:
            if (heap_peek(&e)) printf("Top: #%d %s at %s, severity %d\n", e.id, e.type, e.location, e.severity);
            else puts("Empty.");
            break;
        case 4:
            if (read_int("ID: ", &id) && read_int("New severity: ", &sev))
                puts(heap_update_severity(id, sev) ? "Updated." : "Failed.");
            break;
        case 5: if (read_int("ID: ", &id)) puts(heap_delete(id) ? "Cancelled." : "Not found."); break;
        }
    } while (ch != 0);
}

static void menu_roads(void)
{
    int ch, a, b, w, path[MAX_NODES], len;
    do {
        puts("\n--- ROAD NETWORK (Graph + Dijkstra) ---");
        puts("1. Display  2. Shortest route  3. Nearest hub to zone");
        puts("4. Block road  5. Open road  6. Add road  0. Back");
        if (!read_int("Choice: ", &ch)) return;
        if (ch >= 2 && ch <= 6) list_nodes();
        switch (ch) {
        case 1: graph_display(&g); break;
        case 2:
            if (read_int("From: ", &a) && read_int("To: ", &b) && a >= 0 && b >= 0 && a < g.n && b < g.n)
                graph_shortest_route(&g, a, b, path, &len);
            break;
        case 3:
            if (read_int("Zone: ", &a) && a >= 0 && a < g.n) {
                int h = graph_nearest_hub(&g, a, &w);
                if (h < 0) puts("No hub can reach this zone.");
                else printf("Nearest hub: %s (%d min)\n", g.nodes[h].name, w);
            }
            break;
        case 4: case 5:
            if (read_int("From: ", &a) && read_int("To: ", &b) && a >= 0 && a < g.n) {
                int blk = (ch == 4);
                if (graph_set_blocked(&g, a, b, blk)) {
                    Operation op = {0};
                    op.type = blk ? OP_BLOCK_ROAD : OP_UNBLOCK_ROAD; op.src = a; op.dest = b;
                    stack_push_undo(op);
                    puts(blk ? "Road blocked (undoable)." : "Road opened (undoable).");
                } else puts("No such road.");
            }
            break;
        case 6:
            if (read_int("From: ", &a) && read_int("To: ", &b) && read_int("Minutes: ", &w))
                { graph_add_two_way(&g, a, b, w); puts("Two-way road added."); }
            break;
        }
    } while (ch != 0);
}

static void menu_survivors(void)
{
    int ch, st;
    char name[SURV_NAME_LEN], loc[LOC_LEN];
    Survivor s;
    do {
        puts("\n--- SURVIVOR REGISTRY (Trie) ---");
        puts("1. Display all  2. Register  3. Exact search  4. Prefix search");
        puts("5. Update status  6. Update location  7. List by status  8. Counts  9. Delete  0. Back");
        if (!read_int("Choice: ", &ch)) return;
        switch (ch) {
        case 1: trie_display_all(); break;
        case 2:
            memset(&s, 0, sizeof s);
            if (read_text("Name (letters/spaces): ", s.name, sizeof s.name) &&
                read_int("Age: ", &s.age) && read_text("Location: ", s.location, sizeof s.location) &&
                read_int("Status 0=Missing 1=Safe 2=Injured 3=Evacuated: ", &s.status)) {
                s.id = nextSurvivorId;
                if (trie_insert(s)) { nextSurvivorId++; puts("Registered."); }
                else puts("Duplicate or invalid name.");
            }
            break;
        case 3:
            if (read_text("Name: ", name, sizeof name)) {
                Survivor *p = trie_search(name);
                if (p) printf("#%d %s, %d, %s, %s\n", p->id, p->name, p->age, p->location, trie_status_text(p->status));
                else puts("Not found.");
            }
            break;
        case 4: if (read_text("Prefix: ", name, sizeof name)) trie_prefix_search(name); break;
        case 5:
            if (read_text("Name: ", name, sizeof name) && read_int("Status 0-3: ", &st))
                puts(trie_update_status(name, st) ? "Updated." : "Failed.");
            break;
        case 6:
            if (read_text("Name: ", name, sizeof name) && read_text("New location: ", loc, sizeof loc))
                puts(trie_update_location(name, loc) ? "Updated." : "Failed.");
            break;
        case 7: if (read_int("Status 0-3: ", &st)) trie_list_by_status(st); break;
        case 8:
            printf("Total %d | Missing %d | Safe %d | Injured %d | Evacuated %d\n", trie_count(),
                   trie_count_by_status(STATUS_MISSING), trie_count_by_status(STATUS_SAFE),
                   trie_count_by_status(STATUS_INJURED), trie_count_by_status(STATUS_EVACUATED));
            break;
        case 9: if (read_text("Name: ", name, sizeof name)) puts(trie_delete(name) ? "Deleted." : "Not found."); break;
        }
    } while (ch != 0);
}

static void menu_convoy(void)
{
    int ch, id, after;
    char loc[WP_NAME];
    do {
        puts("\n--- CONVOY TRANSIT LOG (Doubly Linked List) ---");
        puts("1. Forward route  2. Return route  3. Insert detour after waypoint");
        puts("4. Remove waypoint  5. Mark waypoint BLOCKED  6. Mark OPEN  7. Count  0. Back");
        if (!read_int("Choice: ", &ch)) return;
        switch (ch) {
        case 1: convoy_display_forward(); break;
        case 2: convoy_display_backward(); break;
        case 3:
            if (read_int("Insert after waypoint id: ", &after) && read_int("New waypoint id: ", &id) &&
                read_text("Location: ", loc, sizeof loc))
                puts(convoy_insert_after(after, id, loc, "OPEN") ? "Detour inserted." : "Failed.");
            break;
        case 4: if (read_int("Waypoint id: ", &id)) puts(convoy_delete(id) ? "Removed." : "Not found."); break;
        case 5: case 6:
            if (read_int("Waypoint id: ", &id))
                puts(convoy_update_status(id, ch == 5 ? "BLOCKED" : "OPEN") ? "Updated." : "Not found.");
            break;
        case 7: printf("Waypoints: %zu\n", convoy_count()); break;
        }
    } while (ch != 0);
}

static void menu_teams(void)
{
    int ch;
    char name[30], spec[20];
    MedTeam m;
    do {
        puts("\n--- MEDICAL TEAMS (Circular Queue, round-robin) ---");
        puts("1. Add team  2. Assign next team  3. Next in line  4. Take team off duty");
        puts("5. Display  6. Status  0. Back");
        if (!read_int("Choice: ", &ch)) return;
        switch (ch) {
        case 1:
            if (read_text("Team name: ", name, sizeof name) && read_text("Specialty: ", spec, sizeof spec)) {
                int id = cq_enqueue(name, spec);
                if (id) printf("Team #%d on duty.\n", id); else puts("Queue FULL.");
            }
            break;
        case 2:
            if (cq_assign_next(&m)) printf("Team %s sent (mission #%d), moved to rear.\n", m.teamName, m.missions);
            else puts("No teams.");
            break;
        case 3: if (cq_peek(&m)) printf("Next: %s (%s)\n", m.teamName, m.specialty); else puts("No teams."); break;
        case 4: if (cq_dequeue(&m)) printf("%s off duty.\n", m.teamName); else puts("No teams."); break;
        case 5: cq_display(); break;
        case 6: cq_status(); break;
        }
    } while (ch != 0);
}

static void menu_undo(void)
{
    int ch;
    do {
        puts("\n--- UNDO / REDO (Stack) ---");
        puts("1. Undo  2. Redo  3. Show history  0. Back");
        if (!read_int("Choice: ", &ch)) return;
        if (ch == 1) undo();
        else if (ch == 2) redo();
        else if (ch == 3) stack_display();
    } while (ch != 0);
}

/* ================= MAIN ================= */
int main(void)
{
    int ch;
    flood_reset();
    seed_data();

    do {
        puts("\n==================================================");
        puts("   DISASTER RELIEF COORDINATION SYSTEM");
        puts("==================================================");
        printf(" Pending SOS: %d | Survivors: %d | Teams: %d | Undo: %zu\n",
               heap_size(), trie_count(), cq_count(), stack_undo_count());
        puts(" 1. Flood Grid          (2D Array)");
        puts(" 2. Depot Inventory     (Hash Table)");
        puts(" 3. Emergencies / SOS   (Max Heap)");
        puts(" 4. Road Network        (Graph + Dijkstra)");
        puts(" 5. Survivor Registry   (Trie)");
        puts(" 6. Convoy Transit Log  (Doubly Linked List)");
        puts(" 7. Medical Teams       (Circular Queue)");
        puts(" 8. Undo / Redo         (Stack)");
        puts(" 9. >> DISPATCH TOP EMERGENCY (uses all modules)");
        puts(" 0. Exit");
        if (!read_int("Choice: ", &ch)) break;

        switch (ch) {
        case 1: menu_flood(); break;
        case 2: menu_inventory(); break;
        case 3: menu_emergency(); break;
        case 4: menu_roads(); break;
        case 5: menu_survivors(); break;
        case 6: menu_convoy(); break;
        case 7: menu_teams(); break;
        case 8: menu_undo(); break;
        case 9: dispatch_top_emergency(); break;
        case 0: puts("Shutting down. Freeing memory..."); break;
        default: puts("Invalid choice.");
        }
    } while (ch != 0);

    graph_free(&g);
    for (int i = 0; i < NUM_DEPOTS; i++) ht_free(&depots[i]);
    trie_free();
    convoy_clear();
    stack_free();
    return 0;
}
