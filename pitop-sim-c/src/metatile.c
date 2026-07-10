#include "metatile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * metatile.c — Table des comportements de métatiles
 *
 * Charge les tables de comportement depuis les fichiers dumpés
 * (format texte : "ID | Behavior | Layer"). Les tilesets secondaires
 * sont indexés par leur ID.
 */

/* ------------------------------------------------------------------ */
/*  Tables                                                             */
/* ------------------------------------------------------------------ */

#define MAX_PRIMARY   512
#define MAX_SECONDARY 256  /* nombre max de tilesets secondaires */

static Metatile primary_table[MAX_PRIMARY];
static Metatile *secondary_tables[MAX_SECONDARY];
static int secondary_count = 0;
static bool primary_loaded = false;

/* ------------------------------------------------------------------ */
/*  Parsing d'une ligne de table de comportement                      */
/* ------------------------------------------------------------------ */

static MetatileBehavior parse_behavior(const char *s) {
    if (strstr(s, "NORMAL"))              return MB_NORMAL;
    if (strstr(s, "TALL_GRASS"))          return MB_TALL_GRASS;
    if (strstr(s, "WALL"))                return MB_WALL;
    if (strstr(s, "WATER"))               return MB_WATER;
    if (strstr(s, "DOOR"))                return MB_DOOR;
    if (strstr(s, "WARP"))                return MB_WARP;
    if (strstr(s, "LEDGE"))               return MB_LEDGE;
    if (strstr(s, "SAND"))                return MB_SAND;
    if (strstr(s, "STAIRS"))              return MB_STAIRS;
    if (strstr(s, "UNDERWATER"))          return MB_UNDERWATER;
    if (strstr(s, "JUMP_WEST"))           return MB_JUMP_WEST;
    if (strstr(s, "JUMP_EAST"))           return MB_JUMP_EAST;
    if (strstr(s, "JUMP_SOUTH"))          return MB_JUMP_SOUTH;
    if (strstr(s, "JUMP_NORTH"))          return MB_JUMP_NORTH;
    if (strstr(s, "OCEAN_WATER"))         return MB_OCEAN_WATER;
    if (strstr(s, "POND_WATER"))          return MB_POND_WATER;
    if (strstr(s, "SHALLOW_WATER"))       return MB_SHALLOW_WATER;
    if (strstr(s, "MOUNTAIN_TOP"))        return MB_MOUNTAIN_TOP;
    if (strstr(s, "ANIMATED_DOOR"))       return MB_ANIMATED_DOOR;
    if (strstr(s, "BERRY_TREE"))          return MB_BERRY_TREE;
    if (strstr(s, "MUDDY_SLOPE"))         return MB_MUDDY_SLOPE;
    if (strstr(s, "PUDDLE"))              return MB_PUDDLE;
    if (strstr(s, "NON_ANIMATED_DOOR"))   return MB_NON_ANIMATED_DOOR;
    if (strstr(s, "SECRET_BASE"))         return MB_SECRET_BASE;
    if (strstr(s, "SOUTH_ARROW_WARP"))    return MB_SOUTH_ARROW_WARP;
    return MB_UNKNOWN;
}

static Metatile make_metatile(uint16_t id, MetatileBehavior behavior) {
    Metatile m = {
        .id = id,
        .behavior = behavior,
        .walkable = false,
        .surfable = false,
        .triggers_battle = false,
        .is_door = false,
        .is_warp = false,
        .is_ledge = false
    };

    switch (behavior) {
        case MB_NORMAL:
            m.walkable = true;
            break;
        case MB_TALL_GRASS:
            m.walkable = true;
            m.triggers_battle = true;
            break;
        case MB_WALL:
        case MB_MOUNTAIN_TOP:
            break; /* pas walkable */
        case MB_WATER:
        case MB_OCEAN_WATER:
        case MB_POND_WATER:
        case MB_SHALLOW_WATER:
            m.surfable = true;
            break;
        case MB_DOOR:
        case MB_ANIMATED_DOOR:
        case MB_NON_ANIMATED_DOOR:
            m.walkable = true;
            m.is_door = true;
            break;
        case MB_WARP:
        case MB_SOUTH_ARROW_WARP:
            m.walkable = true;
            m.is_warp = true;
            break;
        case MB_LEDGE:
        case MB_JUMP_WEST:
        case MB_JUMP_EAST:
        case MB_JUMP_SOUTH:
        case MB_JUMP_NORTH:
            m.walkable = false;
            m.is_ledge = true;
            break;
        case MB_SAND:
        case MB_STAIRS:
        case MB_PUDDLE:
            m.walkable = true;
            break;
        default:
            break;
    }
    return m;
}

/* ------------------------------------------------------------------ */
/*  Chargement d'un fichier de table                                  */
/* ------------------------------------------------------------------ */

static int load_table_file(const char *path, Metatile *table, int max_id) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "metatile: impossible d'ouvrir %s\n", path);
        return -1;
    }

    char buf[512];
    int count = 0;
    while (fgets(buf, sizeof(buf), f)) {
        /* Skip commentaires et entête */
        if (buf[0] == '#' || buf[0] == '\n' || strstr(buf, "-----"))
            continue;

        int id;
        char behavior_str[64] = {0};
        char layer_str[16] = {0};

        if (sscanf(buf, "%d | %63[^|] | %15s", &id, behavior_str, layer_str) >= 2) {
            if (id >= 0 && id < max_id) {
                /* Nettoyer le nom du comportement */
                char *end = behavior_str + strlen(behavior_str) - 1;
                while (end > behavior_str && *end == ' ') *end-- = '\0';
                MetatileBehavior mb = parse_behavior(behavior_str);
                table[id] = make_metatile((uint16_t)id, mb);
                count++;
            }
        }
    }
    fclose(f);
    return count;
}

/* ------------------------------------------------------------------ */
/*  API publique                                                      */
/* ------------------------------------------------------------------ */

void metatile_load_primary(const char *path) {
    int n = load_table_file(path, primary_table, MAX_PRIMARY);
    if (n > 0) {
        primary_loaded = true;
        printf("Metatile: %d entrées chargées (tileset primaire)\n", n);
    }
}

void metatile_load_secondary(int tileset_id, const char *path) {
    if (tileset_id < 0 || tileset_id >= MAX_SECONDARY) return;
    if (secondary_tables[tileset_id]) {
        free(secondary_tables[tileset_id]);
    }
    Metatile *table = calloc(512, sizeof(Metatile));
    int n = load_table_file(path, table, 512);
    if (n > 0) {
        secondary_tables[tileset_id] = table;
        if (tileset_id >= secondary_count)
            secondary_count = tileset_id + 1;
        printf("Metatile: %d entrées chargées (tileset secondaire %d)\n",
               n, tileset_id);
    } else {
        free(table);
    }
}

Metatile metatile_get(int tileset_id, uint16_t metatile_id) {
    Metatile unknown = { .id = metatile_id, .behavior = MB_UNKNOWN };

    if (metatile_id < MAX_PRIMARY && primary_loaded) {
        return primary_table[metatile_id];
    }

    /* IDs 512+ → tileset secondaire */
    if (metatile_id >= 512 && tileset_id >= 0 && tileset_id < secondary_count) {
        Metatile *table = secondary_tables[tileset_id];
        if (table) {
            int idx = metatile_id - 512;
            if (idx >= 0 && idx < 512)
                return table[idx];
        }
    }

    return unknown;
}

bool metatile_is_walkable(int tileset_id, uint16_t metatile_id) {
    return metatile_get(tileset_id, metatile_id).walkable;
}

bool metatile_triggers_battle(int tileset_id, uint16_t metatile_id) {
    return metatile_get(tileset_id, metatile_id).triggers_battle;
}

void metatile_free(void) {
    for (int i = 0; i < secondary_count; i++) {
        free(secondary_tables[i]);
        secondary_tables[i] = NULL;
    }
    secondary_count = 0;
    primary_loaded = false;
}
