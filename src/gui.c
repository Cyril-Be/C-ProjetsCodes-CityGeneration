#include "city.h"
#include <SDL2/SDL.h>
#include <ctype.h>
#include <time.h>

#define CELL_SCALE    8
#define HUD_HEIGHT  140
#define WINDOW_W   (MAP_WIDTH * CELL_SCALE)
#define WINDOW_H   (MAP_HEIGHT * CELL_SCALE + HUD_HEIGHT)
#define STEP_DELAY_MS 140

typedef struct {
    CityParams params;
    Map        map;
    int        has_city;
    int        generating;
    int        step_index;      /* 0..6 */
    Uint32     last_step_ticks;
    char       last_export[64];
} AppState;

typedef struct { unsigned char r, g, b; } RGB;

typedef struct {
    char c;
    unsigned char rows[7];
} Glyph;

static const Glyph GLYPHS[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
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

static void render_frame(SDL_Renderer *renderer, const AppState *app)
{
    SDL_SetRenderDrawColor(renderer, 20, 22, 26, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < app->map.height; y++) {
        for (int x = 0; x < app->map.width; x++) {
            const Cell *cell = &app->map.grid[y][x];
            RGB c = cell_colour(cell, app->map.city_type);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_Rect r = {x * CELL_SCALE, y * CELL_SCALE, CELL_SCALE, CELL_SCALE};
            SDL_RenderFillRect(renderer, &r);
        }
    }

    SDL_Rect hud = {0, MAP_HEIGHT * CELL_SCALE, WINDOW_W, HUD_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 26, 30, 36, 255);
    SDL_RenderFillRect(renderer, &hud);

    SDL_Color white = {235, 238, 242, 255};
    SDL_Color info  = {140, 195, 255, 255};
    SDL_Color ok    = {120, 230, 130, 255};
    SDL_Color warn  = {245, 190, 95, 255};

    char line[256];
    snprintf(line, sizeof(line), "TYPE[T]: %s    SEED[R]: %u",
             app->params.city_type == CITY_MEDIEVAL ? "MEDIEVAL" : "MODERN",
             app->params.seed);
    draw_text(renderer, 10, MAP_HEIGHT * CELL_SCALE + 10, 2, white, line);

    snprintf(line, sizeof(line), "RIVERS[N]: %s   WIDTH[L]: %s   DENSITY[D]: %s",
             river_count_label(app->params.num_rivers),
             river_width_label(app->params.river_width),
             density_label(app->params.density));
    draw_text(renderer, 10, MAP_HEIGHT * CELL_SCALE + 30, 2, white, line);

    if (app->generating) {
        snprintf(line, sizeof(line), "STATUS: GENERATING  STEP: %d/6  %s",
                 app->step_index, step_name(app->step_index));
        draw_text(renderer, 10, MAP_HEIGHT * CELL_SCALE + 50, 2, warn, line);
    } else if (app->has_city) {
        snprintf(line, sizeof(line), "STATUS: DONE  STEP: 6/6");
        draw_text(renderer, 10, MAP_HEIGHT * CELL_SCALE + 50, 2, ok, line);
    } else {
        draw_text(renderer, 10, MAP_HEIGHT * CELL_SCALE + 50, 2, info, "STATUS: IDLE");
    }

    int completed = app->generating ? app->step_index : (app->has_city ? 6 : 0);
    SDL_Rect progress_bg = {10, MAP_HEIGHT * CELL_SCALE + 72, WINDOW_W - 20, 16};
    SDL_SetRenderDrawColor(renderer, 50, 58, 70, 255);
    SDL_RenderFillRect(renderer, &progress_bg);
    SDL_Rect progress_fg = progress_bg;
    progress_fg.w = (progress_bg.w * completed) / 6;
    SDL_SetRenderDrawColor(renderer, 87, 180, 255, 255);
    SDL_RenderFillRect(renderer, &progress_fg);

    draw_text(renderer, 10, MAP_HEIGHT * CELL_SCALE + 96, 2, white,
              "CONTROLS: G GENERATE  P EXPORT PPM  Q OR ESC QUIT");

    if (app->last_export[0] != '\0') {
        snprintf(line, sizeof(line), "LAST EXPORT: %s", app->last_export);
        draw_text(renderer, 10, MAP_HEIGHT * CELL_SCALE + 114, 2, info, line);
    }

    SDL_RenderPresent(renderer);
}

static void update_window_title(SDL_Window *window, const AppState *app)
{
    char title[256];
    snprintf(title, sizeof(title),
             "City Generator SDL | Type=%s Seed=%u Rivers=%s Width=%s Density=%s | G generate, T/N/L/D set, R seed, P ppm, Q quit",
             app->params.city_type == CITY_MEDIEVAL ? "Medieval" : "Modern",
             app->params.seed,
             river_count_label(app->params.num_rivers),
             river_width_label(app->params.river_width),
             density_label(app->params.density));
    SDL_SetWindowTitle(window, title);
}

int run_gui_app(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "City Generator SDL",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL window error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "SDL renderer error: %s\n", SDL_GetError());
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
    map_init(&app.map, &app.params);
    start_generation(&app);

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode key = event.key.keysym.sym;
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
                    default:
                        break;
                }
            }
        }

        update_generation(&app, SDL_GetTicks());
        update_window_title(window, &app);
        render_frame(renderer, &app);
        SDL_Delay(8);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
