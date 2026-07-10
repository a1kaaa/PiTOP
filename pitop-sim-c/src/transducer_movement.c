#include "transducer.h"
#include "map_data.h"
#include "metatile.h"

/*
 * transducer_movement.c — Mouvement et collisions
 *
 * Gère les actions UP/DOWN/LEFT/RIGHT :
 *   1. Met à jour la direction du joueur
 *   2. Calcule la case cible
 *   3. Vérifie les collisions via le comportement du métatile
 *   4. Si praticable : déplace le joueur
 *   5. Gère les bordures de carte (connexions)
 */

static void target_coords(uint8_t x, uint8_t y, Direction dir,
                          uint8_t *tx, uint8_t *ty) {
    switch (dir) {
        case DIR_UP:    *tx = x;     *ty = y - 1; break;
        case DIR_DOWN:  *tx = x;     *ty = y + 1; break;
        case DIR_LEFT:  *tx = x - 1; *ty = y;     break;
        case DIR_RIGHT: *tx = x + 1; *ty = y;     break;
    }
}

static int direction_to_connection(Direction dir) {
    switch (dir) {
        case DIR_UP:    return (int)CONN_NORTH;
        case DIR_DOWN:  return (int)CONN_SOUTH;
        case DIR_LEFT:  return (int)CONN_WEST;
        case DIR_RIGHT: return (int)CONN_EAST;
    }
    return (int)CONN_NONE;
}

GameState transducer_handle_movement(GameState state, Action action) {
    /* Traduire l'action en direction */
    Direction new_dir;
    switch (action) {
        case ACTION_UP:    new_dir = DIR_UP;    break;
        case ACTION_DOWN:  new_dir = DIR_DOWN;  break;
        case ACTION_LEFT:  new_dir = DIR_LEFT;  break;
        case ACTION_RIGHT: new_dir = DIR_RIGHT; break;
        default: return state;
    }

    /* La direction change toujours, même si le mouvement est bloqué */
    state.pos.dir = new_dir;

    /* Charger la carte courante */
    MapData *map = map_get(state.pos.map_group, state.pos.map_num);
    if (!map) return state;

    /* Calculer la position cible */
    uint8_t tx, ty;
    target_coords(state.pos.x, state.pos.y, new_dir, &tx, &ty);

    /* Vérifier les limites de la carte */
    if (!map_in_bounds(map, (int)tx, (int)ty)) {
        /* Tentative de connexion vers la carte voisine */
        int conn_dir = direction_to_connection(new_dir);
        for (int i = 0; i < map->num_connections; i++) {
            if ((int)map->connections[i].dir == conn_dir) {
                MapConnection *conn = &map->connections[i];
                MapData *dest = map_get(conn->dest_map_group,
                                        conn->dest_map_num);
                if (!dest) return state;

                state.pos.map_group = conn->dest_map_group;
                state.pos.map_num   = conn->dest_map_num;

                /* Calculer la position d'arrivée sur la nouvelle carte */
                switch (new_dir) {
                    case DIR_UP:
                        state.pos.x = state.pos.x;
                        state.pos.y = dest->height - 1;
                        break;
                    case DIR_DOWN:
                        state.pos.x = state.pos.x;
                        state.pos.y = 0;
                        break;
                    case DIR_LEFT:
                        state.pos.x = dest->width - 1;
                        state.pos.y = state.pos.y;
                        break;
                    case DIR_RIGHT:
                        state.pos.x = 0;
                        state.pos.y = state.pos.y;
                        break;
                }
                state.total_steps++;
                return state;
            }
        }
        /* Aucune connexion : mouvement bloqué */
        return state;
    }

    /* Vérifier la collision sur la case cible */
    uint16_t tile_id = map_get_metatile(map, (int)tx, (int)ty);

    /* Déterminer le tileset : IDs < 512 = primaire, ≥ 512 = secondaire */
    int tileset_id;
    if (tile_id < 512) {
        tileset_id = -1; /* primaire */
    } else {
        tileset_id = map->secondary_tileset;
    }

    if (!metatile_is_walkable(tileset_id, tile_id)) {
        /* Mur, arbre, eau sans surf : bloqué */
        return state;
    }

    /* Déplacement effectif */
    state.pos.x = tx;
    state.pos.y = ty;
    state.total_steps++;

    return state;
}
