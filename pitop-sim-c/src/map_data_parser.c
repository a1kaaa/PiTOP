#include "map_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * map_data_parser.c — Parse les fichiers de dump de cartes
 *
 * Format attendu (fichier texte) :
 *
 *   LIGNE 1 : <width> <height> <primary_tileset> <secondary_tileset>
 *   LIGNE 2 : <num_connections>
 *   LIGNES 3.. : <dir> <dest_group> <dest_num> <offset>
 *   LIGNE suivante : <num_warps>
 *   LIGNES suivantes : <x> <y> <dest_grp> <dest_num> <dest_x> <dest_y> [hole]
 *   LIGNE suivante : <num_triggers>
 *   LIGNES suivantes : <x> <y> <type> <script_id>
 *   LIGNES restantes : grille de métatiles (height lignes de width entiers)
 */

#define LINE_BUF_SIZE 65536

static int next_line(FILE *f, char *buf, size_t size) {
    if (!fgets(buf, (int)size, f)) return -1;
    size_t len = strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
        buf[--len] = '\0';
    return (int)len;
}

static int parse_connection_dir(const char *s) {
    if (strcmp(s, "NORTH") == 0 || strcmp(s, "0") == 0) return CONN_NORTH;
    if (strcmp(s, "SOUTH") == 0 || strcmp(s, "1") == 0) return CONN_SOUTH;
    if (strcmp(s, "WEST")  == 0 || strcmp(s, "2") == 0) return CONN_WEST;
    if (strcmp(s, "EAST")  == 0 || strcmp(s, "3") == 0) return CONN_EAST;
    return CONN_NONE;
}

MapData *map_data_parse(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "map_data: impossible d'ouvrir %s : %s\n",
                path, strerror(errno));
        return NULL;
    }

    MapData *map = calloc(1, sizeof(MapData));
    if (!map) { fclose(f); return NULL; }

    char buf[LINE_BUF_SIZE];
    int line = 0;

    /* LIGNE 1 : dimensions + tilesets */
    if (next_line(f, buf, sizeof(buf)) < 0) goto parse_err;
    if (sscanf(buf, "%hhu %hhu %hhu %hhu",
               &map->width, &map->height,
               &map->primary_tileset, &map->secondary_tileset) < 4)
        goto parse_err;
    line++;

    /* Allouer la grille de métatiles */
    uint32_t tile_count = (uint32_t)map->width * map->height;
    if (tile_count == 0 || tile_count > 65536) goto parse_err;
    map->metatiles = malloc(tile_count * sizeof(uint16_t));
    if (!map->metatiles) goto parse_err;

    /* LIGNE 2 : connexions */
    if (next_line(f, buf, sizeof(buf)) < 0) goto parse_err;
    int nconn;
    if (sscanf(buf, "%d", &nconn) < 1) goto parse_err;
    if (nconn > MAX_CONNECTIONS) nconn = MAX_CONNECTIONS;
    map->num_connections = nconn;
    line++;

    for (int i = 0; i < nconn; i++) {
        if (next_line(f, buf, sizeof(buf)) < 0) goto parse_err;
        char dir_str[16];
        int dg, dn, off;
        if (sscanf(buf, "%15s %d %d %d", dir_str, &dg, &dn, &off) < 4)
            goto parse_err;
        map->connections[i].dir = parse_connection_dir(dir_str);
        map->connections[i].dest_map_group = (uint8_t)dg;
        map->connections[i].dest_map_num   = (uint8_t)dn;
        map->connections[i].offset = (int8_t)off;
        line++;
    }

    /* LIGNE suivante : warps */
    if (next_line(f, buf, sizeof(buf)) < 0) goto parse_err;
    int nwarp;
    if (sscanf(buf, "%d", &nwarp) < 1) goto parse_err;
    if (nwarp > MAX_WARPS) nwarp = MAX_WARPS;
    map->num_warps = nwarp;
    line++;

    for (int i = 0; i < nwarp; i++) {
        if (next_line(f, buf, sizeof(buf)) < 0) goto parse_err;
        WarpData *w = &map->warps[i];
        char hole_str[8] = "";
        sscanf(buf, "%hhu %hhu %hhu %hhu %hhu %hhu %7s",
               &w->x, &w->y,
               &w->dest_map_group, &w->dest_map_num,
               &w->dest_x, &w->dest_y, hole_str);
        w->hole = (strcmp(hole_str, "hole") == 0);
        line++;
    }

    /* LIGNE suivante : triggers */
    if (next_line(f, buf, sizeof(buf)) < 0) goto parse_err;
    int ntrig;
    if (sscanf(buf, "%d", &ntrig) < 1) goto parse_err;
    if (ntrig > MAX_TRIGGERS) ntrig = MAX_TRIGGERS;
    map->num_triggers = ntrig;
    line++;

    for (int i = 0; i < ntrig; i++) {
        if (next_line(f, buf, sizeof(buf)) < 0) goto parse_err;
        TriggerData *t = &map->triggers[i];
        sscanf(buf, "%hhu %hhu %hhu %hu",
               &t->x, &t->y, &t->type, &t->script_id);
        line++;
    }

    /* Grille de métatiles : height lignes de width entiers */
    for (int y = 0; y < map->height; y++) {
        if (next_line(f, buf, sizeof(buf)) < 0) goto parse_err;
        char *token = strtok(buf, " \t");
        for (int x = 0; x < map->width; x++) {
            if (!token) goto parse_err;
            map->metatiles[y * map->width + x] = (uint16_t)atoi(token);
            token = strtok(NULL, " \t");
        }
        line++;
    }

    fclose(f);
    return map;

parse_err:
    fprintf(stderr, "map_data: erreur de parsing %s à la ligne %d\n",
            path, line);
    if (map->metatiles) free(map->metatiles);
    free(map);
    fclose(f);
    return NULL;
}

void map_data_free(MapData *map) {
    if (map) {
        free(map->metatiles);
        free(map);
    }
}
