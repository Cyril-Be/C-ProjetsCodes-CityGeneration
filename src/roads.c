/*
 * roads.c — generate the street network.
 *
 * Two-phase generation
 * --------------------
 * Phase 1 – Radial arteries
 *   6–8 straight roads radiate from the city centre toward the map edges,
 *   like spokes on a wheel.  The starting angle is randomised so no two
 *   cities look the same.
 *
 * Phase 2 – Orthogonal grid
 *   Horizontal and vertical road lines are placed at intervals that depend
 *   on the distance of that row / column from the city centre:
 *
 *     distance < 8   →  spacing ~4   (dense inner grid)
 *     distance < 18  →  spacing ~7
 *     distance < 30  →  spacing ~10
 *     distance ≥ 30  →  spacing ~14  (sparse outer grid)
 *
 *   Each road gets a small ±1 jitter so the result looks less mechanical.
 *
 * Roads are drawn ONLY on empty land — they stop at river banks.
 * generate_bridges() then fills in the gaps with bridge cells wherever a
 * road on one side faces water and a road on the other side of the river.
 */
#include "city.h"

/* ── helpers ──────────────────────────────────────────────────────────── */

/* Place a road on land only; never overwrite water. */
static void set_road_cell(Map *map, int x, int y)
{
    if (!map_in_bounds(map, x, y)) return;
    Cell *c = &map->grid[y][x];
    if (c->type == CELL_EMPTY)
        c->type = CELL_ROAD;
}

/* Bresenham line — used for diagonal arteries */
static void draw_diag_road(Map *map, int x0, int y0, int x1, int y1)
{
    int dx  =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy  = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int cx  = x0, cy = y0;

    for (;;) {
        set_road_cell(map, cx, cy);
        if (cx == x1 && cy == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; cx += sx; }
        if (e2 <= dx) { err += dx; cy += sy; }
    }
}

/* Full horizontal road line — skips water cells */
static void draw_road_h(Map *map, int y)
{
    for (int x = 0; x < map->width; x++)
        set_road_cell(map, x, y);
}

/* Full vertical road line — skips water cells */
static void draw_road_v(Map *map, int x)
{
    for (int y = 0; y < map->height; y++)
        set_road_cell(map, x, y);
}

/* Return grid spacing for a given distance from the city centre */
static int road_spacing(Map *map, float dist)
{
    if      (dist < 8.0f)  return 4 + map_rand_range(map, 0, 2);
    else if (dist < 18.0f) return 6 + map_rand_range(map, 0, 3);
    else if (dist < 30.0f) return 9 + map_rand_range(map, 0, 3);
    else                   return 13 + map_rand_range(map, 0, 4);
}

/* ── public entry point ────────────────────────────────────────────────── */

void generate_roads(Map *map)
{
    int   cx        = map->center_x;
    int   cy        = map->center_y;
    float max_dist  = sqrtf((float)(map->width  * map->width  +
                                    map->height * map->height));

    /* ── Phase 1: radial arteries ──────────────────────────────────────── */
    int   num_art   = 6 + map_rand_range(map, 0, 3);   /* 6–8 spokes */
    float base_ang  = map_randf(map) * 3.14159f / (float)num_art;

    for (int i = 0; i < num_art; i++) {
        float angle = base_ang + (float)i * 2.0f * 3.14159f / (float)num_art;
        int   ex    = cx + (int)(cosf(angle) * max_dist);
        int   ey    = cy + (int)(sinf(angle) * max_dist);

        /* Clamp to map bounds */
        if (ex < 0)              ex = 0;
        if (ex >= map->width)    ex = map->width  - 1;
        if (ey < 0)              ey = 0;
        if (ey >= map->height)   ey = map->height - 1;

        draw_diag_road(map, cx, cy, ex, ey);
    }

    /* ── Phase 2a: horizontal grid lines ──────────────────────────────── */
    {
        int y = 1;
        while (y < map->height - 1) {
            float dist    = fabsf((float)(y - cy));
            int   spacing = road_spacing(map, dist);
            int   jitter  = map_rand_range(map, -1, 2);
            int   road_y  = y + jitter;

            if (road_y >= 0 && road_y < map->height)
                draw_road_h(map, road_y);

            y += spacing;
        }
    }

    /* ── Phase 2b: vertical grid lines ────────────────────────────────── */
    {
        int x = 1;
        while (x < map->width - 1) {
            float dist    = fabsf((float)(x - cx));
            int   spacing = road_spacing(map, dist);
            int   jitter  = map_rand_range(map, -1, 2);
            int   road_x  = x + jitter;

            if (road_x >= 0 && road_x < map->width)
                draw_road_v(map, road_x);

            x += spacing;
        }
    }
}

