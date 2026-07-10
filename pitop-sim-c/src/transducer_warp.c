#include "transducer.h"
#include "map_data.h"
#include "metatile.h"

/*
 * transducer_warp.c — Warps et changement de carte
 *
 * Après chaque déplacement, on vérifie si la case sur laquelle
 * le joueur se trouve est un warp (porte, escalier, trou, etc.).
 * Les warps sont définis dans le dump de chaque carte.
 *
 * NOTE : les warps "automatiques" (portes, escaliers) sont déclenchés
 * par le pas lui-même. Les warps "sur action" (examiner un meuble)
 * sont déclenchés par le bouton A (traité dans transducer_interact.c).
 */

GameState transducer_check_warp(GameState state) {
    MapData *map = map_get(state.pos.map_group, state.pos.map_num);
    if (!map) return state;

    /* Parcourir les warps de la carte */
    for (int i = 0; i < map->num_warps; i++) {
        WarpData *w = &map->warps[i];
        if (w->x == state.pos.x && w->y == state.pos.y) {
            /* Warp déclenché ! */
            state.pos.map_group = w->dest_map_group;
            state.pos.map_num   = w->dest_map_num;
            state.pos.x = w->dest_x;
            state.pos.y = w->dest_y;

            /* Les warps "hole" (trous) font tomber → direction bas */
            if (w->hole) {
                state.pos.dir = DIR_DOWN;
            }

            break;
        }
    }

    return state;
}
