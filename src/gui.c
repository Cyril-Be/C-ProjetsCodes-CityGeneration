#include "city.h"
#include <SDL2/SDL.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

#define BASE_WINDOW_W    1280
#define BASE_WINDOW_H     920
#define HUD_HEIGHT        150
#define MAP_CELL_PX        12
#define MAP_TEX_W        (MAP_WIDTH  * MAP_CELL_PX)
#define MAP_TEX_H        (MAP_HEIGHT * MAP_CELL_PX)
#define STEP_DELAY_MS     140
#define MIN_ZOOM         0.70f
#define MAX_ZOOM         7.00f
#define ZOOM_STEP        1.15f
#define DEFAULT_ZOOM     1.45f
#define TITLE_BUFFER_SIZE 320

typedef struct {
    CityParams params;
    Map        map;
    int        has_city;
    int        generating;
    int        step_index;      /* 0..6 */
    Uint32     last_step_ticks;
    char       last_export[64];
    float      zoom;
    float      cam_x;
    float      cam_y;
    int        fullscreen;
} AppState;

typedef struct { unsigned char r, g, b; } RGB;

typedef struct {
    char c;
    unsigned char rows[7];
} Glyph;

static const Glyph GLYPHS[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {'+', {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}},
    {'-', {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
    {':', {0x00,0x04,0x00,0x00,0x00,0x04,0x00}},
    {'/', {0x01,0x02,0x04,0x08,0x10,0x00,0x00}},
    {'[', {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E}},
    {']', {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E}},
    {'0', {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2', {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
    {'3', {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}},
    {'4', {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
    {'5', {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}},
    {'6', {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}},
    {'7', {0x1F,0x01,0x02,0x04,0x08,0x10,0x10}},
    {'8', {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
    {'9', {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}},
    {'A', {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'B', {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C', {0x0F,0x10,0x10,0x10,0x10,0x10,0x0F}},
    {'D', {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
    {'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G', {0x0F,0x10,0x10,0x17,0x11,0x11,0x0F}},
    {'H', {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I', {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}},
    {'J', {0x01,0x01,0x01,0x01,0x11,0x11,0x0E}},
    {'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M', {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},
    {'N', {0x11,0x11,0x19,0x15,0x13,0x11,0x11}},
    {'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'Q', {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
    {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U', {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'V', {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W', {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}},
    {'X', {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
    {'Y', {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'Z', {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}}
};

static const unsigned int GLYPH_COUNT = sizeof(GLYPHS) / sizeof(GLYPHS[0]);

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static uint32_t hash2(int x, int y, uint32_t seed)
{
    uint32_t h = (uint32_t)(x * 374761393u) ^ (uint32_t)(y * 668265263u) ^ seed;
    h ^= h >> 13;
    h *= 1274126177u;
    h ^= h >> 16;
    return h;
}

static RGB rgb_mul(RGB c, float factor)
{
    RGB out;
    out.r = (unsigned char)clampi((int)((float)c.r * factor), 0, 255);
    out.g = (unsigned char)clampi((int)((float)c.g * factor), 0, 255);
    out.b = (unsigned char)clampi((int)((float)c.b * factor), 0, 255);
    return out;
}

static const char *river_count_label(int n)
{
    switch (n) {
        case 0: return "AUTO";
        case 1: return "1";
        case 2: return "2";
        default: return "3";
    }
}

static const char *river_width_label(int n)
{
    switch (n) {
        case 1: return "NARROW";
        case 2: return "NORMAL";
        default: return "WIDE";
    }
}

static const char *density_label(int n)
{
    switch (n) {
        case 0: return "SPARSE";
        case 1: return "NORMAL";
        default: return "DENSE";
    }
}

static const char *step_name(int idx)
{
    static const char *names[] = {
        "WATERWAYS", "DISTRICTS", "ROADS", "BRIDGES", "BUILDINGS", "PARKS"
    };
    if (idx < 0 || idx >= 6) return "DONE";
    return names[idx];
}

static const unsigned char *glyph_rows(char ch)
{
    for (unsigned int i = 0; i < GLYPH_COUNT; i++) {
        if (GLYPHS[i].c == ch) return GLYPHS[i].rows;
    }
    return GLYPHS[0].rows;
}

static void draw_char(SDL_Renderer *renderer, int x, int y, int scale,
                      SDL_Color color, char ch)
{
    unsigned char up = (unsigned char)toupper((unsigned char)ch);
    const unsigned char *rows = glyph_rows((char)up);

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (rows[row] & (1u << (4 - col))) {
                SDL_Rect pixel = {x + col * scale, y + row * scale, scale, scale};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

static void draw_text(SDL_Renderer *renderer, int x, int y, int scale,
                      SDL_Color color, const char *text)
{
    int pen_x = x;
    for (const char *p = text; *p; p++) {
        draw_char(renderer, pen_x, y, scale, color, *p);
        pen_x += 6 * scale;
    }
}

static RGB cell_colour(const Cell *cell, CityType type)
{
    RGB c;
    switch (cell->type) {
        case CELL_WATER:
            c.r = (type == CITY_MEDIEVAL) ? 30 : 40;
            c.g = (type == CITY_MEDIEVAL) ? 100 : 115;
            c.b = (type == CITY_MEDIEVAL) ? 195 : 205;
            return c;
        case CELL_ROAD:
            c.r = (type == CITY_MEDIEVAL) ? 115 : 75;
            c.g = (type == CITY_MEDIEVAL) ? 88 : 75;
            c.b = (type == CITY_MEDIEVAL) ? 60 : 80;
            return c;
        case CELL_BRIDGE:
            c.r = (type == CITY_MEDIEVAL) ? 160 : 190;
            c.g = (type == CITY_MEDIEVAL) ? 125 : 175;
            c.b = (type == CITY_MEDIEVAL) ? 70 : 155;
            return c;
        case CELL_PARK:
            c.r = (type == CITY_MEDIEVAL) ? 50 : 65;
            c.g = (type == CITY_MEDIEVAL) ? 140 : 175;
            c.b = (type == CITY_MEDIEVAL) ? 50 : 65;
            return c;
        case CELL_PLAZA:
            c.r = 200; c.g = 185; c.b = 150; return c;
        case CELL_WALL:
            c.r = 75; c.g = 65; c.b = 55; return c;
        case CELL_BUILDING:
            if (type == CITY_MEDIEVAL) {
                switch (cell->district) {
                    case DISTRICT_CENTER:      c.r = 160; c.g =  50; c.b =  50; break;
                    case DISTRICT_COMMERCIAL:  c.r = 180; c.g = 110; c.b =  55; break;
                    case DISTRICT_RESIDENTIAL: c.r =  90; c.g = 100; c.b = 150; break;
                    case DISTRICT_INDUSTRIAL:  c.r = 100; c.g =  85; c.b =  70; break;
                    default:                   c.r = 140; c.g = 130; c.b = 120; break;
                }
            } else {
                switch (cell->district) {
                    case DISTRICT_CENTER:      c.r =  40; c.g =  80; c.b = 165; break;
                    case DISTRICT_COMMERCIAL:  c.r = 120; c.g = 140; c.b = 175; break;
                    case DISTRICT_RESIDENTIAL: c.r = 200; c.g = 170; c.b = 130; break;
                    case DISTRICT_INDUSTRIAL:  c.r = 120; c.g = 110; c.b = 100; break;
                    default:                   c.r = 150; c.g = 150; c.b = 150; break;
                }
            }
            return c;
        default:
            c.r = (type == CITY_MEDIEVAL) ? 200 : 210;
            c.g = (type == CITY_MEDIEVAL) ? 185 : 210;
            c.b = (type == CITY_MEDIEVAL) ? 160 : 210;
            return c;
    }
}

static void start_generation(AppState *app)
{
    map_init(&app->map, &app->params);
    app->has_city = 0;
    app->generating = 1;
    app->step_index = 0;
    app->last_step_ticks = 0;
}

static void update_generation(AppState *app, Uint32 now_ms)
{
    if (!app->generating) return;
    if (app->last_step_ticks != 0 &&
        now_ms - app->last_step_ticks < STEP_DELAY_MS) return;

    switch (app->step_index) {
        case 0: generate_waterways(&app->map, &app->params); break;
        case 1: generate_districts(&app->map); break;
        case 2: generate_roads(&app->map); break;
        case 3: generate_bridges(&app->map); break;
        case 4: generate_buildings(&app->map, &app->params); break;
        case 5: generate_parks(&app->map); break;
        default: break;
    }

    app->step_index++;
    app->last_step_ticks = now_ms;
    if (app->step_index >= 6) {
        app->generating = 0;
        app->has_city = 1;
    }
}

static void clamp_camera(AppState *app, int view_w, int view_h)
{
    float src_w = (float)view_w / app->zoom;
    float src_h = (float)view_h / app->zoom;

    if (src_w >= (float)MAP_TEX_W) {
        app->cam_x = 0.0f;
    } else {
        app->cam_x = clampf(app->cam_x, 0.0f, (float)MAP_TEX_W - src_w);
    }

    if (src_h >= (float)MAP_TEX_H) {
        app->cam_y = 0.0f;
    } else {
        app->cam_y = clampf(app->cam_y, 0.0f, (float)MAP_TEX_H - src_h);
    }
}

static void center_camera(AppState *app, int view_w, int view_h)
{
    float src_w = (float)view_w / app->zoom;
    float src_h = (float)view_h / app->zoom;
    app->cam_x = ((float)MAP_TEX_W - src_w) * 0.5f;
    app->cam_y = ((float)MAP_TEX_H - src_h) * 0.5f;
    clamp_camera(app, view_w, view_h);
}

static void zoom_at(AppState *app, float factor, int mouse_x, int mouse_y,
                    int view_w, int view_h)
{
    float old_zoom = app->zoom;
    float new_zoom = clampf(app->zoom * factor, MIN_ZOOM, MAX_ZOOM);
    if (fabsf(new_zoom - old_zoom) < 0.0001f) return;

    float world_x = app->cam_x + (float)mouse_x / old_zoom;
    float world_y = app->cam_y + (float)mouse_y / old_zoom;

    app->zoom = new_zoom;
    app->cam_x = world_x - (float)mouse_x / new_zoom;
    app->cam_y = world_y - (float)mouse_y / new_zoom;
    clamp_camera(app, view_w, view_h);
}

static void draw_tile_base(SDL_Renderer *renderer, int x, int y, RGB col)
{
    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
    SDL_Rect r = {x, y, MAP_CELL_PX, MAP_CELL_PX};
    SDL_RenderFillRect(renderer, &r);
}

static void render_cell_detail(SDL_Renderer *renderer, const Cell *cell, CityType city_type,
                               int cx, int cy, int gx, int gy)
{
    uint32_t h = hash2(gx, gy, 0x9e3779b9u);
    RGB base = cell_colour(cell, city_type);

    float grain = 0.92f + (float)(h & 31u) / 220.0f;
    base = rgb_mul(base, grain);
    draw_tile_base(renderer, cx, cy, base);

    if (cell->type == CELL_WATER) {
        RGB top = rgb_mul(base, 1.08f);
        SDL_SetRenderDrawColor(renderer, top.r, top.g, top.b, 255);
        SDL_Rect wave1 = {cx + 1, cy + 2 + (int)(h & 1u), MAP_CELL_PX - 2, 1};
        SDL_Rect wave2 = {cx + 2, cy + 6 + (int)((h >> 1) & 1u), MAP_CELL_PX - 4, 1};
        SDL_RenderFillRect(renderer, &wave1);
        SDL_RenderFillRect(renderer, &wave2);
    } else if (cell->type == CELL_ROAD || cell->type == CELL_BRIDGE) {
        RGB edge = rgb_mul(base, 0.75f);
        RGB stripe = rgb_mul(base, 1.22f);
        SDL_SetRenderDrawColor(renderer, edge.r, edge.g, edge.b, 255);
        SDL_Rect border = {cx, cy, MAP_CELL_PX, MAP_CELL_PX};
        SDL_RenderDrawRect(renderer, &border);
        SDL_SetRenderDrawColor(renderer, stripe.r, stripe.g, stripe.b, 255);
        SDL_Rect center = {cx + MAP_CELL_PX / 2 - 1, cy + 2, 2, MAP_CELL_PX - 4};
        SDL_RenderFillRect(renderer, &center);
    } else if (cell->type == CELL_BUILDING) {
        float hshade = 0.82f + (float)cell->height * 0.04f;
        if (hshade > 1.18f) hshade = 1.18f;
        RGB roof = rgb_mul(base, hshade);
        RGB wall = rgb_mul(base, 0.67f);
        SDL_SetRenderDrawColor(renderer, roof.r, roof.g, roof.b, 255);
        SDL_Rect top = {cx + 1, cy + 1, MAP_CELL_PX - 2, MAP_CELL_PX / 2};
        SDL_RenderFillRect(renderer, &top);

        SDL_SetRenderDrawColor(renderer, wall.r, wall.g, wall.b, 255);
        SDL_Rect side = {cx + 1, cy + MAP_CELL_PX / 2, MAP_CELL_PX - 2, MAP_CELL_PX / 2 - 1};
        SDL_RenderFillRect(renderer, &side);

        RGB outline = rgb_mul(base, 0.45f);
        SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, 255);
        SDL_Rect border = {cx, cy, MAP_CELL_PX, MAP_CELL_PX};
        SDL_RenderDrawRect(renderer, &border);

        RGB window = rgb_mul(base, 1.35f);
        SDL_SetRenderDrawColor(renderer, window.r, window.g, window.b, 255);
        if (cell->height >= 5) {
            SDL_Rect win1 = {cx + 3, cy + MAP_CELL_PX / 2 + 1, 2, 1};
            SDL_Rect win2 = {cx + MAP_CELL_PX - 5, cy + MAP_CELL_PX / 2 + 2, 2, 1};
            SDL_RenderFillRect(renderer, &win1);
            SDL_RenderFillRect(renderer, &win2);
        }
    } else if (cell->type == CELL_PARK) {
        RGB leaf = rgb_mul(base, 1.20f);
        SDL_SetRenderDrawColor(renderer, leaf.r, leaf.g, leaf.b, 255);
        SDL_Rect t1 = {cx + 2 + (int)(h & 1u), cy + 2, 2, 2};
        SDL_Rect t2 = {cx + 6 + (int)((h >> 1) & 1u), cy + 6, 2, 2};
        SDL_RenderFillRect(renderer, &t1);
        SDL_RenderFillRect(renderer, &t2);
    } else {
        RGB tint = rgb_mul(base, 1.06f);
        SDL_SetRenderDrawColor(renderer, tint.r, tint.g, tint.b, 255);
        SDL_Rect sheen = {cx + 1, cy + 1, MAP_CELL_PX - 2, 1};
        SDL_RenderFillRect(renderer, &sheen);
    }
}

static void render_map_texture(SDL_Renderer *renderer, SDL_Texture *map_tex, const AppState *app)
{
    SDL_SetRenderTarget(renderer, map_tex);
    SDL_SetRenderDrawColor(renderer, 18, 20, 24, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < app->map.height; y++) {
        for (int x = 0; x < app->map.width; x++) {
            int px = x * MAP_CELL_PX;
            int py = y * MAP_CELL_PX;
            const Cell *cell = &app->map.grid[y][x];
            render_cell_detail(renderer, cell, app->map.city_type, px, py, x, y);
        }
    }

    SDL_SetRenderTarget(renderer, NULL);
}

static void render_frame(SDL_Renderer *renderer, SDL_Texture *map_tex, AppState *app)
{
    int win_w = BASE_WINDOW_W;
    int win_h = BASE_WINDOW_H;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    int map_h = win_h - HUD_HEIGHT;
    if (map_h < 120) map_h = 120;
    int hud_y = map_h;
    int hud_h = win_h - map_h;

    clamp_camera(app, win_w, map_h);

    SDL_SetRenderDrawColor(renderer, 18, 20, 24, 255);
    SDL_RenderClear(renderer);

    render_map_texture(renderer, map_tex, app);

    SDL_Rect src = {
        (int)app->cam_x,
        (int)app->cam_y,
        (int)((float)win_w / app->zoom),
        (int)((float)map_h / app->zoom)
    };
    if (src.w < 1) src.w = 1;
    if (src.h < 1) src.h = 1;
    if (src.w > MAP_TEX_W) src.w = MAP_TEX_W;
    if (src.h > MAP_TEX_H) src.h = MAP_TEX_H;
    src.x = clampi(src.x, 0, MAP_TEX_W - src.w);
    src.y = clampi(src.y, 0, MAP_TEX_H - src.h);

    SDL_Rect dst = {0, 0, win_w, map_h};
    SDL_RenderCopy(renderer, map_tex, &src, &dst);

    SDL_SetRenderDrawColor(renderer, 20, 24, 30, 245);
    SDL_Rect hud = {0, hud_y, win_w, hud_h};
    SDL_RenderFillRect(renderer, &hud);

    SDL_SetRenderDrawColor(renderer, 45, 56, 74, 255);
    SDL_Rect sep = {0, hud_y, win_w, 2};
    SDL_RenderFillRect(renderer, &sep);

    int text_scale = (win_w >= 1100) ? 2 : 1;

    SDL_Color white = {235, 238, 242, 255};
    SDL_Color info  = {140, 195, 255, 255};
    SDL_Color ok    = {120, 230, 130, 255};
    SDL_Color warn  = {245, 190, 95, 255};

    char line[256];
    snprintf(line, sizeof(line), "TYPE[T]: %s    SEED[R]: %u    ZOOM[WHEEL,+,-]: %.2fx",
             app->params.city_type == CITY_MEDIEVAL ? "MEDIEVAL" : "MODERN",
             app->params.seed, app->zoom);
    draw_text(renderer, 10, hud_y + 10, text_scale, white, line);

    snprintf(line, sizeof(line), "RIVERS[N]: %s   WIDTH[L]: %s   DENSITY[D]: %s   FULLSCREEN[F]",
             river_count_label(app->params.num_rivers),
             river_width_label(app->params.river_width),
             density_label(app->params.density));
    draw_text(renderer, 10, hud_y + 10 + 12 * text_scale, text_scale, white, line);

    if (app->generating) {
        snprintf(line, sizeof(line), "STATUS: GENERATING  STEP: %d/6  %s",
                 app->step_index, step_name(app->step_index));
        draw_text(renderer, 10, hud_y + 10 + 24 * text_scale, text_scale, warn, line);
    } else if (app->has_city) {
        snprintf(line, sizeof(line), "STATUS: DONE  STEP: 6/6");
        draw_text(renderer, 10, hud_y + 10 + 24 * text_scale, text_scale, ok, line);
    } else {
        draw_text(renderer, 10, hud_y + 10 + 24 * text_scale, text_scale, info, "STATUS: IDLE");
    }

    int completed = app->generating ? app->step_index : (app->has_city ? 6 : 0);
    SDL_Rect progress_bg = {10, hud_y + 14 + 36 * text_scale, win_w - 20, 12};
    SDL_SetRenderDrawColor(renderer, 50, 58, 70, 255);
    SDL_RenderFillRect(renderer, &progress_bg);
    SDL_Rect progress_fg = progress_bg;
    progress_fg.w = (progress_bg.w * completed) / 6;
    SDL_SetRenderDrawColor(renderer, 87, 180, 255, 255);
    SDL_RenderFillRect(renderer, &progress_fg);

    draw_text(renderer, 10, hud_y + 16 + 36 * text_scale + 14, text_scale, white,
              "CONTROLS: G/SPACE GENERATE  ARROWS MOVE  P EXPORT  Q/ESC QUIT");

    if (app->last_export[0] != '\0') {
        snprintf(line, sizeof(line), "LAST EXPORT: %s", app->last_export);
        draw_text(renderer, 10, hud_y + 18 + 48 * text_scale + 14, text_scale, info, line);
    }

    SDL_RenderPresent(renderer);
}

static void update_window_title(SDL_Window *window, const AppState *app)
{
    char title[TITLE_BUFFER_SIZE];
    snprintf(title, sizeof(title),
             "City Generator SDL | %s | Seed=%u | Zoom=%.2fx | G generate, Wheel +/- zoom, Arrows move, F fullscreen",
             app->params.city_type == CITY_MEDIEVAL ? "Medieval" : "Modern",
             app->params.seed, app->zoom);
    SDL_SetWindowTitle(window, title);
}

int run_gui_app(void)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "City Generator SDL",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        BASE_WINDOW_W, BASE_WINDOW_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        fprintf(stderr, "SDL window error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    }
    if (!renderer) {
        fprintf(stderr, "SDL renderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *map_tex = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_TARGET,
                                             MAP_TEX_W, MAP_TEX_H);
    if (!map_tex) {
        fprintf(stderr, "SDL texture error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    AppState app;
    app.params.city_type = CITY_MEDIEVAL;
    app.params.seed = (unsigned int)time(NULL);
    app.params.num_rivers = 0;
    app.params.river_width = 2;
    app.params.density = 1;
    app.has_city = 0;
    app.generating = 0;
    app.step_index = 0;
    app.last_step_ticks = 0;
    app.last_export[0] = '\0';
    app.zoom = DEFAULT_ZOOM;
    app.cam_x = 0.0f;
    app.cam_y = 0.0f;
    app.fullscreen = 0;

    map_init(&app.map, &app.params);
    start_generation(&app);

    int running = 1;
    while (running) {
        int win_w = BASE_WINDOW_W;
        int win_h = BASE_WINDOW_H;
        SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
        int map_h = win_h - HUD_HEIGHT;
        if (map_h < 120) map_h = 120;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEWHEEL) {
                int mx = win_w / 2;
                int my = map_h / 2;
                SDL_GetMouseState(&mx, &my);
                if (my < map_h) {
                    if (event.wheel.y > 0) {
                        zoom_at(&app, ZOOM_STEP, mx, my, win_w, map_h);
                    } else if (event.wheel.y < 0) {
                        zoom_at(&app, 1.0f / ZOOM_STEP, mx, my, win_w, map_h);
                    }
                }
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    clamp_camera(&app, win_w, map_h);
                }
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode key = event.key.keysym.sym;
                float pan_step = 60.0f / app.zoom;
                switch (key) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        running = 0;
                        break;
                    case SDLK_g:
                    case SDLK_SPACE:
                        start_generation(&app);
                        break;
                    case SDLK_t:
                        if (!app.generating) {
                            app.params.city_type = (app.params.city_type == CITY_MEDIEVAL)
                                                   ? CITY_MODERN : CITY_MEDIEVAL;
                        }
                        break;
                    case SDLK_r:
                        if (!app.generating) {
                            app.params.seed = (unsigned int)time(NULL) ^ (unsigned int)SDL_GetTicks();
                        }
                        break;
                    case SDLK_n:
                        if (!app.generating) {
                            app.params.num_rivers = (app.params.num_rivers + 1) % 4;
                        }
                        break;
                    case SDLK_l:
                        if (!app.generating) {
                            app.params.river_width = (app.params.river_width % 3) + 1;
                        }
                        break;
                    case SDLK_d:
                        if (!app.generating) {
                            app.params.density = (app.params.density + 1) % 3;
                        }
                        break;
                    case SDLK_p:
                        if (app.has_city && !app.generating) {
                            snprintf(app.last_export, sizeof(app.last_export),
                                     "city_%u.ppm", app.params.seed);
                            render_ppm(&app.map, app.last_export);
                        }
                        break;
                    case SDLK_f:
                    case SDLK_F11:
                        app.fullscreen = !app.fullscreen;
                        if (app.fullscreen) {
                            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        } else {
                            SDL_SetWindowFullscreen(window, 0);
                            SDL_SetWindowSize(window, BASE_WINDOW_W, BASE_WINDOW_H);
                        }
                        break;
                    case SDLK_PLUS:
                    case SDLK_EQUALS:
                    case SDLK_KP_PLUS:
                        zoom_at(&app, ZOOM_STEP, win_w / 2, map_h / 2, win_w, map_h);
                        break;
                    case SDLK_MINUS:
                    case SDLK_KP_MINUS:
                        zoom_at(&app, 1.0f / ZOOM_STEP, win_w / 2, map_h / 2, win_w, map_h);
                        break;
                    case SDLK_0:
                    case SDLK_KP_0:
                        app.zoom = DEFAULT_ZOOM;
                        center_camera(&app, win_w, map_h);
                        break;
                    case SDLK_LEFT:
                        app.cam_x -= pan_step;
                        break;
                    case SDLK_RIGHT:
                        app.cam_x += pan_step;
                        break;
                    case SDLK_UP:
                        app.cam_y -= pan_step;
                        break;
                    case SDLK_DOWN:
                        app.cam_y += pan_step;
                        break;
                    default:
                        break;
                }
            }
        }

        clamp_camera(&app, win_w, map_h);
        update_generation(&app, SDL_GetTicks());
        update_window_title(window, &app);
        render_frame(renderer, map_tex, &app);
        SDL_Delay(8);
    }

    SDL_DestroyTexture(map_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
