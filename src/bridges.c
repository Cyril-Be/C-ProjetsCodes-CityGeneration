/*
 * bridges.c — place bridges across river gaps in the road network.
 *
 * After generate_roads() runs, road lines stop at river banks.  This pass
 * scans every row and every column for the pattern:
 *
 *   ROAD … [WATER …] … ROAD
 *
 * When the water gap is short enough (≤ MAX_BRIDGE_SPAN cells) the gap is
 * filled with CELL_BRIDGE cells, connecting the two road segments across the
 * river.  Wider rivers are left without a bridge — they act as natural
 * barriers between city zones.
 *
 * A second pass converts any lone ROAD cell sandwiched between two BRIDGE
 * cells into a BRIDGE so that artery approaches look continuous.
 */
#include "city.h"

#define MAX_BRIDGE_SPAN 18  /* maximum bridgeable river width (cells) */

void generate_bridges(Map *map)
{
    /* ── Horizontal bridge scan (row by row) ─────────────────────────── */
    for (int y = 0; y < map->height; y++) {
        int x = 0;
        while (x < map->width) {
            if (map->grid[y][x].type != CELL_ROAD) { x++; continue; }

            int road_end = x;
            while (road_end + 1 < map->width &&
                   map->grid[y][road_end + 1].type == CELL_ROAD)
                road_end++;

            int gap_start = road_end + 1;
            int gap_end   = gap_start;
            while (gap_end < map->width &&
                   map->grid[y][gap_end].type == CELL_WATER)
                gap_end++;

            int gap_len = gap_end - gap_start;

            if (gap_len > 0 && gap_len <= MAX_BRIDGE_SPAN &&
                gap_end < map->width &&
                map->grid[y][gap_end].type == CELL_ROAD) {
                for (int bx = gap_start; bx < gap_end; bx++)
                    map->grid[y][bx].type = CELL_BRIDGE;
            }

            x = (gap_end > road_end) ? gap_end : road_end + 1;
        }
    }

    /* ── Vertical bridge scan (column by column) ─────────────────────── */
    for (int x = 0; x < map->width; x++) {
        int y = 0;
        while (y < map->height) {
            if (map->grid[y][x].type != CELL_ROAD) { y++; continue; }

            int road_end = y;
            while (road_end + 1 < map->height &&
                   map->grid[road_end + 1][x].type == CELL_ROAD)
                road_end++;

            int gap_start = road_end + 1;
            int gap_end   = gap_start;
            while (gap_end < map->height &&
                   map->grid[gap_end][x].type == CELL_WATER)
                gap_end++;

            int gap_len = gap_end - gap_start;

            if (gap_len > 0 && gap_len <= MAX_BRIDGE_SPAN &&
                gap_end < map->height &&
                map->grid[gap_end][x].type == CELL_ROAD) {
                for (int by = gap_start; by < gap_end; by++)
                    map->grid[by][x].type = CELL_BRIDGE;
            }

            y = (gap_end > road_end) ? gap_end : road_end + 1;
        }
    }

    /* ── Artery continuity: ROAD sandwiched between two BRIDGEs ─────── */
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            if (map->grid[y][x].type != CELL_ROAD) continue;
            int bl = (x > 0)              && map->grid[y][x-1].type == CELL_BRIDGE;
            int br = (x < map->width-1)   && map->grid[y][x+1].type == CELL_BRIDGE;
            int bu = (y > 0)              && map->grid[y-1][x].type == CELL_BRIDGE;
            int bd = (y < map->height-1)  && map->grid[y+1][x].type == CELL_BRIDGE;
            if ((bl && br) || (bu && bd))
                map->grid[y][x].type = CELL_BRIDGE;
        }
    }
}
