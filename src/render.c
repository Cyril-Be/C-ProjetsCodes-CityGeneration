/*
 * render.c — two rendering backends.
 *
 * render_terminal()
 *   Prints the map to stdout using ANSI 256-colour escape codes.
 *   One terminal character per map cell.  The colour palette switches
 *   between a warm medieval look and a cool modern look depending on the
 *   city type stored in the map.
 *
 * render_ppm()
 *   Writes a binary PPM (P6) image.  Each map cell is rendered as an
 *   8 × 8 pixel block.  Buildings receive a 1-pixel darker border, and
 *   roads receive a lighter 1-pixel curb line, giving the image a
 *   detailed map-like quality.  The colour palette also reflects the city
 *   type (earthy medieval vs. concrete modern).
 */
#include "city.h"

/* ── ANSI helpers ────────────────────────────────────────────────────────── */
#define ANSI_RESET    "\033[0m"
#define FG_WHITE_BOLD "\033[1;97m"

/* ── Shared cell-type backgrounds ─────────────────────────────────────── */
#define BG_WATER      "\033[48;5;26m"    /* ocean blue            */
#define BG_BRIDGE     "\033[48;5;136m"   /* golden yellow         */
#define BG_PARK       "\033[48;5;34m"    /* green                 */
#define BG_PLAZA      "\033[48;5;180m"   /* sandy stone           */
#define BG_WALL       "\033[48;5;238m"   /* dark stone grey       */

/* ── Medieval palette ─────────────────────────────────────────────────── */
#define MED_BG_EMPTY  "\033[48;5;137m"   /* warm sandy ground     */
#define MED_BG_ROAD   "\033[48;5;94m"    /* dirt brown            */
#define MED_BG_CENTER "\033[48;5;88m"    /* dark red              */
#define MED_BG_COMM   "\033[48;5;130m"   /* terracotta            */
#define MED_BG_RES    "\033[48;5;67m"    /* slate blue            */
#define MED_BG_IND    "\033[48;5;238m"   /* dark grey             */

/* ── Modern palette ───────────────────────────────────────────────────── */
#define MOD_BG_EMPTY  "\033[48;5;250m"   /* light grey ground     */
#define MOD_BG_ROAD   "\033[48;5;240m"   /* asphalt               */
#define MOD_BG_CENTER "\033[48;5;24m"    /* dark blue (glass)     */
#define MOD_BG_COMM   "\033[48;5;60m"    /* purple-blue           */
#define MOD_BG_RES    "\033[48;5;173m"   /* warm peach            */
#define MOD_BG_IND    "\033[48;5;237m"   /* industrial grey       */

/* ── Terminal renderer ────────────────────────────────────────────────── */

static const char *building_bg(DistrictType d, CityType type)
{
    if (type == CITY_MEDIEVAL) {
        switch (d) {
            case DISTRICT_CENTER:      return MED_BG_CENTER;
            case DISTRICT_COMMERCIAL:  return MED_BG_COMM;
            case DISTRICT_RESIDENTIAL: return MED_BG_RES;
            case DISTRICT_INDUSTRIAL:  return MED_BG_IND;
            default:                   return MED_BG_EMPTY;
        }
    } else {
        switch (d) {
            case DISTRICT_CENTER:      return MOD_BG_CENTER;
            case DISTRICT_COMMERCIAL:  return MOD_BG_COMM;
            case DISTRICT_RESIDENTIAL: return MOD_BG_RES;
            case DISTRICT_INDUSTRIAL:  return MOD_BG_IND;
            default:                   return MOD_BG_EMPTY;
        }
    }
}

static char building_char(int height)
{
    if (height >= 9) return '#';
    if (height >= 6) return 'H';
    if (height >= 3) return 'h';
    return 'n';
}

void render_terminal(const Map *map)
{
    int medieval = (map->city_type == CITY_MEDIEVAL);

    const char *bg_road  = medieval ? MED_BG_ROAD  : MOD_BG_ROAD;
    const char *bg_empty = medieval ? MED_BG_EMPTY : MOD_BG_EMPTY;

    /* Legend */
    printf("\n");
    if (medieval)
        printf(" \033[1mMédiéval\033[0m");
    else
        printf(" \033[1mModerne\033[0m ");
    printf("  ");
    printf(BG_WATER   FG_WHITE_BOLD " ~ " ANSI_RESET " Eau     ");
    printf("%s" FG_WHITE_BOLD " . " ANSI_RESET " Route   ", bg_road);
    printf(BG_BRIDGE  FG_WHITE_BOLD " = " ANSI_RESET " Pont    ");
    printf(BG_PARK    FG_WHITE_BOLD " * " ANSI_RESET " Parc\n");
    printf("               ");
    printf(BG_PLAZA   FG_WHITE_BOLD " o " ANSI_RESET " Place   ");
    printf(BG_WALL    FG_WHITE_BOLD " # " ANSI_RESET " Rempart ");
    if (medieval) {
        printf(MED_BG_CENTER FG_WHITE_BOLD " C " ANSI_RESET " Centre  ");
        printf(MED_BG_RES    FG_WHITE_BOLD " r " ANSI_RESET " Résidentiel\n\n");
    } else {
        printf(MOD_BG_CENTER FG_WHITE_BOLD " C " ANSI_RESET " Centre  ");
        printf(MOD_BG_RES    FG_WHITE_BOLD " r " ANSI_RESET " Résidentiel\n\n");
    }

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            const Cell *cell = &map->grid[y][x];
            switch (cell->type) {
                case CELL_WATER:
                    printf(BG_WATER   FG_WHITE_BOLD "~" ANSI_RESET); break;
                case CELL_ROAD:
                    printf("%s" FG_WHITE_BOLD "." ANSI_RESET, bg_road); break;
                case CELL_BRIDGE:
                    printf(BG_BRIDGE  FG_WHITE_BOLD "=" ANSI_RESET); break;
                case CELL_PARK:
                    printf(BG_PARK    FG_WHITE_BOLD "*" ANSI_RESET); break;
                case CELL_PLAZA:
                    printf(BG_PLAZA   FG_WHITE_BOLD "o" ANSI_RESET); break;
                case CELL_WALL:
                    printf(BG_WALL    FG_WHITE_BOLD "#" ANSI_RESET); break;
                case CELL_BUILDING:
                    printf("%s" FG_WHITE_BOLD "%c" ANSI_RESET,
                           building_bg(cell->district, map->city_type),
                           building_char(cell->height));
                    break;
                default:
                    printf("%s " ANSI_RESET, bg_empty); break;
            }
        }
        printf("\n");
    }
}

/* ── PPM image renderer ───────────────────────────────────────────────── */

#define PPM_SCALE 8  /* each map cell → 8 × 8 pixels */

typedef struct { unsigned char r, g, b; } RGB;

static RGB cell_colour(const Cell *cell, CityType type)
{
    RGB c;

    switch (cell->type) {
        case CELL_WATER:
            if (type == CITY_MEDIEVAL) {
                c.r =  30; c.g = 100; c.b = 195;
            } else {
                c.r =  40; c.g = 115; c.b = 205;
            }
            return c;

        case CELL_ROAD:
            if (type == CITY_MEDIEVAL) {
                c.r = 115; c.g =  88; c.b =  60;  /* dirt brown */
            } else {
                c.r =  75; c.g =  75; c.b =  80;  /* asphalt    */
            }
            return c;

        case CELL_BRIDGE:
            if (type == CITY_MEDIEVAL) {
                c.r = 160; c.g = 125; c.b =  70;  /* wooden plank */
            } else {
                c.r = 190; c.g = 175; c.b = 155;  /* concrete     */
            }
            return c;

        case CELL_PARK:
            if (type == CITY_MEDIEVAL) {
                c.r =  50; c.g = 140; c.b =  50;
            } else {
                c.r =  65; c.g = 175; c.b =  65;
            }
            return c;

        case CELL_PLAZA:
            c.r = 200; c.g = 185; c.b = 150;        /* stone paving */
            return c;

        case CELL_WALL:
            c.r =  75; c.g =  65; c.b =  55;        /* dark stone   */
            return c;

        case CELL_BUILDING: {
            if (type == CITY_MEDIEVAL) {
                switch (cell->district) {
                    case DISTRICT_CENTER:
                        c.r = 160; c.g =  50; c.b =  50; break;
                    case DISTRICT_COMMERCIAL:
                        c.r = 180; c.g = 110; c.b =  55; break;
                    case DISTRICT_RESIDENTIAL:
                        c.r =  90; c.g = 100; c.b = 150; break;
                    case DISTRICT_INDUSTRIAL:
                        c.r = 100; c.g =  85; c.b =  70; break;
                    default:
                        c.r = 140; c.g = 130; c.b = 120; break;
                }
            } else {
                switch (cell->district) {
                    case DISTRICT_CENTER:
                        c.r =  40; c.g =  80; c.b = 165; break;
                    case DISTRICT_COMMERCIAL:
                        c.r = 120; c.g = 140; c.b = 175; break;
                    case DISTRICT_RESIDENTIAL:
                        c.r = 200; c.g = 170; c.b = 130; break;
                    case DISTRICT_INDUSTRIAL:
                        c.r = 120; c.g = 110; c.b = 100; break;
                    default:
                        c.r = 150; c.g = 150; c.b = 150; break;
                }
            }
            /* Taller buildings are slightly darker (depth effect) */
            float shade = 0.55f + (float)cell->height * 0.045f;
            if (shade > 1.0f) shade = 1.0f;
            c.r = (unsigned char)((float)c.r * shade);
            c.g = (unsigned char)((float)c.g * shade);
            c.b = (unsigned char)((float)c.b * shade);
            return c;
        }

        default: /* CELL_EMPTY */
            if (type == CITY_MEDIEVAL) {
                c.r = 200; c.g = 185; c.b = 160;   /* warm earth */
            } else {
                c.r = 210; c.g = 210; c.b = 210;   /* cool grey  */
            }
            return c;
    }
}

void render_ppm(const Map *map, const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "render_ppm: cannot open '%s'\n", filename);
        return;
    }

    int pw = map->width  * PPM_SCALE;
    int ph = map->height * PPM_SCALE;

    fprintf(f, "P6\n%d %d\n255\n", pw, ph);

    for (int py = 0; py < ph; py++) {
        int cell_y       = py / PPM_SCALE;
        int py_in_cell   = py % PPM_SCALE;

        for (int px = 0; px < pw; px++) {
            int cell_x     = px / PPM_SCALE;
            int px_in_cell = px % PPM_SCALE;

            const Cell *cell = &map->grid[cell_y][cell_x];
            RGB col = cell_colour(cell, map->city_type);

            /* ── Per-pixel detail effects ──────────────────────────── */
            int on_edge = (px_in_cell == 0 || px_in_cell == PPM_SCALE - 1 ||
                           py_in_cell == 0 || py_in_cell == PPM_SCALE - 1);

            if (cell->type == CELL_BUILDING && on_edge) {
                /* Thin dark outline around each building */
                col.r = (unsigned char)(col.r * 55 / 100);
                col.g = (unsigned char)(col.g * 55 / 100);
                col.b = (unsigned char)(col.b * 55 / 100);
            } else if (cell->type == CELL_ROAD && on_edge) {
                /* Lighter curb line on road edges */
                col.r = (unsigned char)(col.r + (255 - col.r) / 3);
                col.g = (unsigned char)(col.g + (255 - col.g) / 3);
                col.b = (unsigned char)(col.b + (255 - col.b) / 3);
            } else if (cell->type == CELL_WATER) {
                /* Subtle gradient: lighter in centre of cell */
                int cx_off = px_in_cell - PPM_SCALE / 2;
                int cy_off = py_in_cell - PPM_SCALE / 2;
                float d = sqrtf((float)(cx_off*cx_off + cy_off*cy_off));
                float bright = 1.0f + (1.0f - d / (float)(PPM_SCALE / 2)) * 0.12f;
                if (bright > 1.0f) bright = 1.0f;
                col.r = (unsigned char)((float)col.r * bright);
                col.g = (unsigned char)((float)col.g * bright);
                col.b = (unsigned char)((float)col.b * bright < 255.0f
                                        ? (float)col.b * bright : 255.0f);
            }

            fwrite(&col.r, 1, 1, f);
            fwrite(&col.g, 1, 1, f);
            fwrite(&col.b, 1, 1, f);
        }
    }

    fclose(f);
    printf("Image sauvegardée → %s  (%d × %d px)\n", filename, pw, ph);
}
