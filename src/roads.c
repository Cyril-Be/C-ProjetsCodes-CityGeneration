/*
 * roads.c — generate the street network.
 *
 * Two completely different algorithms are used depending on the city type.
 *
 * Medieval mode — organic, historically realistic
 * -----------------------------------------------
 * 1. Primary arteries: 5–7 "gate" points are placed on a rough ellipse
 *    around the city centre.  A meandering road connects the centre to each
 *    gate, and adjacent gates are joined by a ring road (like a city wall
 *    road).  An inner ring close to the centre is also drawn.
 *
 * 2. Secondary streets: a random 8 % of existing road cells sprout short
 *    (6–15 cell) random-walk branches that stop when they hit water or
 *    another road.  This creates the characteristic web of alleys and
 *    back-streets seen in medieval cities.
 *
 * Meandering roads use the "waypoint jitter" technique: intermediate
 * control points are displaced perpendicularly to the road direction using
 * a bell-shaped envelope (maximum deviation at the midpoint, zero at the
 * endpoints), then Bresenham segments connect consecutive waypoints.
 *
 * Modern mode — planned, structured
 * ----------------------------------
 * 1. Three ring roads (inner boulevard, outer ring, highway belt) drawn as
 *    slightly-irregular polygons.
 * 2. Radial arteries from the centre to the map edge (very slight curvature).
 * 3. Two diagonal boulevards at ~45°.
 * 4. Orthogonal grid fill with spacing that increases with distance from the
 *    city centre, identical in principle to v1 but now paired with the rings.
 *
 * In both modes roads never overwrite water cells; generate_bridges() later
 * reconnects interrupted roads across rivers.
 */
#include "city.h"

#define PI_F 3.14159265f

/* ── Shared helpers ─────────────────────────────────────────────────────── */

/* Place a road on land only. */
static void set_road_cell(Map *map, int x, int y)
{
    if (!map_in_bounds(map, x, y)) return;
    Cell *c = &map->grid[y][x];
    if (c->type == CELL_EMPTY)
        c->type = CELL_ROAD;
}

/* Bresenham line — the fundamental building block. */
static void draw_diag_road(Map *map, int x0, int y0, int x1, int y1)
{
    int dx  =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy  = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int cx  = x0, cy = y0;

    for (;;) {
        set_road_cell(map, cx, cy);
        if (cx == x1 && cy == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; cx += sx; }
        if (e2 <= dx) { err += dx; cy += sy; }
    }
}

/* Full horizontal road line. */
static void draw_road_h(Map *map, int y)
{
    for (int x = 0; x < map->width; x++)
        set_road_cell(map, x, y);
}

/* Full vertical road line. */
static void draw_road_v(Map *map, int x)
{
    for (int y = 0; y < map->height; y++)
        set_road_cell(map, x, y);
}

/*
 * draw_waypoint_road — draw a naturally curved road between two points.
 *
 * Intermediate waypoints are placed along the straight line between the
 * endpoints and displaced perpendicular to that line by a random amount
 * weighted by a bell-shaped envelope (0 at the endpoints, maximum in the
 * middle).  jitter_factor controls the maximum lateral deviation as a
 * fraction of the road length.
 */
static void draw_waypoint_road(Map *map,
                                int x0, int y0, int x1, int y1,
                                float jitter_factor)
{
    float dx  = (float)(x1 - x0);
    float dy  = (float)(y1 - y0);
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;

    /* Unit perpendicular vector */
    float px = -dy / len;
    float py =  dx / len;

    /* Number of intermediate waypoints scales with road length */
    int num_pts = (int)(len / 10.0f) + 2;
    if (num_pts < 2)  num_pts = 2;
    if (num_pts > 10) num_pts = 10;

    /* Stack-allocated waypoints (max 10 intermediate + 2 endpoints = 12) */
    int wpx[12], wpy[12];
    wpx[0]           = x0; wpy[0]           = y0;
    wpx[num_pts - 1] = x1; wpy[num_pts - 1] = y1;

    for (int i = 1; i < num_pts - 1; i++) {
        float t   = (float)i / (float)(num_pts - 1);
        float bx  = (float)x0 + t * dx;
        float by  = (float)y0 + t * dy;

        /* Bell envelope: 0 at endpoints, 1 at midpoint */
        float env    = t * (1.0f - t) * 4.0f;
        float jitter = (map_randf(map) - 0.5f) * 2.0f * jitter_factor * len * env;

        wpx[i] = (int)(bx + px * jitter);
        wpy[i] = (int)(by + py * jitter);

        /* Clamp */
        if (wpx[i] < 1)              wpx[i] = 1;
        if (wpx[i] >= map->width-1)  wpx[i] = map->width  - 2;
        if (wpy[i] < 1)              wpy[i] = 1;
        if (wpy[i] >= map->height-1) wpy[i] = map->height - 2;
    }

    for (int i = 0; i < num_pts - 1; i++)
        draw_diag_road(map, wpx[i], wpy[i], wpx[i + 1], wpy[i + 1]);
}

/*
 * draw_ring_road — draw a ring/circle road as a polygon approximation.
 *
 * A small random jitter on the radius at each vertex makes the ring
 * look organic rather than perfectly circular.
 */
static void draw_ring_road(Map *map, int cx, int cy,
                            float radius, float jitter)
{
    int num_seg = 32;
    float r0    = radius + (map_randf(map) - 0.5f) * jitter;
    int prev_x  = cx + (int)(r0);
    int prev_y  = cy;

    /* Clamp starting point */
    if (prev_x < 1)             prev_x = 1;
    if (prev_x >= map->width-1) prev_x = map->width - 2;

    for (int i = 1; i <= num_seg; i++) {
        float angle = (float)i / (float)num_seg * 2.0f * PI_F;
        float r     = radius + (map_randf(map) - 0.5f) * jitter;
        int   nx    = cx + (int)(cosf(angle) * r);
        int   ny    = cy + (int)(sinf(angle) * r);

        if (nx < 1)             nx = 1;
        if (nx >= map->width-1) nx = map->width  - 2;
        if (ny < 1)             ny = 1;
        if (ny >= map->height-1)ny = map->height - 2;

        draw_diag_road(map, prev_x, prev_y, nx, ny);
        prev_x = nx;
        prev_y = ny;
    }
}

/* Grid spacing for modern road fill (increases with distance from centre). */
static int road_spacing(Map *map, float dist)
{
    if      (dist <  8.0f) return 4 + map_rand_range(map, 0, 2);
    else if (dist < 20.0f) return 6 + map_rand_range(map, 0, 3);
    else if (dist < 35.0f) return 9 + map_rand_range(map, 0, 3);
    else                   return 13 + map_rand_range(map, 0, 4);
}

/* ── Medieval road generator ────────────────────────────────────────────── */

static void generate_roads_medieval(Map *map)
{
    int cx = map->center_x;
    int cy = map->center_y;

    /* ── Primary arteries: centre → N gates ──────────────────────────── */
    int num_gates = 5 + map_rand_range(map, 0, 3);  /* 5–7 gates */
    int gate_x[8], gate_y[8];
    float base_ang = map_randf(map) * 2.0f * PI_F;

    for (int i = 0; i < num_gates; i++) {
        float angle  = base_ang + (float)i / (float)num_gates * 2.0f * PI_F;
        /* Map is wider than tall → scale x radius to match aspect ratio */
        float rx = 24.0f + map_randf(map) * 12.0f;  /* 24–36 horizontal */
        float ry = 16.0f + map_randf(map) *  8.0f;  /* 16–24 vertical   */

        gate_x[i] = cx + (int)(cosf(angle) * rx) + map_rand_range(map, -4, 5);
        gate_y[i] = cy + (int)(sinf(angle) * ry) + map_rand_range(map, -3, 4);

        /* Clamp gates to a safe inner region */
        if (gate_x[i] < 6)              gate_x[i] = 6;
        if (gate_x[i] >= map->width-6)  gate_x[i] = map->width  - 7;
        if (gate_y[i] < 5)              gate_y[i] = 5;
        if (gate_y[i] >= map->height-5) gate_y[i] = map->height - 6;

        draw_waypoint_road(map, cx, cy, gate_x[i], gate_y[i], 0.22f);
    }

    /* ── Ring road: connect adjacent gates ───────────────────────────── */
    for (int i = 0; i < num_gates; i++) {
        int j = (i + 1) % num_gates;
        draw_waypoint_road(map,
                           gate_x[i], gate_y[i],
                           gate_x[j], gate_y[j], 0.18f);
    }

    /* ── Inner ring near the city centre (market quarter) ────────────── */
    draw_ring_road(map, cx, cy, 9.0f, 2.5f);

    /* ── Secondary streets: random-walk branches from existing roads ──── */
    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    for (int y = 2; y < map->height - 2; y++) {
        for (int x = 2; x < map->width - 2; x++) {
            if (map->grid[y][x].type != CELL_ROAD) continue;
            if (map_rand_range(map, 0, 100) >= 8) continue; /* 8 % chance */

            int d  = map_rand_range(map, 0, 4);
            int bx = x, by = y;
            int branch_len = map_rand_range(map, 6, 16);

            for (int step = 0; step < branch_len; step++) {
                /* Small turn probability */
                if (map_rand_range(map, 0, 100) < 20)
                    d = (d + map_rand_range(map, 1, 4)) % 4;

                bx += dirs[d][0];
                by += dirs[d][1];

                if (!map_in_bounds(map, bx, by))             break;
                if (map->grid[by][bx].type == CELL_WATER)    break;
                if (map->grid[by][bx].type == CELL_ROAD)     break;
                set_road_cell(map, bx, by);
            }
        }
    }
}

/* ── Modern road generator ──────────────────────────────────────────────── */

static void generate_roads_modern(Map *map)
{
    int   cx       = map->center_x;
    int   cy       = map->center_y;
    float max_dist = sqrtf((float)(map->width  * map->width  +
                                   map->height * map->height));

    /* ── Ring roads ─────────────────────────────────────────────────── */
    draw_ring_road(map, cx, cy, 13.0f, 1.5f);  /* inner boulevard */
    draw_ring_road(map, cx, cy, 26.0f, 2.5f);  /* outer ring      */
    draw_ring_road(map, cx, cy, 42.0f, 3.5f);  /* highway belt    */

    /* ── Radial arteries (slightly curved) ───────────────────────────── */
    int   num_art  = 6 + map_rand_range(map, 0, 3);
    float base_ang = map_randf(map) * PI_F / (float)num_art;

    for (int i = 0; i < num_art; i++) {
        float angle = base_ang + (float)i * 2.0f * PI_F / (float)num_art;
        int ex = cx + (int)(cosf(angle) * max_dist);
        int ey = cy + (int)(sinf(angle) * max_dist);

        if (ex < 0)             ex = 0;
        if (ex >= map->width)   ex = map->width  - 1;
        if (ey < 0)             ey = 0;
        if (ey >= map->height)  ey = map->height - 1;

        draw_waypoint_road(map, cx, cy, ex, ey, 0.03f);
    }

    /* ── Diagonal boulevards (~45°) ─────────────────────────────────── */
    int num_diag = 2 + map_rand_range(map, 0, 2);
    for (int i = 0; i < num_diag; i++) {
        float angle = (float)i * PI_F / (float)num_diag + 0.39f;
        int ex1 = cx + (int)(cosf(angle) * max_dist);
        int ey1 = cy + (int)(sinf(angle) * max_dist);
        int ex2 = cx - (int)(cosf(angle) * max_dist);
        int ey2 = cy - (int)(sinf(angle) * max_dist);

        /* Clamp */
        if (ex1 < 0)            ex1 = 0;
        if (ex1 >= map->width)  ex1 = map->width  - 1;
        if (ey1 < 0)            ey1 = 0;
        if (ey1 >= map->height) ey1 = map->height - 1;
        if (ex2 < 0)            ex2 = 0;
        if (ex2 >= map->width)  ex2 = map->width  - 1;
        if (ey2 < 0)            ey2 = 0;
        if (ey2 >= map->height) ey2 = map->height - 1;

        draw_waypoint_road(map, ex1, ey1, ex2, ey2, 0.02f);
    }

    /* ── Orthogonal grid fill ────────────────────────────────────────── */
    /* Horizontal */
    {
        int y = 1;
        while (y < map->height - 1) {
            float dist    = fabsf((float)(y - cy));
            int   spacing = road_spacing(map, dist);
            int   jitter  = map_rand_range(map, -1, 2);
            int   road_y  = y + jitter;
            if (road_y >= 0 && road_y < map->height)
                draw_road_h(map, road_y);
            y += spacing;
        }
    }
    /* Vertical */
    {
        int x = 1;
        while (x < map->width - 1) {
            float dist    = fabsf((float)(x - cx));
            int   spacing = road_spacing(map, dist);
            int   jitter  = map_rand_range(map, -1, 2);
            int   road_x  = x + jitter;
            if (road_x >= 0 && road_x < map->width)
                draw_road_v(map, road_x);
            x += spacing;
        }
    }
}

/* ── Public entry point ─────────────────────────────────────────────────── */

void generate_roads(Map *map)
{
    if (map->city_type == CITY_MEDIEVAL)
        generate_roads_medieval(map);
    else
        generate_roads_modern(map);
}
