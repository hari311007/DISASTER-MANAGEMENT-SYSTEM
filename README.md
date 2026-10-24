# Disaster Relief System

## Build & Run
(main.c #includes all module .c files, so only main.c is compiled.)
```
gcc main.c -o relief
./relief
```

## Modules
| File | Data Structure | Role |
|---|---|---|
| flood_grid.c | 2D Array + flood fill | Water spread over 8x10 city grid |
| graph.c | Weighted directed graph + Dijkstra | Roads, fastest safe route, nearest hub |
| heap.c | Max Heap | SOS priority (severity > people > id) |
| hash_table.c | Hash table (chaining) | Depot stock per hub (SKU -> item) |
| trie.c | Trie | Survivor registry, prefix search |
| stack.c | Two linked stacks | Undo / Redo of allocations & road blocks |
| circular_queue.c | Circular queue | Round-robin medical teams |
| convoy.c | Doubly linked list | Convoy waypoints (outbound / return) |
| main.c | — | Shared state, menus, integration |

## Integrated flows
**Flood -> Graph:** every graph node sits on a grid cell. Simulating a flood blocks all roads into flooded nodes; each block is pushed on the undo stack.

**Dispatch (menu 9):**
1. HEAP: extract most urgent SOS
2. GRAPH: nearest hub (Dijkstra from each hub), then shortest route
3. HASH: deduct supplies from that hub's depot (Medical->MED-001, Food->FOOD-010, Shelter->TENT-005, Rescue->WAT-002)
4. CONVOY: route nodes loaded as waypoints
5. QUEUE (Medical) assigns next team / TRIE (Rescue) lists missing persons
6. STACK: allocation recorded for undo

If no route or no stock, the SOS is re-inserted into the heap.

**Undo/Redo:** undo returns stock or reopens a road; redo re-applies it. A new action clears redo.
