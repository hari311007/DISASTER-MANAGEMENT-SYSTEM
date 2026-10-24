#ifndef FLOOD_GRID_H
#define FLOOD_GRID_H

/* FLOOD GRID: 2D Array + recursive flood fill
 * Each graph node sits on one grid cell. If its cell floods,
 * roads into that node are BLOCKED in the graph.
 */

#define ROWS 8
#define COLS 10
#define DRY     0
#define FLOODED 1

void  flood_reset(void);
void  flood_display_elevation(void);
void  flood_display_map(void);
int   flood_simulate(int r, int c, int waterLevel);   /* returns cells flooded, -1 invalid */
int   flood_is_flooded(int r, int c);
float flood_report(void);                             /* prints report, returns % area */
void  flood_suggest_camps(void);

#endif
