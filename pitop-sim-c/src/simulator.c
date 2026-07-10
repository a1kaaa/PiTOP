#include "simulator.h"
#include "pi_generator.h"
#include "transducer.h"
#include "map_data.h"
#include "metatile.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * simulator.c — Boucle principale de simulation
 *
 * Implementation de simulator_run() : initialise les composants,
 * puis boucle sur les actions de Pi en appliquant le transducer
 * à chaque étape.
 */

int simulator_run(SimConfig *cfg) {
    /* 1. Initialiser le générateur de Pi */
    PiGenerator *pi = pi_generator_new(cfg->pi_file);
    if (!pi) {
        fprintf(stderr, "simulator: impossible d'initialiser Pi\n");
        return 1;
    }

    /* 2. Charger l'index des cartes */
    map_init_index(cfg->map_index);

    /* 3. Charger les tables de métatiles */
    metatile_load_primary("dump/tables/general_metatile_attributes.txt");
    metatile_load_secondary(1, "dump/tables/petalburg_metatile_attributes.txt");
    metatile_load_secondary(2, "dump/tables/lab_metatile_attributes.txt");

    /* 4. Initialiser le logger */
    Logger logger;
    char trace_path[1024];
    char heatmap_path[1024];

    snprintf(trace_path, sizeof(trace_path), "%s.trace", cfg->log_file);
    snprintf(heatmap_path, sizeof(heatmap_path), "%s.heatmap", cfg->log_file);

    if (logger_init(&logger, cfg->log_file, trace_path, heatmap_path) != 0) {
        fprintf(stderr, "simulator: erreur d'initialisation du logger\n");
        pi_generator_free(pi);
        return 1;
    }

    /* 5. État initial */
    GameState state = cfg->initial_state;
    time_t start_time = time(NULL);
    uint64_t frames = 0;

    printf("Simulation démarrée...\n");

    /* 6. Boucle principale */
    for (uint64_t step = 0; step < cfg->total_actions; step++) {
        /* Lire la décimale de Pi */
        int digit = pi_generator_next(pi);
        int action_id = pi_digit_to_action(digit);
        Action action = (Action)action_id;

        /* Appliquer le transducer */
        state = transducer_apply(state, action);

        /* Compter les frames (12 par action comme dans le Lua) */
        frames += 12;

        /* Logger la frame */
        logger_log_frame(&logger, &state, digit, action, step);
        logger_record_position(&logger, &state);

        /* Afficher la progression à intervalle régulier */
        if ((step + 1) % cfg->log_interval == 0) {
            time_t now = time(NULL);
            double elapsed = (now - start_time) > 0 ? (now - start_time) : 1;
            double speed = (double)(step + 1) / elapsed;

            printf("[Frames: %llu] Step: %llu | Digit: %d -> %s | "
                   "Map: %d-%d | Pos: (%d,%d) %s | "
                   "Steps: %llu | Battles: %llu | Speed: %.0f act/s\n",
                   (unsigned long long)frames,
                   (unsigned long long)step,
                   digit, action_name(action),
                   state.pos.map_group, state.pos.map_num,
                   state.pos.x, state.pos.y,
                   direction_name(state.pos.dir),
                   (unsigned long long)state.total_steps,
                   (unsigned long long)state.total_battles,
                   speed);
        }

        /* Vérifier le mode jeu : si on reste bloqué trop longtemps
         * dans un mode non-overworld, avancer le compteur */
        if (state.mode != MODE_OVERWORLD) {
            state.dialog_counter++;
            if (state.dialog_counter > 600) {
                /* 600 actions ~ 5 min réelles : timeout, on force retour */
                state.mode = MODE_OVERWORLD;
                state.dialog_counter = 0;
                logger_printf(&logger,
                    "[TIMEOUT] Mode non-overworld pendant %u actions, "
                    "retour forcé en overworld\n",
                    state.dialog_counter);
            }
        }
    }

    /* 7. Finalisation */
    time_t end_time = time(NULL);
    double total_elapsed = (end_time - start_time) > 0
                           ? (end_time - start_time) : 1;

    logger_printf(&logger,
        "\n=== RÉSUMÉ ===\n"
        "Actions simulées : %llu\n"
        "Temps réel       : %.0f s\n"
        "Vitesse moyenne   : %.0f actions/s\n"
        "Pas total         : %llu\n"
        "Combats           : %llu\n"
        "Pokémon vaincus   : %llu\n"
        "Morts             : %llu\n"
        "Position finale   : map %d-%d (%d,%d) %s\n",
        (unsigned long long)cfg->total_actions,
        total_elapsed,
        (double)cfg->total_actions / total_elapsed,
        (unsigned long long)state.total_steps,
        (unsigned long long)state.total_battles,
        (unsigned long long)state.total_faints,
        (unsigned long long)state.total_deaths,
        state.pos.map_group, state.pos.map_num,
        state.pos.x, state.pos.y,
        direction_name(state.pos.dir));

    logger_print_summary(&logger);
    logger_close(&logger);
    pi_generator_free(pi);
    map_free_all();
    metatile_free();

    return 0;
}
