/*
 * main.c — entry point for the procedural city generator.
 *
 * Usage
 * -----
 *   ./city_gen [seed [output.ppm]]
 *
 *   seed        Integer seed for reproducible generation (default: time-based)
 *   output.ppm  Path for the PPM image (default: city.ppm)
 *
 * The six generation steps run in order and status messages are printed so
 * the progress is visible even for large maps.
 *
 * A statistics summary is printed at the end, showing how many cells of
 * each type were generated.
 */
#include "city.h"
#include <time.h>

static void print_stats(const Map *map)
{
    int counts[6] = {0};
    int total     = map->width * map->height;

    for (int y = 0; y < map->height; y++)
        for (int x = 0; x < map->width; x++)
            counts[(int)map->grid[y][x].type]++;

    printf("\n── Statistics ──────────────────────────────────────\n");
    printf("  Total cells   : %d\n",  total);
    printf("  Water         : %d  (%.1f %%)\n",
           counts[CELL_WATER],    100.0f * counts[CELL_WATER]    / total);
    printf("  Roads         : %d  (%.1f %%)\n",
           counts[CELL_ROAD],     100.0f * counts[CELL_ROAD]     / total);
    printf("  Bridges       : %d  (%.1f %%)\n",
           counts[CELL_BRIDGE],   100.0f * counts[CELL_BRIDGE]   / total);
    printf("  Buildings     : %d  (%.1f %%)\n",
           counts[CELL_BUILDING], 100.0f * counts[CELL_BUILDING] / total);
    printf("  Parks         : %d  (%.1f %%)\n",
           counts[CELL_PARK],     100.0f * counts[CELL_PARK]     / total);
    printf("  Empty         : %d  (%.1f %%)\n",
           counts[CELL_EMPTY],    100.0f * counts[CELL_EMPTY]    / total);
    printf("────────────────────────────────────────────────────\n\n");
}

int main(int argc, char *argv[])
{
    unsigned int seed     = (unsigned int)time(NULL);
    const char  *ppm_file = "city.ppm";

    if (argc > 1) seed     = (unsigned int)atoi(argv[1]);
    if (argc > 2) ppm_file = argv[2];

    printf("╔══════════════════════════════════════╗\n");
    printf("║   Procedural City Generator  v1.0   ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║  Map   : %d × %d cells              ║\n",
           MAP_WIDTH, MAP_HEIGHT);
    printf("║  Seed  : %-10u                  ║\n", seed);
    printf("╚══════════════════════════════════════╝\n\n");

    Map map;
    map_init(&map, seed);

    printf("[1/6] Generating waterways …\n");
    generate_waterways(&map);

    printf("[2/6] Assigning districts …\n");
    generate_districts(&map);

    printf("[3/6] Laying roads …\n");
    generate_roads(&map);

    printf("[4/6] Building bridges …\n");
    generate_bridges(&map);

    printf("[5/6] Placing buildings …\n");
    generate_buildings(&map);

    printf("[6/6] Adding parks …\n");
    generate_parks(&map);

    printf("\nGeneration complete.\n");
    print_stats(&map);

    render_terminal(&map);
    render_ppm(&map, ppm_file);

    return 0;
}
