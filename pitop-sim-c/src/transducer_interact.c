#include "transducer.h"
#include "map_data.h"

/*
 * transducer_interact.c — Boutons A, B, START
 *
 * Gère les interactions non liées au mouvement :
 *
 * A     → Valider / interagir
 *          - En overword : examine la case devant le joueur
 *            (porte, trigger, signe, NPC)
 *          - En dialogue/menu/battle : "avancer le texte"
 *
 * B     → Annuler / Courir
 *          - Overworld : toggle running shoes
 *          - Dialogue/menu : sortir
 *
 * START → Menu pause
 *          - Overworld : ouvre le menu
 *          - Menu : ferme le menu
 *
 * NOTE : la simulation complète des dialogues, menus et combats
 * est complexe. Ces fonctions sont des squelettes qui modélisent
 * le comportement de manière probabiliste / simplifiée.
 */

/* ------------------------------------------------------------------ */
/*  Helper : case en face du joueur                                   */
/* ------------------------------------------------------------------ */

static void facing_position(Position pos, uint8_t *fx, uint8_t *fy) {
    switch (pos.dir) {
        case DIR_UP:    *fx = pos.x;     *fy = pos.y - 1; break;
        case DIR_DOWN:  *fx = pos.x;     *fy = pos.y + 1; break;
        case DIR_LEFT:  *fx = pos.x - 1; *fy = pos.y;     break;
        case DIR_RIGHT: *fx = pos.x + 1; *fy = pos.y;     break;
    }
}

/* ------------------------------------------------------------------ */
/*  Bouton A                                                          */
/* ------------------------------------------------------------------ */

GameState transducer_handle_a(GameState state) {
    switch (state.mode) {
        case MODE_OVERWORLD: {
            /* Regarder ce qui est face au joueur */
            uint8_t fx = 0, fy = 0;
            facing_position(state.pos, &fx, &fy);

            MapData *map = map_get(state.pos.map_group, state.pos.map_num);
            if (!map) return state;

            /* Vérifier les warps face au joueur (portes) */
            for (int i = 0; i < map->num_warps; i++) {
                WarpData *w = &map->warps[i];
                if (w->x == fx && w->y == fy) {
                    state.pos.map_group = w->dest_map_group;
                    state.pos.map_num   = w->dest_map_num;
                    state.pos.x = w->dest_x;
                    state.pos.y = w->dest_y;
                    return state;
                }
            }

            /* Vérifier les triggers face au joueur (NPC, signes) */
            for (int i = 0; i < map->num_triggers; i++) {
                TriggerData *t = &map->triggers[i];
                if (t->x == fx && t->y == fy && t->type == 0) {
                    /* NPC : passage en mode dialogue */
                    state.mode = MODE_DIALOG;
                    state.dialog_counter = 0;
                    return state;
                }
            }

            /* Rien d'interactif : A ne fait rien */
            return state;
        }

        case MODE_DIALOG:
        case MODE_MENU:
        case MODE_BATTLE:
            /* Avancer le dialogue / confirmer */
            state.dialog_counter++;
            if (state.dialog_counter >= 3) {
                /* Après 3 A pressés, on sort du mode */
                state.mode = MODE_OVERWORLD;
                state.dialog_counter = 0;
            }
            return state;

        default:
            return state;
    }
}

/* ------------------------------------------------------------------ */
/*  Bouton B                                                          */
/* ------------------------------------------------------------------ */

GameState transducer_handle_b(GameState state) {
    switch (state.mode) {
        case MODE_OVERWORLD:
            /* B toggle les running shoes */
            state.running_shoes = !state.running_shoes;
            return state;

        case MODE_DIALOG:
        case MODE_MENU:
            /* B annule → retour overworld */
            state.mode = MODE_OVERWORLD;
            state.dialog_counter = 0;
            return state;

        case MODE_BATTLE:
            /* B = fuite (attempt) */
            /* Dans le simulateur simplifié : la fuite réussit 60% du temps */
            if ((state.rng % 100) < 60) {
                state.mode = MODE_OVERWORLD;
            }
            return state;

        default:
            return state;
    }
}

/* ------------------------------------------------------------------ */
/*  Bouton START                                                      */
/* ------------------------------------------------------------------ */

GameState transducer_handle_start(GameState state) {
    switch (state.mode) {
        case MODE_OVERWORLD:
            state.mode = MODE_MENU;
            state.dialog_counter = 0;
            return state;

        case MODE_MENU:
            state.mode = MODE_OVERWORLD;
            return state;

        case MODE_DIALOG:
            /* START pendant un dialogue : ferme le dialogue */
            state.mode = MODE_OVERWORLD;
            state.dialog_counter = 0;
            return state;

        default:
            return state;
    }
}
