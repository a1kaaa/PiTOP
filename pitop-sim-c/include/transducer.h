#ifndef TRANSDUCER_H
#define TRANSDUCER_H

#include "game_state.h"

/*
 * transducer.h — Moteur de transition d'état
 *
 * Traduit le concept Java (interface Transducer + classe Simulator)
 * en C. La fonction transition() prend un état et une action, applique
 * les règles du jeu (mouvement, collisions, warps, interactions),
 * et retourne le nouvel état.
 *
 * Logique implémentée :
 *   - UP/DOWN/LEFT/RIGHT  → mouvement avec détection de collision
 *   - A                   → interaction (porte, trigger, dialogue)
 *   - B                   → running shoes / cancel
 *   - START               → menu toggle
 */

/* Applique une action et retourne le nouvel état. */
GameState transducer_apply(GameState state, Action action);

/* Sous-modules (appelés par transducer_apply) : */

/* Mouvement : change la direction, déplace si la case est libre.
 * Gère aussi les connexions entre cartes (bordures). */
GameState transducer_handle_movement(GameState state, Action action);

/* Warp : après un déplacement, vérifie si on est sur un warp. */
GameState transducer_check_warp(GameState state);

/* Interaction bouton A : porte, trigger, dialogue, etc. */
GameState transducer_handle_a(GameState state);

/* Interaction bouton B : running shoes ou cancel. */
GameState transducer_handle_b(GameState state);

/* Interaction bouton START : menu toggle. */
GameState transducer_handle_start(GameState state);

/* Combat aléatoire : après un pas, vérifie l'herbe haute + RNG. */
GameState transducer_check_battle(GameState state);

#endif
