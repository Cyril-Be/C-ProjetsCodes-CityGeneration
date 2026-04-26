# C-ProjetsCodes-CityGeneration

Generateur procédural de plans de ville écrit en C — version 3.0 (interface SDL2).

Le programme produit à chaque fois une ville unique et réaliste dans une
fenêtre graphique native Ubuntu, avec génération visible en direct étape par
étape.

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

## Build (Ubuntu)

Nécessite `gcc`, `make`, `pkg-config` et `libsdl2-dev`.

```sh
sudo apt-get update
sudo apt-get install -y build-essential pkg-config libsdl2-dev
```

```sh
make
```

## Lancement (interface graphique)

```sh
./city_gen
```

| Touche | Action |
|---|---|
| `t` | Changer le type de ville (Médiéval ↔ Moderne) |
| `r` | Nouvelle graine aléatoire |
| `n` | Cycle du nombre de rivières (Auto / 1 / 2 / 3) |
| `l` | Cycle de la largeur des rivières (Étroite / Normale / Large) |
| `d` | Cycle de la densité des bâtiments (Éparse / Normale / Dense) |
| `g` / `Espace` | Générer / Regénérer |
| `p` | Sauvegarder en image PPM (`city_<seed>.ppm`, 1280 × 640 px) |
| `q` / `Échap` | Quitter |

---

## Sortie

- **Fenêtre SDL2** : rendu temps réel avec progression visible des 6 étapes
  (waterways, districts, roads, bridges, buildings, parks) et HUD intégré.
- **PPM** : image 1280 × 640 px (échelle 8×), avec contours de bâtiments,
  effet de bordure sur les routes et dégradé sur l'eau.
  Ouvrez `city_<seed>.ppm` dans n'importe quel visionneur d'images.

---

## Nettoyer

```sh
make clean
```
