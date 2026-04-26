/*
 * main.c — interactive entry point for the procedural city generator.
 *
 * Interface
 * ---------
 * The program displays an interactive terminal menu.  The user can cycle
 * through city type, rivers, width, and density settings, enter a custom
 * seed, regenerate the city, view it in the terminal, and save it as a
 * PPM image — all without restarting the program.
 *
 * Raw-mode terminal input (POSIX termios) is used so a single keypress
 * triggers an action without needing to press Enter.
 */
#define _POSIX_C_SOURCE 200809L
#include "city.h"
#include <time.h>
#include <termios.h>
#include <unistd.h>

/* ── Terminal raw mode ──────────────────────────────────────────────────── */

static struct termios g_old_termios;

static void raw_mode_on(void)
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &g_old_termios);
    t = g_old_termios;
    t.c_lflag &= ~((tcflag_t)(ICANON | ECHO));
    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

static void raw_mode_off(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
}

static int read_key(void)
{
    raw_mode_on();
    int ch = getchar();
    raw_mode_off();
    return ch;
}

/* ── Screen utilities ───────────────────────────────────────────────────── */

static void clear_screen(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

/* ── Statistics ─────────────────────────────────────────────────────────── */

static void print_stats(const Map *map)
{
    int counts[8] = {0};
    int total     = map->width * map->height;

    for (int y = 0; y < map->height; y++)
        for (int x = 0; x < map->width; x++)
            counts[(int)map->grid[y][x].type]++;

    printf("  Cellules totales  : %d\n", total);
    printf("  Eau               : %d  (%.1f %%)\n",
           counts[CELL_WATER],
           100.0f * (float)counts[CELL_WATER]    / (float)total);
    printf("  Routes            : %d  (%.1f %%)\n",
           counts[CELL_ROAD],
           100.0f * (float)counts[CELL_ROAD]     / (float)total);
    printf("  Ponts             : %d\n",  counts[CELL_BRIDGE]);
    printf("  Bâtiments         : %d  (%.1f %%)\n",
           counts[CELL_BUILDING],
           100.0f * (float)counts[CELL_BUILDING] / (float)total);
    printf("  Parcs             : %d\n",  counts[CELL_PARK]);
    printf("  Places            : %d\n",  counts[CELL_PLAZA]);
    printf("  Remparts          : %d\n",  counts[CELL_WALL]);
}

/* ── Generation ─────────────────────────────────────────────────────────── */

static void run_generation(Map *map, const CityParams *params)
{
    clear_screen();

    const char *type_str = (params->city_type == CITY_MEDIEVAL)
                           ? "Médiéval" : "Moderne";

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Génération de ville — %-17s║\n", type_str);
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  Carte   : %d × %d cellules            ║\n",
           MAP_WIDTH, MAP_HEIGHT);
    printf("║  Graine  : %-10u                   ║\n", params->seed);
    printf("╚══════════════════════════════════════════╝\n\n");

    map_init(map, params);

    printf("[1/6] Cours d'eau …\n");    generate_waterways(map, params);
    printf("[2/6] Quartiers …\n");      generate_districts(map);
    printf("[3/6] Routes …\n");         generate_roads(map);
    printf("[4/6] Ponts …\n");          generate_bridges(map);
    printf("[5/6] Bâtiments …\n");      generate_buildings(map, params);
    printf("[6/6] Parcs et places …\n");generate_parks(map);

    printf("\nGénération terminée.\n\n");
    print_stats(map);

    printf("\nAppuyez sur une touche pour revenir au menu…");
    fflush(stdout);
    read_key();
}

/* ── Interactive menu ───────────────────────────────────────────────────── */

static const char *city_type_name(CityType t)
{
    return (t == CITY_MEDIEVAL) ? "Médiéval" : "Moderne";
}

static const char *river_count_name(int n)
{
    switch (n) {
        case 0:  return "Auto (1–2)";
        case 1:  return "1";
        case 2:  return "2";
        default: return "3";
    }
}

static const char *river_width_name(int w)
{
    switch (w) {
        case 1:  return "Étroite";
        case 2:  return "Normale";
        default: return "Large";
    }
}

static const char *density_name(int d)
{
    switch (d) {
        case 0:  return "Éparse";
        case 1:  return "Normale";
        default: return "Dense";
    }
}

static void print_menu(const CityParams *params, int has_city)
{
    clear_screen();
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║     Générateur de Villes Procédural  v2.0       ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  Type de ville       : %-25s║\n", city_type_name(params->city_type));
    printf("║  Graine (seed)       : %-25u║\n", params->seed);
    printf("║  Rivières            : %-25s║\n", river_count_name(params->num_rivers));
    printf("║  Largeur des rivières: %-25s║\n", river_width_name(params->river_width));
    printf("║  Densité bâtiments   : %-25s║\n", density_name(params->density));
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  [t]  Changer le type de ville                  ║\n");
    printf("║  [r]  Graine aléatoire                          ║\n");
    printf("║  [s]  Saisir une graine                         ║\n");
    printf("║  [n]  Nombre de rivières                        ║\n");
    printf("║  [l]  Largeur des rivières                      ║\n");
    printf("║  [d]  Densité des bâtiments                     ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  [g]  Générer / Regénérer                       ║\n");
    if (has_city) {
        printf("║  [v]  Afficher dans le terminal                 ║\n");
        printf("║  [p]  Sauvegarder en PPM                        ║\n");
    } else {
        printf("║  [v]  (générez d'abord une ville)               ║\n");
        printf("║  [p]  (générez d'abord une ville)               ║\n");
    }
    printf("║  [q]  Quitter                                   ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");
    printf("\nTouche: ");
    fflush(stdout);
}

/* ── Main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    CityParams params;
    params.city_type   = CITY_MEDIEVAL;
    params.seed        = (unsigned int)time(NULL);
    params.num_rivers  = 0;
    params.river_width = 2;
    params.density     = 1;

    /* Allocate the map on the heap (≈ 150 KB) */
    Map *map = (Map *)malloc(sizeof(Map));
    if (!map) {
        fprintf(stderr, "Erreur : allocation mémoire impossible.\n");
        return 1;
    }

    int has_city = 0;

    for (;;) {
        print_menu(&params, has_city);

        int ch = read_key();

        switch (ch) {
            /* ── Change city type ──────────────────────────────────── */
            case 't': case 'T':
                params.city_type = (params.city_type == CITY_MEDIEVAL)
                                   ? CITY_MODERN : CITY_MEDIEVAL;
                break;

            /* ── Random seed ───────────────────────────────────────── */
            case 'r': case 'R':
                params.seed = (unsigned int)time(NULL) ^ ((unsigned int)rand());
                break;

            /* ── Enter a seed ──────────────────────────────────────── */
            case 's': case 'S': {
                clear_screen();
                printf("Entrez une graine (entier positif, 0 = aléatoire): ");
                fflush(stdout);
                unsigned int entered = 0;
                if (scanf("%u", &entered) == 1 && entered != 0)
                    params.seed = entered;
                else if (entered == 0)
                    params.seed = (unsigned int)time(NULL);
                /* Flush remaining input */
                int c2;
                while ((c2 = getchar()) != '\n' && c2 != EOF) { /* discard */ }
                break;
            }

            /* ── Number of rivers ──────────────────────────────────── */
            case 'n': case 'N':
                params.num_rivers = (params.num_rivers + 1) % 4; /* 0–3 */
                break;

            /* ── River width ───────────────────────────────────────── */
            case 'l': case 'L':
                params.river_width = (params.river_width % 3) + 1; /* 1–3 */
                break;

            /* ── Building density ──────────────────────────────────── */
            case 'd': case 'D':
                params.density = (params.density + 1) % 3; /* 0–2 */
                break;

            /* ── Generate ──────────────────────────────────────────── */
            case 'g': case 'G':
                run_generation(map, &params);
                has_city = 1;
                break;

            /* ── View in terminal ──────────────────────────────────── */
            case 'v': case 'V':
                if (has_city) {
                    clear_screen();
                    render_terminal(map);
                    printf("\nAppuyez sur une touche pour revenir au menu…");
                    fflush(stdout);
                    read_key();
                }
                break;

            /* ── Save PPM ──────────────────────────────────────────── */
            case 'p': case 'P':
                if (has_city) {
                    char filename[64];
                    snprintf(filename, sizeof(filename),
                             "city_%u.ppm", params.seed);
                    clear_screen();
                    render_ppm(map, filename);
                    printf("\nAppuyez sur une touche pour revenir au menu…");
                    fflush(stdout);
                    read_key();
                }
                break;

            /* ── Quit ──────────────────────────────────────────────── */
            case 'q': case 'Q': case 3: /* Ctrl-C */
                free(map);
                printf("\nAu revoir !\n");
                return 0;

            default:
                break;
        }
    }
}
