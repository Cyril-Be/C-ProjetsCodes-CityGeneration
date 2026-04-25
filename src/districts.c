/*
 * districts.c — assign a district/zone to every non-water cell.
 *
 * Districts are determined by the normalised distance from the city centre
 * so that boundaries form roughly circular rings regardless of the map's
 * aspect ratio:
 *
 *   dist < 15  →  CENTER       (dense commercial core)
 *   dist < 30  →  COMMERCIAL   (shops / offices)
 *   dist < 55  →  RESIDENTIAL  (housing)
 *   dist ≥ 55  →  INDUSTRIAL   (factories / warehouses)
 *
 * Cells in the RESIDENTIAL zone that are immediately adjacent to water are
 * re-classed as DISTRICT_PARK (waterfront green space).
 *
 * The city centre is randomly offset from the geometric centre of the map
 * to give each generated city a different feel.
 */
#include "city.h"

void generate_districts(Map *map)
{
    /* Randomise city-centre position (already cached in map) */
    map->center_x += map_rand_range(map, -10, 11);
    map->center_y += map_rand_range(map, -6,   7);
    /* Clamp */
    if (map->center_x < 10) map->center_x = 10;
    if (map->center_x >= map->width  - 10) map->center_x = map->width  - 11;
    if (map->center_y < 5)  map->center_y = 5;
    if (map->center_y >= map->height - 5)  map->center_y = map->height - 6;

    int cx = map->center_x;
    int cy = map->center_y;

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            Cell *cell = &map->grid[y][x];
            if (cell->type == CELL_WATER) continue;

            /* Normalise coordinates so that the map fits in a ~100-unit square */
            float ndx  = (float)(x - cx) / (float)map->width  * 100.0f;
            float ndy  = (float)(y - cy) / (float)map->height * 100.0f;
            float dist = sqrtf(ndx * ndx + ndy * ndy);

            DistrictType district;
            if      (dist < 15.0f) district = DISTRICT_CENTER;
            else if (dist < 30.0f) district = DISTRICT_COMMERCIAL;
            else if (dist < 55.0f) district = DISTRICT_RESIDENTIAL;
            else                   district = DISTRICT_INDUSTRIAL;

            /* Check orthogonal neighbours for water → waterfront park */
            if (district == DISTRICT_RESIDENTIAL) {
                int adx[] = {1, -1, 0, 0};
                int ady[] = {0,  0, 1,-1};
                for (int d = 0; d < 4; d++) {
                    int nx = x + adx[d];
                    int ny = y + ady[d];
                    if (map_in_bounds(map, nx, ny) &&
                        map->grid[ny][nx].type == CELL_WATER) {
                        district = DISTRICT_PARK;
                        break;
                    }
                }
            }

            cell->district = district;
        }
    }
}
