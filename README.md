# C-ProjetsCodes-CityGeneration

Procedural city plan generator written in C.

The program generates a unique, realistic-looking city map every time it runs.
Six generation steps build up the city layer by layer:

1. **Waterways** — rivers are traced across the map with natural-looking meanders.
2. **Districts** — the city is divided into zones (centre, commercial, residential, industrial, park) based on distance from the randomly-placed city core.
3. **Roads** — radial arteries radiate from the city centre; an orthogonal street grid fills in the rest, denser in the centre and sparser on the outskirts.
4. **Bridges** — wherever a road hits a river bank, a bridge is built across the gap (up to 16 cells wide).
5. **Buildings** — every remaining land cell is filled with a building whose height reflects its district.
6. **Parks** — waterfront strips and random pocket parks are placed in residential areas.

The output is:
- a coloured ASCII map in the terminal (ANSI-256 colours),
- a `city.ppm` image (720 × 360 px, scale 6×) that can be opened in any image viewer.

---

## Build

Requires `gcc` and `make`.

```sh
make
```

## Usage

```sh
./city_gen [seed [output.ppm]]
```

| argument | description |
|---|---|
| `seed` | integer seed for reproducible generation (default: current time) |
| `output.ppm` | path for the image output (default: `city.ppm`) |

**Examples**

```sh
./city_gen            # random city
./city_gen 42         # reproducible city with seed 42
./city_gen 42 map.ppm # as above, save image to map.ppm
```

## Clean

```sh
make clean
```
