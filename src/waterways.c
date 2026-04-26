/*
 * waterways.c — procedural river generation.
 *
 * Algorithm
 * ---------
 * 1. Number of rivers is taken from params (0 = auto → 1 or 2).
 * 2. For each river choose a start edge and a different end edge.
 * 3. Build 6 intermediate control points with random perpendicular jitter
 *    (more control points than v1 → smoother, more natural meanders).
 * 4. Trace Bresenham segments between consecutive control points, drawing
 *    each cell with a width determined by params->river_width.
 *
 * For medieval cities the waterways are intentionally slightly wider and
 * more meandering, reflecting the historical role of rivers as city-founders.
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

void generate_waterways(Map *map, const CityParams *params)
{
    int medieval = (map->city_type == CITY_MEDIEVAL);

    /* Determine number of rivers */
    int num_rivers;
    if (params->num_rivers > 0)
        num_rivers = params->num_rivers;
    else
        num_rivers = 1 + map_rand_range(map, 0, 2); /* 1 or 2 */

    /* River half-width from params (1 = ±1, 2 = ±2, 3 = ±3) */
    int base_half_w = params->river_width; /* 1–3 */

    for (int r = 0; r < num_rivers; r++) {
        int sx, sy, tx, ty;

        /* Choose flow axis: top→bottom or left→right */
        if (map_rand(map) & 1) {
            sx = map_rand_range(map, 20, map->width  - 20);
            sy = 0;
            tx = map_rand_range(map, 20, map->width  - 20);
            ty = map->height - 1;
        } else {
            sx = 0;
            sy = map_rand_range(map, 12, map->height - 12);
            tx = map->width - 1;
            ty = map_rand_range(map, 12, map->height - 12);
        }

        /* 6 intermediate control points for a natural meander */
        int num_ctrl = 6;
        int ptx[8], pty[8];
        ptx[0]            = sx; pty[0]            = sy;
        ptx[num_ctrl + 1] = tx; pty[num_ctrl + 1] = ty;

        /* Medieval rivers meander more */
        int jitter_x = medieval ? 20 : 14;
        int jitter_y = medieval ? 12 :  8;

        for (int i = 1; i <= num_ctrl; i++) {
            float t  = (float)i / (float)(num_ctrl + 1);
            int   bx = (int)(sx + t * (tx - sx));
            int   by = (int)(sy + t * (ty - sy));

            ptx[i] = bx + map_rand_range(map, -jitter_x, jitter_x + 1);
            pty[i] = by + map_rand_range(map, -jitter_y, jitter_y + 1);

            /* Keep inside map */
            if (ptx[i] < 3)               ptx[i] = 3;
            if (ptx[i] >= map->width  - 3) ptx[i] = map->width  - 4;
            if (pty[i] < 3)               pty[i] = 3;
            if (pty[i] >= map->height - 3) pty[i] = map->height - 4;
        }

        /* Width: medieval rivers are a touch wider */
        int half_w = base_half_w + (medieval ? map_rand_range(map, 0, 2) : 0);

        for (int i = 0; i <= num_ctrl; i++)
            draw_line(map,
                      ptx[i], pty[i], ptx[i + 1], pty[i + 1],
                      half_w, CELL_WATER);
    }
}
