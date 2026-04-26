/*
 * districts.c — assign a district/zone to every non-water cell.
 *
 * Medieval cities
 * ---------------
 * The city centre is shifted toward the nearest river (historically, towns
 * grew at river crossings).  District boundaries are made organic by adding
 * a per-cell spatial-hash noise of ±8 normalised units to the distance used
 * for classification.
 *
 * Modern cities
 * -------------
 * Classic concentric ring zoning, with a small random offset for the centre
 * and a modest ±3 noise on boundaries so the rings are not geometrically
 * perfect.
 *
 * Waterfront park strips
 * ----------------------
 * Cells in the RESIDENTIAL zone that are orthogonally adjacent to water are
 * re-classed as DISTRICT_PARK regardless of city type.
 */
#include "city.h"

/* Spatial hash noise — consistent per (x, y) regardless of iteration order. */
static float spatial_noise(int x, int y, unsigned int seed)
{
    unsigned int h = (unsigned int)(x * 374761393) ^ (unsigned int)(y * 668265263) ^ seed;
    h  = (h ^ (h >> 13)) * 1540483477u;
    h ^= (h >> 15);
    return (float)(h & 0xFFFFu) / 65535.0f;  /* [0, 1) */
}

void generate_districts(Map *map)
{
    /* ── 1. Randomise city-centre position ──────────────────────────────── */
    map->center_x += map_rand_range(map, -12, 13);
    map->center_y += map_rand_range(map, -7,   8);

    /* Clamp so the centre is never too close to the map edge */
    if (map->center_x < 12) map->center_x = 12;
    if (map->center_x >= map->width  - 12) map->center_x = map->width  - 13;
    if (map->center_y < 8)  map->center_y = 8;
    if (map->center_y >= map->height - 8)  map->center_y = map->height - 9;

    /* ── 2. Medieval: bias centre toward nearest river ──────────────────── */
    if (map->city_type == CITY_MEDIEVAL) {
        int   nwx = -1, nwy = -1;
        int   min_d2 = map->width * map->width + map->height * map->height;

        for (int y = 0; y < map->height; y++) {
            for (int x = 0; x < map->width; x++) {
                if (map->grid[y][x].type != CELL_WATER) continue;
                int d2 = (x - map->center_x) * (x - map->center_x)
                       + (y - map->center_y) * (y - map->center_y);
                if (d2 < min_d2) { min_d2 = d2; nwx = x; nwy = y; }
            }
        }
        if (nwx >= 0) {
            /* Shift centre 35 % of the way toward the nearest river cell */
            map->center_x = map->center_x + (nwx - map->center_x) * 35 / 100;
            map->center_y = map->center_y + (nwy - map->center_y) * 35 / 100;
        }
    }

    int cx = map->center_x;
    int cy = map->center_y;

    /* Noise amplitude: medieval = organic, modern = subtle */
    float noise_amp = (map->city_type == CITY_MEDIEVAL) ? 10.0f : 4.0f;

    /* ── 3. Assign district to every land cell ──────────────────────────── */
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            Cell *cell = &map->grid[y][x];
            if (cell->type == CELL_WATER) continue;

            /* Normalise coordinates to a ~100-unit square */
            float ndx  = (float)(x - cx) / (float)map->width  * 100.0f;
            float ndy  = (float)(y - cy) / (float)map->height * 100.0f;

            /* Per-cell noise for organic boundaries */
            float noise = (spatial_noise(x, y, map->rng) - 0.5f) * 2.0f * noise_amp;
            float dist  = sqrtf(ndx * ndx + ndy * ndy) + noise;

            DistrictType district;
            if      (dist < 14.0f) district = DISTRICT_CENTER;
            else if (dist < 28.0f) district = DISTRICT_COMMERCIAL;
            else if (dist < 55.0f) district = DISTRICT_RESIDENTIAL;
            else                   district = DISTRICT_INDUSTRIAL;

            /* Waterfront → park (overrides RESIDENTIAL only) */
            if (district == DISTRICT_RESIDENTIAL) {
                int adx[] = {1, -1, 0,  0};
                int ady[] = {0,  0, 1, -1};
                for (int d = 0; d < 4; d++) {
                    int nx = x + adx[d], ny = y + ady[d];
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
