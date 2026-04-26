# C-ProjetsCodes-CityGeneration

Generateur procédural de plans de ville écrit en C — version 2.0.

Le programme produit à chaque fois une ville unique et réaliste grâce à une
interface interactive qui permet de choisir le type de ville, la graine, les
paramètres des cours d'eau et la densité des bâtiments.

---

## Types de villes

### Médiéval
- Rues organiques générées par un algorithme "waypoint + jitter" : les artères
  principales relient le centre aux portes de la ville en ondulant naturellement,
  un anneau de routes rejoint les portes entre elles et des rues secondaires
  se ramifient en marchés aléatoires.
- Le centre-ville est automatiquement biaisé vers le cours d'eau le plus proche
  (les villes médiévales naissaient aux gués et aux ponts).
- Les quartiers ont des frontières irrégulières grâce à un bruit spatial.
- Bâtiments bas (1–4 étages), places de marché (`CELL_PLAZA`), remparts éventuels.

### Moderne
- Routes en anneaux (boulevard intérieur, ring road, ceinture périphérique)
  superposées à une grille orthogonale dense au centre et éparse en banlieue.
- Artères radiales légèrement courbes et 2–3 boulevards diagonaux.
- Bâtiments hauts dans le centre (6–10 étages), couleurs palette verre/béton.

---

## Étapes de génération

| Étape | Module | Description |
|---|---|---|
| 1 | `waterways.c` | Cours d'eau méandres (6 points de contrôle, largeur paramétrable) |
| 2 | `districts.c` | Quartiers avec biais eau (médiéval) et bruit spatial |
| 3 | `roads.c` | Routes organiques (médiéval) ou structurées avec anneaux (moderne) |
| 4 | `bridges.c` | Ponts automatiques sur les traversées de rivières |
| 5 | `buildings.c` | Bâtiments (médiéval : longe les rues ; moderne : remplit les îlots) |
| 6 | `parks.c` | Parcs riverains, squares de quartier, places de marché |

---

## Build

Nécessite `gcc` et `make`.

```sh
make
```

## Lancement (interface interactive)

```sh
./city_gen
```

| Touche | Action |
|---|---|
| `t` | Changer le type de ville (Médiéval ↔ Moderne) |
| `r` | Nouvelle graine aléatoire |
| `s` | Saisir une graine précise |
| `n` | Cycle du nombre de rivières (Auto / 1 / 2 / 3) |
| `l` | Cycle de la largeur des rivières (Étroite / Normale / Large) |
| `d` | Cycle de la densité des bâtiments (Éparse / Normale / Dense) |
| `g` | Générer / Regénérer |
| `v` | Afficher dans le terminal (ANSI 256 couleurs) |
| `p` | Sauvegarder en image PPM (`city_<seed>.ppm`, 1280 × 640 px) |
| `q` | Quitter |

---

## Sortie

- **Terminal** : carte colorée ANSI-256 (160 × 80 caractères).
- **PPM** : image 1280 × 640 px (échelle 8×), avec contours de bâtiments,
  effet de bordure sur les routes et dégradé sur l'eau.
  Ouvrez `city_<seed>.ppm` dans n'importe quel visionneur d'images.

---

## Nettoyer

```sh
make clean
```
