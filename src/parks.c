/*
 * parks.c — place green space and public squares across the city.
 *
 * Three sources of open space:
 *
 * 1. DISTRICT_PARK cells (waterfront strips) that are still CELL_EMPTY after
 *    buildings.c has run → converted to CELL_PARK.
 *
 * 2. A random 4 % of CELL_BUILDING cells in RESIDENTIAL districts are turned
 *    into pocket parks.
 *
 * 3. Medieval cities only: 1–2 market plazas (CELL_PLAZA) are stamped near
 *    the city centre.  Plazas overwrite whatever is there (buildings, roads)
 *    so that a proper open square is always present at the heart of medieval
 *    cities.  Modern cities get a small central plaza too.
 */
#include "city.h"

/* Stamp a rectangular plaza centred at (px, py) with given half-size. */
static void place_plaza(Map *map, int px, int py, int half)
{
    for (int dy = -half; dy <= half; dy++) {
        for (int dx = -half; dx <= half; dx++) {
            int nx = px + dx, ny = py + dy;
            if (!map_in_bounds(map, nx, ny)) continue;
            if (map->grid[ny][nx].type == CELL_WATER) continue;
            map->grid[ny][nx].type   = CELL_PLAZA;
            map->grid[ny][nx].height = 0;
        }
    }
}

void generate_parks(Map *map)
{
    /* ── 1. Waterfront park strips ───────────────────────────────────── */
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            Cell *cell = &map->grid[y][x];
            if (cell->type     == CELL_EMPTY &&
                cell->district == DISTRICT_PARK) {
                cell->type = CELL_PARK;
            }
        }
    }

    /* ── 2. Pocket parks in residential areas (~4 %) ─────────────────── */
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            Cell *cell = &map->grid[y][x];
            if (cell->type     == CELL_BUILDING          &&
                cell->district == DISTRICT_RESIDENTIAL   &&
                map_rand_range(map, 0, 25) == 0) {
                cell->type   = CELL_PARK;
                cell->height = 0;
            }
        }
    }

    /* ── 3. Central plazas ───────────────────────────────────────────── */
    int cx = map->center_x;
    int cy = map->center_y;

    if (map->city_type == CITY_MEDIEVAL) {
        /* Main market square at/near the centre */
        int half_main = 2 + map_rand_range(map, 0, 2);  /* 2–3 cells radius */
        place_plaza(map, cx, cy, half_main);

        /* Optional second plaza (church square) offset from centre */
        if (map_rand_range(map, 0, 2) == 0) {
            int ox = cx + map_rand_range(map, -10, 11);
            int oy = cy + map_rand_range(map,  -6,  7);
            if (map_in_bounds(map, ox, oy))
                place_plaza(map, ox, oy, 1 + map_rand_range(map, 0, 2));
        }
    } else {
        /* Modern city: small civic plaza in the downtown core */
        place_plaza(map, cx, cy, 1 + map_rand_range(map, 0, 2));
    }
}
