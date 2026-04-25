/*
 * render.c — two rendering backends.
 *
 * render_terminal()
 *   Prints the map to stdout using ANSI 256-colour escape codes.
 *   One terminal character per map cell.  A legend is printed above the map.
 *
 * render_ppm()
 *   Writes a binary PPM (P6) image.  Each map cell is rendered as a
 *   PPM_SCALE × PPM_SCALE block of pixels.  Building height slightly
 *   darkens the base colour so taller buildings appear richer.
 */
#include "city.h"

/* ── ANSI colour helpers ─────────────────────────────────────────────── */
/*
 * All background codes use 256-colour sequences (\033[48;5;<n>m) so that
 * each district has a clearly distinct, true background colour and there is
 * no risk of accidentally using a foreground-only code as a background.
 */
#define ANSI_RESET        "\033[0m"
#define FG_WHITE_BOLD     "\033[1;97m"   /* bright white, bold            */

/* Cell-type backgrounds */
#define BG_WATER          "\033[48;5;33m"   /* medium blue                */
#define BG_ROAD           "\033[48;5;240m"  /* dark grey                  */
#define BG_BRIDGE         "\033[48;5;178m"  /* golden yellow              */
#define BG_PARK           "\033[48;5;34m"   /* green                      */

/* Building backgrounds (one per district) */
#define BG_CENTER         "\033[48;5;124m"  /* dark red                   */
#define BG_COMMERCIAL     "\033[48;5;166m"  /* orange                     */
#define BG_RESIDENTIAL    "\033[48;5;25m"   /* steel blue                 */
#define BG_INDUSTRIAL     "\033[48;5;236m"  /* near-black grey            */
#define BG_EMPTY          "\033[48;5;250m"  /* light grey (fallback)      */

/* Choose background escape for a building cell */
static const char *building_bg(DistrictType d)
{
    switch (d) {
        case DISTRICT_CENTER:      return BG_CENTER;
        case DISTRICT_COMMERCIAL:  return BG_COMMERCIAL;
        case DISTRICT_RESIDENTIAL: return BG_RESIDENTIAL;
        case DISTRICT_INDUSTRIAL:  return BG_INDUSTRIAL;
        default:                   return BG_EMPTY;
    }
}

/* Character representing a building based on height */
static char building_char(int height)
{
    if (height >= 9) return '#';
    if (height >= 6) return 'H';
    if (height >= 3) return 'h';
    return 'n';
}

/* ── Terminal renderer ────────────────────────────────────────────────── */

void render_terminal(const Map *map)
{
    /* Legend */
    printf("\n");
    printf(BG_WATER    FG_WHITE_BOLD " ~ " ANSI_RESET " Water       ");
    printf(BG_ROAD     FG_WHITE_BOLD " . " ANSI_RESET " Road        ");
    printf(BG_BRIDGE   FG_WHITE_BOLD " = " ANSI_RESET " Bridge      ");
    printf(BG_PARK     FG_WHITE_BOLD " * " ANSI_RESET " Park\n");

    printf(BG_CENTER      FG_WHITE_BOLD " # " ANSI_RESET " Center      ");
    printf(BG_COMMERCIAL  FG_WHITE_BOLD " H " ANSI_RESET " Commercial  ");
    printf(BG_RESIDENTIAL FG_WHITE_BOLD " h " ANSI_RESET " Residential ");
    printf(BG_INDUSTRIAL  FG_WHITE_BOLD " n " ANSI_RESET " Industrial\n\n");

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            const Cell *cell = &map->grid[y][x];
            switch (cell->type) {
                case CELL_WATER:
                    printf(BG_WATER  FG_WHITE_BOLD "~" ANSI_RESET);
                    break;
                case CELL_ROAD:
                    printf(BG_ROAD   FG_WHITE_BOLD "." ANSI_RESET);
                    break;
                case CELL_BRIDGE:
                    printf(BG_BRIDGE FG_WHITE_BOLD "=" ANSI_RESET);
                    break;
                case CELL_PARK:
                    printf(BG_PARK   FG_WHITE_BOLD "*" ANSI_RESET);
                    break;
                case CELL_BUILDING:
                    printf("%s" FG_WHITE_BOLD "%c" ANSI_RESET,
                           building_bg(cell->district),
                           building_char(cell->height));
                    break;
                default:
                    printf(BG_EMPTY  " " ANSI_RESET);
                    break;
            }
        }
        printf("\n");
    }
}

/* ── PPM image renderer ───────────────────────────────────────────────── */

#define PPM_SCALE 6  /* each cell → PPM_SCALE × PPM_SCALE pixels */

typedef struct { unsigned char r, g, b; } RGB;

static RGB cell_colour(const Cell *cell)
{
    RGB c = {220, 220, 220};   /* default: light grey (empty) */

    switch (cell->type) {
        case CELL_WATER:
            c.r = 30;  c.g = 110; c.b = 200;
            break;

        case CELL_ROAD:
            c.r = 110; c.g = 110; c.b = 110;
            break;

        case CELL_BRIDGE:
            c.r = 190; c.g = 160; c.b = 80;
            break;

        case CELL_PARK:
            c.r = 60;  c.g = 180; c.b = 60;
            break;

        case CELL_BUILDING: {
            /* Base colour by district */
            switch (cell->district) {
                case DISTRICT_CENTER:
                    c.r = 210; c.g = 50;  c.b = 50;  break;
                case DISTRICT_COMMERCIAL:
                    c.r = 210; c.g = 120; c.b = 50;  break;
                case DISTRICT_RESIDENTIAL:
                    c.r = 70;  c.g = 90;  c.b = 200; break;
                case DISTRICT_INDUSTRIAL:
                    c.r = 150; c.g = 100; c.b = 50;  break;
                default:
                    c.r = 150; c.g = 150; c.b = 150; break;
            }
            /* Taller buildings appear slightly darker */
            float shade = 0.55f + (float)cell->height * 0.045f;
            if (shade > 1.0f) shade = 1.0f;
            c.r = (unsigned char)((float)c.r * shade);
            c.g = (unsigned char)((float)c.g * shade);
            c.b = (unsigned char)((float)c.b * shade);
            break;
        }

        default:
            break;
    }
    return c;
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
        int cy = py / PPM_SCALE;
        for (int px = 0; px < pw; px++) {
            int   cx  = px / PPM_SCALE;
            RGB   col = cell_colour(&map->grid[cy][cx]);
            fwrite(&col.r, 1, 1, f);
            fwrite(&col.g, 1, 1, f);
            fwrite(&col.b, 1, 1, f);
        }
    }

    fclose(f);
    printf("Image saved → %s  (%d × %d px)\n", filename, pw, ph);
}
