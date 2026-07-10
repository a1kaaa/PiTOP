#include "transducer.h"
#include "map_data.h"
#include "metatile.h"

/*
 * transducer.c — Point d'entrée du moteur de jeu
 *
 * Dispatche chaque action vers le sous-module approprié,
 * puis applique les vérifications post-mouvement (warp, combat).
 *
 * Chaque appel = 12 frames simulées (constante du Lua HOLD_FRAMES).
 */

GameState transducer_apply(GameState state, Action action) {
    /* Avancer le RNG à chaque action */
    state.rng = lcg_step(state.rng);

    switch (action) {
        case ACTION_UP:
        case ACTION_DOWN:
        case ACTION_LEFT:
        case ACTION_RIGHT:
            state = transducer_handle_movement(state, action);
            state = transducer_check_warp(state);
            state = transducer_check_battle(state);
            break;

        case ACTION_A:
            state = transducer_handle_a(state);
            break;

        case ACTION_B:
            state = transducer_handle_b(state);
            break;

        case ACTION_START:
            state = transducer_handle_start(state);
            break;
    }

    return state;
}
