/*
 * city.c — map initialisation and core random-number-generator helpers.
 *
 * The LCG uses the Numerical Recipes constants (multiplier 1664525,
 * increment 1013904223, modulus 2^32).  A fixed seed reproduces the
 * same city every time, making generation fully deterministic.
 */
#include "city.h"

void map_init(Map *map, const CityParams *params)
{
    map->width     = MAP_WIDTH;
    map->height    = MAP_HEIGHT;
    map->rng       = params->seed;
    map->city_type = params->city_type;
    memset(map->grid, 0, sizeof(map->grid));

    /* City centre starts at the geometric centre; districts.c will shift it */
    map->center_x = map->width  / 2;
    map->center_y = map->height / 2;
}

int map_in_bounds(const Map *map, int x, int y)
{
    return (x >= 0 && x < map->width && y >= 0 && y < map->height);
}

unsigned int map_rand(Map *map)
{
    map->rng = map->rng * 1664525u + 1013904223u;
    return map->rng;
}

float map_randf(Map *map)
{
    return (float)(map_rand(map) >> 1) / (float)0x7FFFFFFFu;
}

int map_rand_range(Map *map, int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + (int)(map_rand(map) % (unsigned int)(hi - lo));
}
