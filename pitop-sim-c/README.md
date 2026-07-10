# pitop-sim — Simulation C de PiTOP (Pi Takes Over Pokemon)

Simulateur offline en C du projet [PiTOP](..). Remplace le pipeline Java/BizHawk/Lua par un moteur de simulation capable d'exécuter des millions d'actions de Pi par seconde.

## Architecture

```
                    ┌─────────────┐
                    │   main.c    │  ← Point d'entrée (CLI)
                    └──────┬──────┘
                           │
              ┌────────────▼────────────┐
              │     simulator.c         │  ← Boucle principale
              │  Pi → action → transduc │
              └────────────┬────────────┘
                           │
              ┌────────────▼────────────┐
              │    transducer.c/.h      │  ← Moteur de jeu
              │  transition(state,act)  │
              └────┬──────┬──────┬──────┘
                   │      │      │
         ┌─────────▼┐ ┌───▼──┐ ┌▼─────────┐
         │movement  │ │warp  │ │interact  │
         │collisions│ │conn. │ │A/B/START │
         └─────────┬┘ └──────┘ └┬─────────┘
                   │            │
         ┌─────────▼┐    ┌──────▼──────┐
         │metatile  │    │ battle.c    │
         │behavior  │    │ RNG combat  │
         └─────────┬┘    └─────────────┘
                   │
         ┌─────────▼────────┐
         │   map_data.c     │  ← Cache des cartes dumpées
         │   map_data_parser│
         └─────────┬────────┘
                   │
         ┌─────────▼────────┐
         │ data/maps/*.txt  │  ← Fichiers de dump
         └──────────────────┘

Pi Generator:
    pi_generator.c/.h  ← Lit les décimales depuis un fichier
                          pré-calculé (ou GMP)

Logger:
    logger.c/.h        ← Log texte + trace binaire + heatmap
```

## Fichiers et fonctions

### `include/game_state.h` — Types fondamentaux du modèle de jeu

| Type / Fonction | Rôle |
|---|---|
| `Direction` (enum) | UP, DOWN, LEFT, RIGHT |
| `Action` (enum) | UP, DOWN, LEFT, RIGHT, A, B, START |
| `GameMode` (enum) | OVERWORLD, DIALOG, MENU, BATTLE, PARTY, BAG, POKENAV |
| `Position` (struct) | map_group, map_num, x, y, direction |
| `GameState` (struct) | État complet : position, RNG, event_flags, mode, compteurs |
| `lcg_step()` | Avance le RNG (linear congruential generator) : `s[n+1] = s[n] * 0x41C64E6D + 0x3039` |
| `action_name()` | Retourne le nom lisible d'une action |
| `direction_name()` | Retourne le nom lisible d'une direction |

### `include/map_data.h` — Stockage et chargement des cartes

| Type / Fonction | Rôle |
|---|---|
| `MapConnection` (struct) | Connexion vers une carte voisine (direction, destination, offset) |
| `WarpData` (struct) | Point de téléportation (position déclencheur → destination) |
| `TriggerData` (struct) | Événement au sol (script, signe) avec script_id |
| `MapData` (struct) | Carte complète : dimensions, tilesets, grille métatiles, connexions, warps, triggers |
| `map_init_index()` | Charge l'index CSV des cartes (table de hachage group/num → fichier) |
| `map_get()` | Retourne la carte (group, num), la charge depuis le disque si nécessaire |
| `map_get_metatile()` | Récupère l'ID du métatile à (x, y) sur une carte |
| `map_in_bounds()` | Vérifie que (x, y) est dans les limites de la carte |
| `map_free_all()` | Libère toutes les cartes chargées en mémoire |

### `include/metatile.h` — Comportements des tuiles

| Type / Fonction | Rôle |
|---|---|
| `MetatileBehavior` (enum) | NORMAL, TALL_GRASS, WALL, WATER, DOOR, WARP, LEDGE, etc. |
| `Metatile` (struct) | ID + comportement + propriétés dérivées (walkable, surfable, triggers_battle, is_door, is_warp, is_ledge) |
| `metatile_load_primary()` | Charge la table du tileset primaire (IDs 0-511) depuis un fichier texte |
| `metatile_load_secondary()` | Charge la table d'un tileset secondaire (IDs 512+) |
| `metatile_get()` | Retourne le Metatile complet pour un tileset et un ID donnés |
| `metatile_is_walkable()` | Raccourci : la tuile est-elle praticable à pied ? |
| `metatile_triggers_battle()` | Raccourci : la tuile déclenche-t-elle des combats ? |
| `metatile_free()` | Libère toutes les tables de métatiles |

### `include/pi_generator.h` — Générateur de Pi

| Type / Fonction | Rôle |
|---|---|
| `PiGenerator` (struct opaque) | Générateur de décimales de Pi |
| `pi_generator_new()` | Ouvre un fichier de décimales pré-calculées, ou NULL si indisponible |
| `pi_generator_next()` | Retourne la décimale suivante (0-9). Boucle sur le fichier si arrivé à la fin |
| `pi_generator_reset()` | Réinitialise au début |
| `pi_generator_free()` | Libère le générateur |
| `pi_digit_to_action()` | Mapping Java→C : chiffre 0-9 → action GBA (identique à Main.java) |

### `include/transducer.h` — Moteur de transition

| Fonction | Rôle |
|---|---|
| `transducer_apply()` | Point d'entrée : applique une action à l'état courant et retourne le nouvel état |
| `transducer_handle_movement()` | UP/DOWN/LEFT/RIGHT : change la direction, déplace si collision libre, gère les connexions entre cartes |
| `transducer_check_warp()` | Après un déplacement, vérifie si la case courante est un warp et téléporte |
| `transducer_handle_a()` | Bouton A : interagit (porte, NPC, dialogue, confirmation) |
| `transducer_handle_b()` | Bouton B : running shoes (overworld) / annulation (menu) |
| `transducer_handle_start()` | Bouton START : ouvre/ferme le menu |
| `transducer_check_battle()` | Après un pas sur de l'herbe, test RNG pour combat aléatoire |

### `include/simulator.h` — Orchestrateur

| Type / Fonction | Rôle |
|---|---|
| `SimConfig` (struct) | Configuration : fichier Pi, index cartes, log, nombre d'actions, intervalle de log, état initial |
| `SIM_CONFIG_DEFAULT` (macro) | Valeurs par défaut (10M actions, état initial à Bourg-en-Vol) |
| `simulator_run()` | Boucle principale : initialise tout, itère sur les actions, log, finalise |

### `include/logger.h` — Journalisation

| Type / Fonction | Rôle |
|---|---|
| `Logger` (struct) | Fichiers de log + trace binaire + heatmap + compteurs |
| `logger_init()` | Ouvre les fichiers et alloue la heatmap |
| `logger_log_frame()` | Écrit une ligne texte + 10 octets binaires pour chaque action |
| `logger_record_position()` | Incrémente le compteur de visites pour la position courante |
| `logger_printf()` | Écrit un message arbitraire dans le log |
| `logger_close()` | Finalise la heatmap, ferme les fichiers |
| `logger_print_summary()` | Affiche les stats récapitulatives sur stdout |

### `src/main.c` — Point d'entrée

Parse les arguments CLI (`-n`, `-p`, `-m`, `-l`, `-h`) et lance `simulator_run()`.

### `src/pi_generator.c` — Implémentation du générateur de Pi

Lit les décimales depuis un fichier binaire pré-calculé (format : entête 4 octets = nombre de décimales, puis les chiffres en caractères ASCII). Boucle automatiquement quand le fichier est épuisé.

### `src/map_data.c` — Cache des cartes

Maintient une liste chaînée des cartes indexées. `map_get()` charge le fichier au premier accès via `map_data_parse()`.

### `src/map_data_parser.c` — Parseur de fichiers de dump

Parse le format texte décrit dans `PLAN_SIMULATION_C.md` :
1. Dimensions + tilesets
2. Connexions
3. Warps
4. Triggers
5. Grille de métatiles

### `src/metatile.c` — Table des comportements

Charge les fichiers de table de métatiles (format : `ID | Behavior | Layer`), les pars, et construit pour chaque ID une structure `Metatile` avec les propriétés dérivées (walkable, triggers_battle, etc.).

### `src/transducer_movement.c` — Moteur de mouvement

Pour chaque action directionnelle :
1. Met à jour la direction (toujours)
2. Calcule les coordonnées cibles
3. Si hors limites → cherche une connexion vers la carte voisine
4. Si dans les limites → vérifie la collision via `metatile_is_walkable()`
5. Si libre → déplace le joueur et incrémente `total_steps`

### `src/transducer_warp.c` — Warps

Après chaque déplacement, parcourt la liste des warps de la carte courante. Si la position du joueur correspond à un warp, téléporte vers la destination.

### `src/transducer_interact.c` — Interactions A/B/START

- **A** : en overworld, examine la case devant le joueur (porte → warp, NPC → dialogue). En dialogue/battle, avance le compteur (3 pressions pour sortir).
- **B** : overworld → toggle running shoes. Dialogue/menu → retour overworld. Battle → tentative de fuite (60%).
- **START** : overworld → menu. Menu → overworld. Dialogue → ferme.

### `src/transducer_battle.c` — Combats aléatoires

Après un pas sur de l'herbe haute, utilise le RNG pour déterminer (~10% de chance) si un combat sauvage a lieu. Résolution simplifiée : 60% fuite, 30% victoire, 10% défaite (respawn à Bourg-en-Vol).

### `src/logger.c` — Système de logging

Produit trois fichiers :
- `simulation.log` : log textuel avec timeline complète
- `simulation.trace` : trace binaire 10 octets/frame (compatible `BinaryTraceStream.java`)
- `simulation.heatmap` : compteurs de visites par (map, x, y)

### `dump_scripts/dump_all_maps.lua` — Script Lua de dump

Script BizHawk à exécuter une fois pour dumper toutes les cartes du jeu. Itère sur les map_group/map_num, lit les entêtes de carte dans la ROM, et exporte les données (métatiles, warps, connexions, triggers) au format texte attendu par `map_data_parser.c`.

## Dépendances

- **gcc** (ou clang) avec support C99
- **make**
- **GNU MP** (optionnel, pour le mode GMP du générateur Pi)
- **BizHawk** (uniquement pour le dump initial des cartes via Lua)
- **Java + Maven** (uniquement pour pré-calculer les décimales de Pi)

## Build

```bash
make          # Compile
make run      # Lance avec les paramètres par défaut
make run ARGS='-n 1000000 -l test_run.log'   # Paramètres personnalisés
make clean    # Nettoie
```

## Génération du fichier Pi

```bash
# Depuis le répertoire pitop-sim-c/ :
make pi_digits.bin
```

Ou manuellement via le projet Java :
```bash
cd .. && mvn -q exec:java -Dexec.mainClass="pokemonpi.tools.GenPiFile" \
  -Dexec.args="10000000" > pitop-sim-c/pi_digits.bin
```

## Dump des cartes

1. Lancer Pokemon Saphir (USA/Europe v1.1 / Rev 1) dans BizHawk
2. Exécuter `dump_scripts/dump_all_maps.lua` depuis l'interface Lua de BizHawk
3. Les fichiers générés dans `data/maps/` sont automatiquement chargés par le simulateur
4. Générer l'index : `ls data/maps/*.txt | sed 's/.*\/\([0-9]*\)_\([0-9]*\)\.txt/\1,\2,\1_\2.txt/' > data/index.csv`
