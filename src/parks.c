/*
 * parks.c — place green space across the city.
 *
 * Two sources of parks:
 *
 * 1. DISTRICT_PARK cells (waterfront strips assigned in districts.c) that
 *    are still CELL_EMPTY after buildings.c has run.  These are converted
 *    to CELL_PARK.
 *
 * 2. A random 4 % of CELL_BUILDING cells in the RESIDENTIAL district are
 *    converted to small pocket parks.  This gives the residential areas a
 *    scattered, lived-in feel.
 */
#include "city.h"

void generate_parks(Map *map)
{
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            Cell *cell = &map->grid[y][x];

            /* Waterfront park strips */
            if (cell->type == CELL_EMPTY &&
                cell->district == DISTRICT_PARK) {
                cell->type = CELL_PARK;
                continue;
            }

            /* Random pocket parks in residential blocks — 1/25 ≈ 4% chance */
            if (cell->type     == CELL_BUILDING &&
                cell->district == DISTRICT_RESIDENTIAL &&
                map_rand_range(map, 0, 25) == 0) {  /* [0, 25) → hits 0 ~4 % */
                cell->type   = CELL_PARK;
                cell->height = 0;
            }
        }
    }
}
