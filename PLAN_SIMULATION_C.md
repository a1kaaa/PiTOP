# Plan de Simulation C pour PiTOP

## Résumé

Remplacer le pipeline actuel (Java + BizHawk + Lua) par un simulateur en C qui :

1. **Dump** toutes les cartes de Pokemon Saphir depuis l'émulateur
2. **Charge** ces données dans un simulateur offline
3. **Exécute** les actions générées par les décimales de Pi sur ce modèle
4. **Observe** la progression sans avoir à faire tourner l'émulateur

---

## 1. Pourquoi un simulateur C ?

### Limites du système actuel
- **Dépendance à BizHawk** : nécessite un PC capable de faire tourner l'émulateur 24/7
- **Vitesse réelle** : 60 fps max, 12 frames par action → 5 actions/s maximum
- **Impossible à paralléliser** : un seul run à la fois
- **Pas de rejouabilité** : on ne peut pas "rembobiner" ou tester des variantes

### Avantages du simulateur C
- **Vitesse ×1000** : des millions d'actions par seconde au lieu de 5
- **Offline** : pas besoin d'émulateur, tourne sur n'importe quelle machine
- **Parallélisable** : lancer 1000 simulations avec des starting seeds différentes
- **Rejouabilité** : permettre de skipper les séquences non-déterministes (dialogues, menus) en les modélisant comme des états probabilistes
- **Statistiques** : heatmaps de positions visitées, cartographie des zones explorées par Pi, détection des softlocks

---

## 2. Pipeline de données

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    PHASE 1 : DUMP (une seule fois)                      │
│                                                                         │
│  BizHawk + Lua                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ 1. Itérer sur tous les map_group (0..N) et map_num (0..M)       │  │
│  │ 2. Lire les entêtes de carte dans la ROM                        │  │
│  │ 3. Lire les grilles de métatiles                                │  │
│  │ 4. Lire les connexions, warps, triggers, signs                  │  │
│  │ 5. Exporter en fichiers texte/data                              │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                    │                                    │
│                                    ▼                                    │
│                        data/maps/ (fichiers .bin/.txt)                 │
│                        data/tables/ (comportements)                    │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                    PHASE 2 : SIMULATION C                               │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  main.c                                                         │  │
│  │  Boucle : pi_digit → action → transducer → log                  │  │
│  └──────────────────────────────────┬───────────────────────────────┘  │
│                                     │                                   │
│  ┌──────────────────────────────────▼───────────────────────────────┐  │
│  │  transducer.c                                                     │  │
│  │  GameState transition(GameState, Action)                          │  │
│  │  - MOUVEMENT : collision, direction, marche                       │  │
│  │  - WARP : portes, bordures de carte, connexions                   │  │
│  │  - INTERACTION : A → trigger, B → run, START → menu              │  │
│  │  - COMBAT : détection herbe → RNG → résultat probabiliste        │  │
│  └──────────────────────────────────┬───────────────────────────────┘  │
│                                     │                                   │
│  ┌──────────────────────────────────▼───────────────────────────────┐  │
│  │  map_data.c                                                       │  │
│  │  - Chargement des fichiers dumpés                                 │  │
│  │  - Hash table (map_group, map_num) → MapData                      │  │
│  │  - Lookup métatiles, comportements, warps                         │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  pi_generator.c                                                   │  │
│  │  - Algorithme de Gibbons porté en C (BigInteger → uint64)        │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Structure du projet C

```
pitop-sim-c/
├── Makefile
├── README.md
│
├── include/
│   ├── pi_generator.h
│   ├── game_state.h
│   ├── map_data.h
│   ├── metatile.h
│   ├── collision.h
│   ├── transducer.h
│   ├── simulator.h
│   └── logger.h
│
├── src/
│   ├── main.c                   # Point d'entrée
│   ├── pi_generator.c           # Gibbons algorithm
│   ├── map_data.c               # Map loader + storage
│   ├── map_data_parser.c        # Parser des fichiers dump
│   ├── metatile.c               # Comportements des métatiles
│   ├── collision.c              # Collision/walkability checks
│   ├── transducer.c             # Moteur de jeu
│   ├── transducer_movement.c    # Mouvement + collision
│   ├── transducer_warp.c        # Warps + connexions
│   ├── transducer_interact.c    # A/B/START
│   ├── transducer_battle.c      # Combats aléatoires (RNG)
│   ├── simulator.c              # Boucle principale
│   └── logger.c                 # Logging + statistiques
│
├── data/
│   ├── maps/                    # Dumps des cartes
│   │   ├── 0_0.bin              # map_group=0, map_num=0 (Littleroot)
│   │   ├── 0_1.bin
│   │   └── ...
│   ├── tables/
│   │   ├── primary_tileset.bin   # Comportements tileset 0 (IDs 0-511)
│   │   └── secondary_tilesets.bin # Tous les tilesets secondaires
│   └── index.csv                # Index de toutes les cartes
│
├── dump_scripts/                # Scripts Lua pour le dump
│   ├── dump_all_maps.lua
│   └── dump_tilesets.lua
│
└── tests/
    ├── test_transducer.c
    ├── test_collision.c
    └── test_pi_generator.c
```

---

## 4. Définition des types (datamodel C)

### 4.1 Position / Direction / Action

```c
// include/game_state.h

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef enum {
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_A,
    ACTION_B,
    ACTION_START
} Action;

typedef struct {
    uint8_t map_group;
    uint8_t map_num;
    uint8_t x;
    uint8_t y;
    Direction dir;
} Position;

typedef struct {
    Position pos;
    uint32_t rng;           // LCG: s[n+1] = s[n] * 0x41C64E6D + 0x3039
    uint8_t event_flags[1024]; // bitfield, 8192 flags
    uint8_t game_mode;      // 0=overworld, 1=menu, 2=battle, 3=dialog
    uint8_t dialog_count;   // compteur A-spam pour sortir des dialogues
    bool running;           // running shoes on/off
} GameState;

#endif
```

### 4.2 MapData

```c
// include/map_data.h

#ifndef MAP_DATA_H
#define MAP_DATA_H

#include "game_state.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_WARPS 32
#define MAX_TRIGGERS 16
#define MAX_SIGNS 16
#define MAX_EVENTS 64
#define MAX_CONNECTIONS 4

typedef enum {
    CONNECTION_NONE,
    CONNECTION_NORTH,
    CONNECTION_SOUTH,
    CONNECTION_WEST,
    CONNECTION_EAST
} ConnectionDirection;

typedef struct {
    ConnectionDirection dir;
    uint8_t dest_map_group;
    uint8_t dest_map_num;
    uint8_t offset;  // décalage d'alignement
} MapConnection;

typedef struct {
    uint8_t x, y;
    uint8_t dest_map_group;
    uint8_t dest_map_num;
    uint8_t dest_x, dest_y;
    bool hole;        // trou (fall down)
} WarpData;

typedef struct {
    uint8_t x, y;
    uint8_t trigger_type;  // 0=script, 1=sign
    uint16_t script_id;
} TriggerData;

typedef struct {
    uint8_t map_group;
    uint8_t map_num;
    uint8_t width;
    uint8_t height;
    uint8_t primary_tileset;
    uint8_t secondary_tileset;
    uint16_t *metatiles;         // width × height
    MapConnection connections[4];
    int num_warps;
    WarpData warps[MAX_WARPS];
    int num_triggers;
    TriggerData triggers[MAX_TRIGGERS];
    int num_events;
    uint8_t event_ids[MAX_EVENTS];
} MapData;

// Chargement / déchargement
MapData *map_load(uint8_t group, uint8_t num);
void map_free(MapData *map);
uint16_t map_get_metatile(MapData *map, int x, int y);
bool map_in_bounds(MapData *map, int x, int y);

// Lookup global
void map_init_index(const char *index_path);
MapData *map_get(uint8_t group, uint8_t num);

#endif
```

### 4.3 Metatile / Collision

```c
// include/metatile.h

#ifndef METATILE_H
#define METATILE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MB_NORMAL           = 0x00, // Walkable
    MB_TALL_GRASS       = 0x01, // Combats possibles
    MB_WALL             = 0x02, // Bloquant
    MB_WATER            = 0x03, // Surf requis
    MB_DOOR             = 0x04, // Porte animée
    MB_WARP             = 0x05, // Warp / changement de carte
    MB_LEDGE            = 0x06, // Saut (marche bloquée, saut par dessus)
    MB_SAND             = 0x07, // Stems similaires à normal
    MB_STAIRS           = 0x08, // Escalier (collision normale)
    MB_UNDERWATER       = 0x09, // Sous l'eau (Deep Savenger)
    MB_JUMP_WEST        = 0x0A,
    MB_JUMP_EAST        = 0x0B,
    MB_JUMP_SOUTH       = 0x0C,
    MB_JUMP_NORTH       = 0x0D,
    MB_OCEAN_WATER      = 0x0E,
    MB_POND_WATER       = 0x0F,
    MB_SHALLOW_WATER    = 0x10,
    MB_MOUNTAIN_TOP     = 0x11,
    MB_ANIMATED_DOOR    = 0x12,
    MB_BERRY_TREE       = 0x13,
    MB_MUDDY_SLOPE      = 0x14,
    MB_PUDDLE           = 0x15,
    MB_UNKNOWN          = 0xFF
} MetatileBehavior;

typedef struct {
    uint16_t id;
    MetatileBehavior behavior;
    bool walkable;      // true si on peut marcher dessus
    bool surfable;      // true si on peut surfer
    bool triggers_battle; // herbes hautes
    bool is_door;       // porte → warp
    bool is_ledge;      // saut directionnel
} Metatile;

// Init des tables depuis le fichier dumpé
void metatile_init(const char *primary_path, const char *secondary_path);
Metatile metatile_get(uint8_t tileset_id, uint16_t metatile_id);
bool metatile_is_walkable(uint8_t tileset_id, uint16_t metatile_id);

#endif
```

---

## 5. Transducer — le moteur de jeu

### Algorithme principal

```
GameState transition(GameState state, Action action):
    // 1. Copier l'état (immutabilité)
    // 2. Avancer le RNG
    state.rng = lcg_step(state.rng)

    // 3. Traiter l'action
    switch action:
        case UP/DOWN/LEFT/RIGHT → handle_movement(state, action)
        case A                 → handle_a_button(state)
        case B                 → handle_b_button(state)
        case START             → handle_start_button(state)

    // 4. Vérifications post-mouvement
    // (uniquement si la position a changé)
    if state.pos a changé:
        check_grass_battle(state)   // combat aléatoire ?
        check_warp_trigger(state)   // warp au sol ?
        check_map_border(state)     // sortie de carte ?

    return state
```

### Gestion du mouvement

```c
// transducer_movement.c

#define TARGET_COORDS(x, y, d) \
    d == DIR_UP    ? (x), (y)-1 : \
    d == DIR_DOWN  ? (x), (y)+1 : \
    d == DIR_LEFT  ? (x)-1, (y) : \
                     (x)+1, (y)

bool handle_movement(GameState *state, Action action) {
    Direction new_dir = action_to_dir(action);
    state->pos.dir = new_dir;

    MapData *map = map_get(state->pos.map_group, state->pos.map_num);
    if (!map) return false;

    int tx, ty;
    target_coords(state->pos.x, state->pos.y, new_dir, &tx, &ty);

    // Vérifier si on reste dans la carte
    if (!map_in_bounds(map, tx, ty)) {
        // Tentative de connexion à la carte voisine
        return try_map_connection(state, new_dir);
    }

    // Vérifier la collision
    uint16_t tile = map_get_metatile(map, tx, ty);
    uint8_t tileset = tile < 512 ? map->primary_tileset : map->secondary_tileset;
    // Note: IDs 0-511 = tileset primaire, 512+ = tileset secondaire
    // (à valider selon le format réel des dumps)

    Metatile meta = metatile_get(tileset, tile);

    if (!meta.walkable && !meta.is_ledge) {
        return false;  // Mur ou équivalent : pas de mouvement
    }

    // Marcher sur de l'herbe → flag pour check battle
    state->walked_on_grass = meta.triggers_battle;

    // Déplacement effectif
    state->pos.x = tx;
    state->pos.y = ty;
    return true;
}
```

### Gestion des connexions entre cartes

```c
// transducer_warp.c

bool try_map_connection(GameState *state, Direction dir) {
    MapData *map = map_get(state->pos.map_group, state->pos.map_num);
    if (!map) return false;

    // Trouver la connexion dans la direction
    MapConnection *conn = NULL;
    for (int i = 0; i < 4; i++) {
        if (map->connections[i].dir == dir) {
            conn = &map->connections[i];
            break;
        }
    }
    if (!conn) return false; // Pas de connexion, bloqué

    // Charger la carte destination
    MapData *dest = map_get(conn->dest_map_group, conn->dest_map_num);
    if (!dest) return false;

    // Calculer la nouvelle position sur la carte destination
    int new_x, new_y;
    switch (dir) {
        case DIR_NORTH: new_x = state->pos.x; new_y = dest->height - 1; break;
        case DIR_SOUTH: new_x = state->pos.x; new_y = 0; break;
        case DIR_WEST:  new_x = dest->width - 1; new_y = state->pos.y; break;
        case DIR_EAST:  new_x = 0; new_y = state->pos.y; break;
    }

    state->pos.map_group = conn->dest_map_group;
    state->pos.map_num   = conn->dest_map_num;
    state->pos.x = new_x;
    state->pos.y = new_y;
    return true;
}

bool try_warp(GameState *state, int trigger_x, int trigger_y) {
    MapData *map = map_get(state->pos.map_group, state->pos.map_num);
    if (!map) return false;

    for (int i = 0; i < map->num_warps; i++) {
        if (map->warps[i].x == trigger_x && map->warps[i].y == trigger_y) {
            WarpData *w = &map->warps[i];
            state->pos.map_group = w->dest_map_group;
            state->pos.map_num   = w->dest_map_num;
            state->pos.x = w->dest_x;
            state->pos.y = w->dest_y;
            return true;
        }
    }
    return false;
}
```

### Gestion des interactions (A / B / Start)

```c
// transducer_interact.c

void handle_a_button(GameState *state) {
    // Mode menu → confirmer
    if (state->game_mode != MODE_OVERWORLD) {
        state->dialog_count++;
        if (state->dialog_count > 60) {
            // Après 60 A presses : sortir du mode dialogue
            state->game_mode = MODE_OVERWORLD;
            state->dialog_count = 0;
        }
        return;
    }

    // Overworld : vérifier ce qui est face au joueur
    MapData *map = map_get(state->pos.map_group, state->pos.map_num);
    int fx, fy;
    facing_coords(state->pos, &fx, &fy);

    // Vérifier si c'est un warp (porte, etc.)
    for (int i = 0; i < map->num_warps; i++) {
        if (map->warps[i].x == fx && map->warps[i].y == fy) {
            try_warp(state, fx, fy);
            return;
        }
    }

    // Vérifier si c'est un trigger / NPC
    for (int i = 0; i < map->num_triggers; i++) {
        if (map->triggers[i].x == fx && map->triggers[i].y == fy) {
            // Dialog triggered
            state->game_mode = MODE_DIALOG;
            return;
        }
    }
    // Rien d'interactif : A ne fait rien
}

void handle_b_button(GameState *state) {
    // Overworld : toggle running shoes
    if (state->game_mode == MODE_OVERWORLD) {
        state->running = !state->running;
    } else {
        // Menu/dialog : back/cancel
        state->dialog_count = 0;
        state->game_mode = MODE_OVERWORLD;
    }
}

void handle_start_button(GameState *state) {
    if (state->game_mode == MODE_OVERWORLD) {
        state->game_mode = MODE_MENU;
    } else {
        state->game_mode = MODE_OVERWORLD;
    }
}
```

### Gestion des combats aléatoires (RNG)

```c
// transducer_battle.c

void check_grass_battle(GameState *state) {
    MapData *map = map_get(state->pos.map_group, state->pos.map_num);
    uint16_t tile = map_get_metatile(map, state->pos.x, state->pos.y);
    Metatile meta = metatile_get(/* tileset */, tile);

    if (!meta.triggers_battle) return;

    // Le jeu utilise le RNG + le pas du joueur pour décider d'un combat
    // Taux de rencontre : ~8-12% par pas selon la zone
    // Pour une simulation simplifiée : seuil basé sur RNG
    uint32_t encounter_threshold = 0x0A000000; // ~10%
    if (state->rng % 0xFFFFFFFF < encounter_threshold) {
        // Wild battle !
        state->game_mode = MODE_BATTLE;
        state->battle_rng_seed = state->rng;
        // Dans une version simplifiée : on simule le combat
        simulate_battle_outcome(state);
    }
}

void simulate_battle_outcome(GameState *state) {
    // Modèle probabiliste simplifié :
    // - Pi peut appuyer sur A (combattre/capturer), B (fuir),
    //   START (menu), ou les directions
    // - Dans un simulateur simplifié, on peut modéliser :
    //   60% : fuite (B suffit plusieurs fois)
    //   30% : combat gagné (A spam + RNG favorable)
    //   10% : combat perdu / softlock (retour au dernier centre Pokemon)

    uint32_t r = state->rng % 100;
    if (r < 60) {
        // Fuite réussie
        state->game_mode = MODE_OVERWORLD;
    } else if (r < 90) {
        // Combat gagné (gain d'XP)
        state->game_mode = MODE_OVERWORLD;
        state->total_faints++;
    } else {
        // Défaite → retour au centre Pokemon
        state->game_mode = MODE_OVERWORLD;
        state->total_deaths++;
        warp_to_last_center(state);
    }
}
```

---

## 6. Algorithme de Gibbons en C

```c
// src/pi_generator.c

#include "pi_generator.h"
#include <gmp.h>   // Utilisation de GMP pour les BigIntegers
// Alternative : implémentation maison avec uint64_t limitée

typedef struct {
    mpz_t q, r, t, k, n, l;
} PiGenerator;

void pi_init(PiGenerator *pg) {
    mpz_init_set_ui(pg->q, 1);
    mpz_init_set_ui(pg->r, 0);
    mpz_init_set_ui(pg->t, 1);
    mpz_init_set_ui(pg->k, 1);
    mpz_init_set_ui(pg->n, 3);
    mpz_init_set_ui(pg->l, 3);
}

int pi_next_digit(PiGenerator *pg) {
    while (1) {
        // 4*q + r - t < n*t
        mpz_t tmp1, tmp2;
        mpz_init(tmp1);
        mpz_init(tmp2);

        mpz_mul_ui(tmp1, pg->q, 4);
        mpz_add(tmp1, tmp1, pg->r);
        mpz_sub(tmp1, tmp1, pg->t);
        mpz_mul(tmp2, pg->n, pg->t);

        if (mpz_cmp(tmp1, tmp2) < 0) {
            int digit = mpz_get_ui(pg->n);

            mpz_mul_ui(tmp1, pg->t, 10);
            mpz_mul_ui(tmp2, pg->n, 10);
            mpz_t nr, nn;
            mpz_init(nr);
            mpz_init(nn);
            mpz_sub(nr, pg->r, tmp2);
            mpz_mul_ui(nr, nr, 10);
            mpz_mul_ui(nn, pg->q, 3);
            mpz_add(nn, nn, pg->r);
            mpz_div(nn, nn, pg->t);
            mpz_sub_ui(nn, nn, digit * 10);
            // ... simplification: voir l'implémentation Java

            mpz_set(pg->r, nr);
            mpz_set(pg->n, nn);
            mpz_mul_ui(pg->q, pg->q, 10);

            mpz_clear(tmp1); mpz_clear(tmp2);
            mpz_clear(nr); mpz_clear(nn);
            return digit;
        } else {
            mpz_mul_ui(tmp1, pg->q, 2);
            mpz_add(tmp1, tmp1, pg->r);
            mpz_mul(tmp1, tmp1, pg->l);
            // ... suite de l'algorithme
        }
    }
}
```

**Alternative sans GMP** : l'algorithme de Gibbons utilise des `BigInteger` de taille croissante. Pour Pi uniquement, on peut pré-calculer les 10M premières décimales et les stocker dans un tableau (40 Mo). Cela suffit pour des simulations très longues et évite la dépendance à GMP. Le fichier peut être généré une fois par le Java `PiGenerator` et sauvegardé.

---

## 7. Script Lua de dump automatique des cartes

```lua
-- dump_scripts/dump_all_maps.lua
-- But : itérer sur toutes les cartes de Pokemon Saphir et exporter
--       les données (métatiles, warps, connexions, triggers)

-- Adresses ROM pour les entêtes de carte (à vérifier pour Saphir v1.1)
local ROM_BASE = 0x08000000

-- Structures GBA : chaque entête de carte est un bloc de 20+ octets
-- Layout d'un map header (d'après les recherches de la communauté):
--   +0 : u8  width        (en blocs de 16px)
--   +1 : u8  height       (en blocs de 16px)
--   +2 : u16 primary_tileset
--   +4 : u16 secondary_tileset
--   +6 : u8  border_width
--   +7 : u8  border_height
--   +8 : u32 *metatile_data_ptr
--  +12 : u32 *warp_data_ptr
--  +16 : u32 *trigger_data_ptr
--  +20 : u32 *connection_data_ptr
--  +24 : etc.

local function read_map_header(map_group, map_num)
    -- Chercher dans la table de pointeurs (map_layout_table)
    local layout_ptr = read_pointer_at(0x0D3A64 + map_group * 4 + map_num * 4)
    -- Note: ces adresses sont approximatives, à vérifier
    --       sur la ROM avec un debugger / documentation de decompr

    local map_data_ptr = read_long(layout_ptr)

    local width     = memory.readbyte(map_data_ptr, "ROM")
    local height    = memory.readbyte(map_data_ptr + 1, "ROM")
    local prim_tset = memory.readword(map_data_ptr + 2, "ROM")
    local sec_tset  = memory.readword(map_data_ptr + 4, "ROM")
    local metatile_ptr = read_long(read_long(map_data_ptr + 8))

    -- Lire la grille de métatiles : width * height * 2 octets
    local metatiles = {}
    for y = 0, height - 1 do
        metatiles[y] = {}
        for x = 0, width - 1 do
            local addr = metatile_ptr + (y * width + x) * 2
            metatiles[y][x] = memory.readword(addr, "ROM")
        end
    end

    return {
        map_group = map_group,
        map_num = map_num,
        width = width,
        height = height,
        primary_tileset = prim_tset,
        secondary_tileset = sec_tset,
        metatiles = metatiles,
        -- etc.
    }
end

local function export_map_to_file(data, path)
    local file = io.open(path, "w")
    file:write(string.format("%d %d %d %d\n",
        data.width, data.height,
        data.primary_tileset, data.secondary_tileset
    ))
    for y = 0, data.height - 1 do
        for x = 0, data.width - 1 do
            file:write(data.metatiles[y][x])
            if x < data.width - 1 then file:write(" ") end
        end
        file:write("\n")
    end
    file:close()
end

-- Boucle principale de dump
for mg = 0, 15 do     -- Limite à trouver
    for mn = 0, 255 do
        local ok, data = pcall(read_map_header, mg, mn)
        if ok and data and data.width > 0 then
            local path = string.format("data/maps/%d_%d.txt", mg, mn)
            export_map_to_file(data, path)
        end
    end
end
```

---

## 8. Boucle principale du simulateur

```c
// src/simulator.c

#include "simulator.h"
#include "pi_generator.h"
#include "transducer.h"
#include "logger.h"
#include <time.h>

void run_simulation(uint64_t total_actions) {
    // Initialisation
    PiGenerator pg;
    pi_init(&pg);

    GameState state = {
        .pos = { .map_group = 0, .map_num = 0, .x = 7, .y = 7, .dir = DIR_DOWN },
        .rng = 0,
        .game_mode = MODE_OVERWORLD,
        .dialog_count = 0,
        .running = false
    };

    Logger log;
    log_init(&log, "simulation.log");

    time_t start = time(NULL);
    uint64_t frames = 0;

    for (uint64_t step = 0; step < total_actions; step++) {
        int digit = pi_next_digit(&pg);
        Action action = digit_to_action(digit);

        state = transition(state, action);
        frames += 12;  // chaque action = 12 frames

        // Log toutes les 5000 actions
        if (step % 5000 == 0) {
            time_t now = time(NULL);
            double speed = (now - start) > 0
                ? (double)step / (now - start)
                : 0;

            log_write(&log,
                "[Frames: %llu] Step: %llu | Digit: %d -> %s | "
                "Map: %d-%d | Pos: (%d,%d) | %s | Speed: %.0f acts/s",
                frames, step, digit, action_name(action),
                state.pos.map_group, state.pos.map_num,
                state.pos.x, state.pos.y,
                direction_name(state.pos.dir),
                speed
            );
        }

        // Stats running : heatmap
        log_record_position(&log, state.pos);
    }

    log_close(&log);
    log_print_summary(&log);
}

Action digit_to_action(int digit) {
    // Mapping exact de Main.java
    switch (digit) {
        case 1: return ACTION_UP;
        case 2: return ACTION_DOWN;
        case 3: return ACTION_LEFT;
        case 4: return ACTION_RIGHT;
        case 5: return ACTION_A;
        case 6: return ACTION_B;
        case 7: return ACTION_START;
        case 8: return ACTION_B;
        case 9: return ACTION_B;
        case 0: return ACTION_A;
        default: return ACTION_UP;
    }
}
```

---

## 9. Format des fichiers de dump

### data/maps/{group}_{num}.txt

```
<width> <height> <primary_tileset_id> <secondary_tileset_id>
<connexion_count>
<dir> <dest_group> <dest_num> <offset>
<dir> <dest_group> <dest_num> <offset>  (max 4)
<warp_count>
<x> <y> <dest_group> <dest_num> <dest_x> <dest_y> [hole]
... (max 32)
<trigger_count>
<x> <y> <type> <script_id>
... (max 16)
<metatile_grid width×height lignes>
<id> <id> <id> ...
<id> <id> <id> ...
```

### data/index.csv

```
group,num,width,height,primary_ts,secondary_ts,file
0,0,20,20,0,1,maps/0_0.txt
0,1,30,30,0,2,maps/0_1.txt
...
```

---

## 10. Roadmap

### Phase 1 — Dump des cartes (1-2 jours)
- [ ] Identifier les structures ROM exactes des entêtes de carte pour Saphir v1.1
- [ ] Écrire un script Lua qui dump une carte donnée (map_group, map_num)
- [ ] Valider le dump en comparant avec les fichiers déjà présents dans `dump/`
- [ ] Étendre à toutes les cartes accessibles (itération automatique)
- [ ] Dumper les tables de comportement des tilesets secondaires

### Phase 2 — Squelette C (2-3 jours)
- [ ] Créer la structure de projet (Makefile, headers, src)
- [ ] Implémenter `pi_generator.c` (Gibbons ou pré-calcul)
- [ ] Implémenter le parser de fichiers de cartes (`map_data_parser.c`)
- [ ] Implémenter le loader + hash table (`map_data.c`)
- [ ] Implémenter la table des métatiles + comportements (`metatile.c`, `collision.c`)
- [ ] Tester le chargement de Bourg-en-Vol (données existantes)

### Phase 3 — Transducer minimal (3-5 jours)
- [ ] `handle_movement` avec collisions (marche / mur)
- [ ] `try_map_connection` (bordures de carte)
- [ ] `try_warp` (portes)
- [ ] Boucle principale qui logue la progression
- [ ] Tester sur Bourg-en-Vol : Pi doit pouvoir entrer dans les maisons, aller au labo
- [ ] Valider en comparant frame par frame avec un run réel dans BizHawk

### Phase 4 — Simulation enrichie (5-7 jours)
- [ ] Herbes hautes → combats aléatoires (modèle RNG)
- [ ] Interactions A (warps, triggers, dialogues simplifiés)
- [ ] B (running shoes), Start (menu → skip)
- [ ] Gestion des modes (overworld, dialog, battle, menu)
- [ ] Statistiques : heatmap, zones visitées, softlocks détectés, nombre de pas
- [ ] Export des traces au format compatible avec l'analyseur Java existant

### Phase 5 — Exploitation (continu)
- [ ] Lancer Pi sur la simulation complète (10⁹ actions = ~1 seconde CPU)
- [ ] Comparer avec le run réel en cours
- [ ] Détecter les routes que Pi explore vs celles qu'il rate systématiquement
- [ ] Tester des mappings d'actions alternatifs (favoriser A/B ?)
- [ ] Tester avec d'autres nombres transcendants (e, √2, φ)

---

## 11. Dépendances C

| Bibliothèque | Utilité | Alternative |
|---|---|---|
| **GMP** (libgmp-dev) | BigIntegers pour Pi | Pré-calculer les décimales en Java et les embarquer |
| **glib** (libglib2.0) | Hash tables, listes chaînées | Implémentation maison minimale |
| **argp / getopt** | Parsing des arguments CLI | stdlib.h / getopt.h |
| **zlib** | Compression des traces | Optionnel |

Pour minimiser les dépendances : une version "bare metal" sans GMP peut utiliser un fichier pré-calculé des N premières décimales de Pi (40 Mo pour 10M), généré une fois par le code Java existant (`PiGenerator`).

---

## 12. Vérification du simulateur

Pour valider que le simulateur est correct, on compare avec un run réel :

```
Run réel BizHawk :                    Simulation C :
  Frame 0012: Pi=3, map=0-0, (7,7)     Step 0001: Pi=3, map=0-0, (7,7)
  Frame 0024: Pi=1, map=0-0, (7,6)     Step 0002: Pi=1, map=0-0, (7,6)
  Frame 0036: Pi=4, map=0-0, (8,6)     Step 0003: Pi=4, map=0-0, (8,6)
  ...                                    ...

  Les positions, maps et actions doivent coïncider exactement
  (modulo le comportement des NPCs qui est non-déterministe).
```

Pour les divergences inévitables (movement des NPCs, RNG des combats) : on switch en mode **statistique** — le simulateur ne cherche pas à reproduire exactement le run réel, mais à produire la même **distribution de comportements**.
