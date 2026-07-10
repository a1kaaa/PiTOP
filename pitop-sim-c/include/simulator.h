#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "game_state.h"
#include <stdint.h>

/*
 * simulator.h — Boucle principale de la simulation
 *
 * Orchestre la boucle : Pi digit → action → transducer → log.
 * Équivalent du Simulator.java + Main.java du projet Java.
 */

/* Configuration du simulateur. */
typedef struct {
    const char *pi_file;
    const char *map_index;
    const char *log_file;
    uint64_t total_actions;
    uint64_t log_interval;
    GameState initial_state;
} SimConfig;

/* Valeurs par défaut de la configuration. */
#define SIM_CONFIG_DEFAULT { \
    .pi_file = NULL, \
    .map_index = "data/index.csv", \
    .log_file = "simulation.log", \
    .total_actions = 10000000, \
    .log_interval = 5000, \
    .initial_state = { \
        .pos = {0, 0, 7, 7, DIR_DOWN}, \
        .rng = 0, \
        .event_flags = {0}, \
        .mode = MODE_OVERWORLD, \
        .dialog_counter = 0, \
        .running_shoes = false, \
        .total_steps = 0, \
        .total_battles = 0, \
        .total_faints = 0, \
        .total_deaths = 0 \
    } \
}

/* Lance la simulation. Retourne 0 si succès. */
int simulator_run(SimConfig *cfg);

#endif
