/**
 * city.h — shared types, map structure, and generation function prototypes
 * for the procedural city generation project.
 */
#ifndef CITY_H
#define CITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Map dimensions ─────────────────────────────────────────────────────── */
#define MAP_WIDTH  120
#define MAP_HEIGHT  60

/* ── Cell type ──────────────────────────────────────────────────────────── */
typedef enum {
    CELL_EMPTY    = 0,
    CELL_WATER    = 1,
    CELL_ROAD     = 2,
    CELL_BRIDGE   = 3,
    CELL_BUILDING = 4,
    CELL_PARK     = 5
} CellType;

/* ── District / zone type ───────────────────────────────────────────────── */
typedef enum {
    DISTRICT_NONE        = 0,
    DISTRICT_CENTER      = 1,  /* dense commercial core          */
    DISTRICT_COMMERCIAL  = 2,  /* shops, offices                 */
    DISTRICT_RESIDENTIAL = 3,  /* housing                        */
    DISTRICT_INDUSTRIAL  = 4,  /* factories, warehouses          */
    DISTRICT_PARK        = 5   /* green space (near water etc.)  */
} DistrictType;

/* ── Single map cell ────────────────────────────────────────────────────── */
typedef struct {
    CellType     type;
    DistrictType district;
    int          height;   /* building height 1-10, 0 if not a building */
} Cell;

/* ── City map ───────────────────────────────────────────────────────────── */
typedef struct {
    int          width;
    int          height;
    Cell         grid[MAP_HEIGHT][MAP_WIDTH];
    unsigned int rng;      /* LCG state — guarantees reproducible output */
    int          center_x; /* cached city-center column                  */
    int          center_y; /* cached city-center row                     */
} Map;

/* ── Core utilities ─────────────────────────────────────────────────────── */
void         map_init(Map *map, unsigned int seed);
int          map_in_bounds(const Map *map, int x, int y);
unsigned int map_rand(Map *map);
float        map_randf(Map *map);
int          map_rand_range(Map *map, int lo, int hi); /* [lo, hi) */

/* ── Generation pipeline (call in order) ────────────────────────────────── */
void generate_waterways(Map *map);   /* step 1 */
void generate_districts(Map *map);   /* step 2 */
void generate_roads(Map *map);       /* step 3 */
void generate_bridges(Map *map);     /* step 4 */
void generate_buildings(Map *map);   /* step 5 */
void generate_parks(Map *map);       /* step 6 */

/* ── Rendering ──────────────────────────────────────────────────────────── */
void render_terminal(const Map *map);
void render_ppm(const Map *map, const char *filename);

#endif /* CITY_H */
