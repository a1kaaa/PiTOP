#ifndef MAP_DATA_H
#define MAP_DATA_H

#include <stdint.h>
#include <stdbool.h>

/*
 * map_data.h — Stockage et chargement des cartes dumpées
 *
 * Chaque carte (identifiée par un couple map_group / map_num) contient
 * sa grille de métatiles, ses warps, ses connexions vers les cartes
 * voisines, et ses triggers. Les données sont chargées depuis les
 * fichiers dumpés par le script Lua.
 */

/* ------------------------------------------------------------------ */
/*  Limites                                                            */
/* ------------------------------------------------------------------ */

#define MAX_WARPS        32
#define MAX_TRIGGERS     16
#define MAX_CONNECTIONS   4

/* ------------------------------------------------------------------ */
/*  Types                                                             */
/* ------------------------------------------------------------------ */

typedef enum {
    CONN_NONE,
    CONN_NORTH,
    CONN_SOUTH,
    CONN_WEST,
    CONN_EAST
} ConnectionDir;

typedef struct {
    ConnectionDir dir;
    uint8_t dest_map_group;
    uint8_t dest_map_num;
    int8_t offset;
} MapConnection;

typedef struct {
    uint8_t x, y;
    uint8_t dest_map_group;
    uint8_t dest_map_num;
    uint8_t dest_x, dest_y;
    bool hole;
} WarpData;

typedef struct {
    uint8_t x, y;
    uint8_t type;
    uint16_t script_id;
} TriggerData;

typedef struct {
    uint8_t map_group;
    uint8_t map_num;
    uint8_t width;
    uint8_t height;
    uint8_t primary_tileset;
    uint8_t secondary_tileset;
    uint16_t *metatiles;
    int num_connections;
    MapConnection connections[MAX_CONNECTIONS];
    int num_warps;
    WarpData warps[MAX_WARPS];
    int num_triggers;
    TriggerData triggers[MAX_TRIGGERS];
} MapData;

/* ------------------------------------------------------------------ */
/*  API publique                                                      */
/* ------------------------------------------------------------------ */

/* Initialise l'index des cartes à partir d'un fichier CSV. */
void map_init_index(const char *index_path);

/* Retourne la carte (map_group, map_num). La charge si nécessaire. */
MapData *map_get(uint8_t group, uint8_t num);

/* Raccourci : récupère le métatile à (x, y) sur la carte donnée. */
uint16_t map_get_metatile(MapData *map, int x, int y);

/* Vérifie que (x, y) est dans les limites de la carte. */
bool map_in_bounds(MapData *map, int x, int y);

/* Libère toutes les cartes chargées. */
void map_free_all(void);

#endif
