/* Flood Grid module (2D array) */
#include <stdio.h>
#include "flood_grid.h"

/* Ground elevation of each cell in metres */
static int elevation[ROWS][COLS] = {
    {9, 9, 8, 7, 7, 6, 6, 7, 8, 9},
    {9, 8, 6, 5, 4, 4, 5, 6, 8, 9},
    {8, 6, 4, 3, 2, 2, 3, 5, 7, 8},
    {7, 5, 3, 2, 1, 1, 2, 4, 6, 8},
    {7, 5, 3, 2, 1, 2, 3, 5, 7, 9},
    {8, 6, 4, 3, 3, 4, 5, 6, 8, 9},
    {9, 7, 6, 5, 5, 6, 7, 8, 9, 9},
    {9, 9, 8, 7, 7, 8, 8, 9, 9, 9}
};

/* Flood state of each cell: DRY or FLOODED */
static int floodMap[ROWS][COLS];

void flood_reset(void)
{
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            floodMap[i][j] = DRY;
}

void flood_display_elevation(void)
{
    printf("\nElevation map (metres):\n     ");
    for (int j = 0; j < COLS; j++)
        printf("%3d", j);
    printf("\n");
    for (int i = 0; i < ROWS; i++) {
        printf("%3d |", i);
        for (int j = 0; j < COLS; j++)
            printf("%3d", elevation[i][j]);
        printf("\n");
    }
}

void flood_display_map(void)
{
    printf("\nFlood map ( ~ = flooded, . = dry ):\n     ");
    for (int j = 0; j < COLS; j++)
        printf("%3d", j);
    printf("\n");
    for (int i = 0; i < ROWS; i++) {
        printf("%3d |", i);
        for (int j = 0; j < COLS; j++)
            printf("%3c", floodMap[i][j] == FLOODED ? '~' : '.');
        printf("\n");
    }
}

/* Recursive flood fill: water spreads to connected cells below waterLevel */
static void floodFill(int r, int c, int waterLevel)
{
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS)
        return;                         /* outside the city */
    if (floodMap[r][c] == FLOODED)
        return;                         /* already under water */
    if (elevation[r][c] >= waterLevel)
        return;                         /* high ground, water stops */

    floodMap[r][c] = FLOODED;

    floodFill(r - 1, c, waterLevel);    /* up    */
    floodFill(r + 1, c, waterLevel);    /* down  */
    floodFill(r, c - 1, waterLevel);    /* left  */
    floodFill(r, c + 1, waterLevel);    /* right */
}

int flood_simulate(int r, int c, int waterLevel)
{
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS) {
        printf("Invalid breach point.\n");
        return -1;
    }
    if (elevation[r][c] >= waterLevel) {
        printf("Breach cell (%d,%d) is at %d m, water level %d m: no flooding.\n",
               r, c, elevation[r][c], waterLevel);
        return 0;
    }
    floodFill(r, c, waterLevel);
    printf("Flood simulated from (%d,%d) with water level %d m.\n", r, c, waterLevel);
    flood_display_map();
    int n = 0;
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            n += floodMap[i][j] == FLOODED;
    return n;
}

int flood_is_flooded(int r, int c)
{
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return 0;
    return floodMap[r][c] == FLOODED;
}

float flood_report(void)
{
    int total = 0;

    printf("\nFlooded cells per row (sector):\n");
    for (int i = 0; i < ROWS; i++) {
        int rowCount = 0;
        for (int j = 0; j < COLS; j++)
            if (floodMap[i][j] == FLOODED)
                rowCount++;
        printf("  Row %d : %d cell(s)\n", i, rowCount);
        total += rowCount;
    }

    float percent = (total * 100.0f) / (ROWS * COLS);
    printf("Total flooded cells : %d of %d\n", total, ROWS * COLS);
    printf("Area under water    : %.2f%%\n", percent);

    if (percent >= 40)
        printf("Severity            : HIGH - deploy boats and airlift supplies\n");
    else if (percent >= 15)
        printf("Severity            : MEDIUM - deploy road convoys to the edge\n");
    else if (percent > 0)
        printf("Severity            : LOW - local response teams sufficient\n");
    else
        printf("Severity            : NONE\n");
    return percent;
}

/* A good relief camp is a DRY cell that touches at least one FLOODED cell */
void flood_suggest_camps(void)
{
    int dr[4] = {-1, 1, 0, 0};
    int dc[4] = {0, 0, -1, 1};
    int found = 0;

    printf("\nSuggested relief camp sites (dry cells on the flood edge):\n");
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (floodMap[i][j] != DRY)
                continue;
            for (int k = 0; k < 4; k++) {
                int ni = i + dr[k], nj = j + dc[k];
                if (ni >= 0 && ni < ROWS && nj >= 0 && nj < COLS &&
                    floodMap[ni][nj] == FLOODED) {
                    printf("  (%d,%d) elevation %d m\n", i, j, elevation[i][j]);
                    found++;
                    break;
                }
            }
        }
    }
    if (found == 0)
        printf("  None - run a flood simulation first.\n");
    else
        printf("Total candidate sites: %d\n", found);
}

