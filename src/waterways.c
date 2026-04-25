/*
 * waterways.c — procedural river generation.
 *
 * Algorithm
 * ---------
 * 1. Pick 1 or 2 rivers.
 * 2. For each river choose a start point on one map edge and an end point
 *    on a different edge (so rivers always cross the map).
 * 3. Create 4 intermediate control points along the straight line between
 *    the endpoints and add random jitter so the river meanders.
 * 4. Trace Bresenham segments between consecutive control points, drawing
 *    each cell with a width of 1 or 2 extra cells on each side.
 */
#include "city.h"

/* Draw a thick Bresenham line with the given cell type. */
static void draw_line(Map *map, int x0, int y0, int x1, int y1,
                      int half_w, CellType type)
{
    int dx  =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy  = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int cx  = x0, cy = y0;

    for (;;) {
        for (int ry = -half_w; ry <= half_w; ry++) {
            for (int rx = -half_w; rx <= half_w; rx++) {
                int nx = cx + rx, ny = cy + ry;
                if (map_in_bounds(map, nx, ny))
                    map->grid[ny][nx].type = type;
            }
        }
        if (cx == x1 && cy == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; cx += sx; }
        if (e2 <= dx) { err += dx; cy += sy; }
    }
}

void generate_waterways(Map *map)
{
    int num_rivers = 1 + map_rand_range(map, 0, 2); /* 1 or 2 */

    for (int r = 0; r < num_rivers; r++) {
        int sx, sy, tx, ty;

        /* Choose flow axis: top→bottom or left→right */
        if (map_rand(map) & 1) {
            sx = map_rand_range(map, 15, map->width  - 15);
            sy = 0;
            tx = map_rand_range(map, 15, map->width  - 15);
            ty = map->height - 1;
        } else {
            sx = 0;
            sy = map_rand_range(map, 10, map->height - 10);
            tx = map->width - 1;
            ty = map_rand_range(map, 10, map->height - 10);
        }

        /* Build control points */
        int num_ctrl = 4;
        int ptx[6], pty[6];
        ptx[0]            = sx; pty[0]            = sy;
        ptx[num_ctrl + 1] = tx; pty[num_ctrl + 1] = ty;

        for (int i = 1; i <= num_ctrl; i++) {
            float t  = (float)i / (float)(num_ctrl + 1);
            int   bx = (int)(sx + t * (tx - sx));
            int   by = (int)(sy + t * (ty - sy));

            ptx[i] = bx + map_rand_range(map, -14, 15);
            pty[i] = by + map_rand_range(map, -8,   9);

            /* Clamp so river stays inside the map */
            if (ptx[i] < 2)               ptx[i] = 2;
            if (ptx[i] >= map->width  - 2) ptx[i] = map->width  - 3;
            if (pty[i] < 2)               pty[i] = 2;
            if (pty[i] >= map->height - 2) pty[i] = map->height - 3;
        }

        int half_w = 1 + map_rand_range(map, 0, 2); /* 1 or 2 extra cells */

        for (int i = 0; i <= num_ctrl; i++)
            draw_line(map,
                      ptx[i], pty[i], ptx[i + 1], pty[i + 1],
                      half_w, CELL_WATER);
    }
}
