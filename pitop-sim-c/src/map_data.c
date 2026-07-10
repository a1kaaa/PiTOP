#include "map_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * map_data.c — Cache global des cartes chargées
 *
 * Maintient une table de hachage des cartes (group, num) → MapData*.
 * Les cartes sont chargées à la demande via map_get() et parsées
 * depuis les fichiers dump/.
 */

/* Prototype du parseur déclaré dans map_data_parser.c */
MapData *map_data_parse(const char *path);
void map_data_free(MapData *map);

/* ------------------------------------------------------------------ */
/*  Cache — simple liste chaînée (suffisant pour ~500 cartes)         */
/* ------------------------------------------------------------------ */

typedef struct MapEntry {
    uint8_t group, num;
    MapData *data;
    char file_path[2048];
    struct MapEntry *next;
} MapEntry;

static MapEntry *cache = NULL;
static char base_dir[1024] = "data/maps";

void map_init_index(const char *index_path) {
    /* Lit l'index CSV : group,num,file
     * Chaque ligne associe un couple (group,num) à un fichier. */
    FILE *f = fopen(index_path, "r");
    if (!f) {
        fprintf(stderr, "map_init_index: impossible d'ouvrir %s\n", index_path);
        return;
    }

    /* Extraire le répertoire de base depuis index_path */
    const char *slash = strrchr(index_path, '/');
    if (slash) {
        size_t len = slash - index_path;
        if (len > 0 && len < sizeof(base_dir)) {
            memcpy(base_dir, index_path, len);
            base_dir[len] = '\0';
        }
    }

    char buf[1024];
    int count = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (count == 0 && strstr(buf, "group")) { count++; continue; }

        int g, n;
        char file[256] = {0};
        if (sscanf(buf, "%d,%d,%255s", &g, &n, file) >= 2) {
            MapEntry *entry = calloc(1, sizeof(MapEntry));
            entry->group = (uint8_t)g;
            entry->num   = (uint8_t)n;
            if (file[0]) {
                snprintf(entry->file_path, sizeof(entry->file_path),
                         "%s/%s", base_dir, file);
            } else {
                snprintf(entry->file_path, sizeof(entry->file_path),
                         "%s/%d_%d.txt", base_dir, g, n);
            }
            entry->next = cache;
            cache = entry;
            count++;
        }
    }
    fclose(f);
    printf("MapData: %d cartes indexées depuis %s\n", count, index_path);
}

static MapEntry *find_entry(uint8_t group, uint8_t num) {
    for (MapEntry *e = cache; e; e = e->next)
        if (e->group == group && e->num == num)
            return e;
    return NULL;
}

MapData *map_get(uint8_t group, uint8_t num) {
    MapEntry *entry = find_entry(group, num);
    if (!entry) {
        /* Créer une entrée avec chemin par défaut */
        entry = calloc(1, sizeof(MapEntry));
        entry->group = group;
        entry->num   = num;
        snprintf(entry->file_path, sizeof(entry->file_path),
                 "%s/%d_%d.txt", base_dir, group, num);
        entry->next = cache;
        cache = entry;
    }

    if (!entry->data) {
        entry->data = map_data_parse(entry->file_path);
        if (!entry->data) {
            fprintf(stderr, "map_get(%d, %d): échec chardepuis %s\n",
                    group, num, entry->file_path);
        }
    }
    return entry->data;
}

uint16_t map_get_metatile(MapData *map, int x, int y) {
    if (!map || !map->metatiles) return 0;
    if (x < 0 || x >= map->width || y < 0 || y >= map->height) return 0;
    return map->metatiles[y * (int)map->width + x];
}

bool map_in_bounds(MapData *map, int x, int y) {
    return map && x >= 0 && x < (int)map->width
              && y >= 0 && y < (int)map->height;
}

void map_free_all(void) {
    MapEntry *e = cache;
    while (e) {
        MapEntry *next = e->next;
        if (e->data) map_data_free(e->data);
        free(e);
        e = next;
    }
    cache = NULL;
}
