/*
 * buildings.c — fill empty land cells with buildings.
 *
 * City-type differences
 * ----------------------
 * Medieval
 *   Buildings tend to line the streets.  Empty cells that are NOT adjacent
 *   to a road have only a 30 % chance of receiving a building (inner yards,
 *   wasteland).  Building heights are modest: centre 2–4, others 1–3.
 *
 * Modern
 *   Complete block fill: every empty district cell becomes a building.
 *   Heights are much taller in the centre (6–10) and scale down outward.
 *
 * Density parameter (from CityParams)
 *   0 = sparse  → 60 % fill probability
 *   1 = normal  → 90 % fill probability
 *   2 = dense   → 100 % fill probability
 */
#include "city.h"

static int building_height(Map *map, DistrictType d, CityType type)
{
    if (type == CITY_MEDIEVAL) {
        switch (d) {
            case DISTRICT_CENTER:      return 2 + map_rand_range(map, 0, 3); /* 2–4 */
            case DISTRICT_COMMERCIAL:  return 1 + map_rand_range(map, 0, 3); /* 1–3 */
            case DISTRICT_RESIDENTIAL: return 1 + map_rand_range(map, 0, 2); /* 1–2 */
            case DISTRICT_INDUSTRIAL:  return 1 + map_rand_range(map, 0, 2); /* 1–2 */
            default:                   return 1;
        }
    } else {
        switch (d) {
            case DISTRICT_CENTER:      return 6 + map_rand_range(map, 0, 5); /* 6–10 */
            case DISTRICT_COMMERCIAL:  return 3 + map_rand_range(map, 0, 4); /* 3–6  */
            case DISTRICT_RESIDENTIAL: return 1 + map_rand_range(map, 0, 3); /* 1–3  */
            case DISTRICT_INDUSTRIAL:  return 2 + map_rand_range(map, 0, 3); /* 2–4  */
            default:                   return 1;
        }
    }
}

void generate_buildings(Map *map, const CityParams *params)
{
    /* Fill probability by density parameter */
    float fill_prob = (params->density == 0) ? 0.60f :
                      (params->density == 1) ? 0.90f : 1.00f;

    int medieval = (map->city_type == CITY_MEDIEVAL);

    int adx[] = {1, -1,  0, 0};
    int ady[] = {0,  0, -1, 1};

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            Cell *cell = &map->grid[y][x];
            if (cell->type     != CELL_EMPTY)    continue;
            if (cell->district == DISTRICT_NONE) continue;
            if (cell->district == DISTRICT_PARK) continue;

            /* Density roll */
            if (map_randf(map) > fill_prob) continue;

            /* Medieval: cells far from any road are mostly left empty */
            if (medieval && cell->district != DISTRICT_CENTER) {
                int near_road = 0;
                for (int d = 0; d < 4; d++) {
                    int nx = x + adx[d], ny = y + ady[d];
                    if (map_in_bounds(map, nx, ny)) {
                        CellType t = map->grid[ny][nx].type;
                        if (t == CELL_ROAD || t == CELL_BRIDGE) {
                            near_road = 1;
                            break;
                        }
                    }
                }
                /* Cells not adjacent to a road: only 30 % chance of building */
                if (!near_road && map_randf(map) > 0.30f) continue;
            }

            cell->type   = CELL_BUILDING;
            cell->height = building_height(map, cell->district, map->city_type);
        }
    }
}
