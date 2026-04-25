/*
 * buildings.c — fill empty land cells with buildings.
 *
 * Any cell that is:
 *   • still CELL_EMPTY after roads, waterways, and bridges have been placed,
 *   • has a district assigned (not DISTRICT_NONE or DISTRICT_PARK),
 *
 * becomes a building.  Its height reflects the district:
 *
 *   CENTER      →  5–10  (high-rise)
 *   COMMERCIAL  →  3–6   (mid-rise)
 *   RESIDENTIAL →  1–3   (low-rise)
 *   INDUSTRIAL  →  2–4   (mixed industrial)
 */
#include "city.h"

static int building_height(Map *map, DistrictType d)
{
    switch (d) {
        case DISTRICT_CENTER:      return 5 + map_rand_range(map, 0, 6);
        case DISTRICT_COMMERCIAL:  return 3 + map_rand_range(map, 0, 4);
        case DISTRICT_RESIDENTIAL: return 1 + map_rand_range(map, 0, 3);
        case DISTRICT_INDUSTRIAL:  return 2 + map_rand_range(map, 0, 3);
        default:                   return 1;
    }
}

void generate_buildings(Map *map)
{
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            Cell *cell = &map->grid[y][x];
            if (cell->type     != CELL_EMPTY)    continue;
            if (cell->district == DISTRICT_NONE) continue;
            if (cell->district == DISTRICT_PARK) continue;

            cell->type   = CELL_BUILDING;
            cell->height = building_height(map, cell->district);
        }
    }
}
