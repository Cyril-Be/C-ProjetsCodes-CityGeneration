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
#define MAP_WIDTH  160
#define MAP_HEIGHT  80

/* ── City type ──────────────────────────────────────────────────────────── */
typedef enum {
    CITY_MEDIEVAL = 0,
    CITY_MODERN   = 1
} CityType;

/* ── Cell type ──────────────────────────────────────────────────────────── */
typedef enum {
    CELL_EMPTY    = 0,
    CELL_WATER    = 1,
    CELL_ROAD     = 2,
    CELL_BRIDGE   = 3,
    CELL_BUILDING = 4,
    CELL_PARK     = 5,
    CELL_PLAZA    = 6,   /* market square / town plaza        */
    CELL_WALL     = 7    /* defensive city wall (medieval)     */
} CellType;

/* ── District / zone type ───────────────────────────────────────────────── */
typedef enum {
    DISTRICT_NONE        = 0,
    DISTRICT_CENTER      = 1,  /* dense commercial core          */
    DISTRICT_COMMERCIAL  = 2,  /* shops, offices                 */
    DISTRICT_RESIDENTIAL = 3,  /* housing                        */
    DISTRICT_INDUSTRIAL  = 4,  /* factories, warehouses          */
    DISTRICT_PARK        = 5   /* green space                    */
} DistrictType;

/* ── Single map cell ────────────────────────────────────────────────────── */
typedef struct {
    CellType     type;
    DistrictType district;
    int          height;   /* building height 1–10, 0 otherwise */
} Cell;

/* ── Generation parameters ──────────────────────────────────────────────── */
typedef struct {
    CityType     city_type;
    unsigned int seed;
    int          num_rivers;   /* 0 = auto (1–2), 1–3 = fixed count  */
    int          river_width;  /* 1 = narrow, 2 = normal, 3 = wide   */
    int          density;      /* 0 = sparse, 1 = normal, 2 = dense  */
} CityParams;

/* ── City map ───────────────────────────────────────────────────────────── */
typedef struct {
    int          width;
    int          height;
    Cell         grid[MAP_HEIGHT][MAP_WIDTH];
    unsigned int rng;      /* LCG state — reproducible output */
    int          center_x;
    int          center_y;
    CityType     city_type;
} Map;

/* ── Core utilities ─────────────────────────────────────────────────────── */
void         map_init(Map *map, const CityParams *params);
int          map_in_bounds(const Map *map, int x, int y);
unsigned int map_rand(Map *map);
float        map_randf(Map *map);
int          map_rand_range(Map *map, int lo, int hi); /* [lo, hi) */

/* ── Generation pipeline (call in order) ────────────────────────────────── */
void generate_waterways(Map *map, const CityParams *params); /* step 1 */
void generate_districts(Map *map);                           /* step 2 */
void generate_roads(Map *map);                               /* step 3 */
void generate_bridges(Map *map);                             /* step 4 */
void generate_buildings(Map *map, const CityParams *params); /* step 5 */
void generate_parks(Map *map);                               /* step 6 */

/* ── Rendering ──────────────────────────────────────────────────────────── */
void render_terminal(const Map *map);
void render_ppm(const Map *map, const char *filename);
int  run_gui_app(void);

#endif /* CITY_H */
