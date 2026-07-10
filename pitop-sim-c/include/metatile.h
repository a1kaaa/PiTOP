#ifndef METATILE_H
#define METATILE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * metatile.h — Comportements des métatiles
 *
 * Chaque tuile du jeu (metatile) a un comportement qui détermine
 * si le joueur peut marcher dessus, déclenche un combat, est une
 * porte, etc. Ces données sont dumpées depuis la ROM via le script Lua
 * et chargées au démarrage du simulateur.
 *
 * Les IDs 0-511 appartiennent au tileset primaire (global).
 * Les IDs 512+ appartiennent au tileset secondaire (propre à chaque zone).
 */

/* ------------------------------------------------------------------ */
/*  Comportements de métatiles (repris de la ROM Pokémon)             */
/* ------------------------------------------------------------------ */

typedef enum {
    MB_NORMAL               = 0,
    MB_TALL_GRASS           = 1,
    MB_WALL                 = 2,
    MB_WATER                = 3,
    MB_DOOR                 = 4,
    MB_WARP                 = 5,
    MB_LEDGE                = 6,
    MB_SAND                 = 7,
    MB_STAIRS               = 8,
    MB_UNDERWATER           = 9,
    MB_JUMP_WEST            = 10,
    MB_JUMP_EAST            = 11,
    MB_JUMP_SOUTH           = 12,
    MB_JUMP_NORTH           = 13,
    MB_OCEAN_WATER          = 14,
    MB_POND_WATER           = 15,
    MB_SHALLOW_WATER        = 16,
    MB_MOUNTAIN_TOP         = 17,
    MB_ANIMATED_DOOR        = 18,
    MB_BERRY_TREE           = 19,
    MB_MUDDY_SLOPE          = 20,
    MB_PUDDLE               = 21,
    MB_NON_ANIMATED_DOOR    = 22,
    MB_SECRET_BASE           = 23,
    MB_SOUTH_ARROW_WARP     = 24,
    MB_UNKNOWN              = 0xFF
} MetatileBehavior;

/* ------------------------------------------------------------------ */
/*  Structure Metatile                                                */
/* ------------------------------------------------------------------ */

typedef struct {
    uint16_t id;
    MetatileBehavior behavior;
    bool walkable;
    bool surfable;
    bool triggers_battle;
    bool is_door;
    bool is_warp;
    bool is_ledge;
} Metatile;

/* ------------------------------------------------------------------ */
/*  API publique                                                      */
/* ------------------------------------------------------------------ */

/* Charge la table du tileset primaire (fichier texte dumpé). */
void metatile_load_primary(const char *path);

/* Charge une table de tileset secondaire (un fichier par tileset). */
void metatile_load_secondary(int tileset_id, const char *path);

/* Retourne le comportement d'un métatile. tileset = 0 pour primaire. */
Metatile metatile_get(int tileset_id, uint16_t metatile_id);

/* Raccourci : la tuile est-elle praticable à pied ? */
bool metatile_is_walkable(int tileset_id, uint16_t metatile_id);

/* Raccourci : la tuile déclenche-t-elle des combats aléatoires ? */
bool metatile_triggers_battle(int tileset_id, uint16_t metatile_id);

/* Libère la mémoire allouée pour les tables. */
void metatile_free(void);

#endif
