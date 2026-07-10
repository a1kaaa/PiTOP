#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

/*
 * logger.c — Système de logging et statistiques
 *
 * Produit trois fichiers :
 *   - simulation.log    : log textuel lisible (progression, événements)
 *   - simulation.trace  : trace binaire format 10 octets/frame
 *   - simulation.heatmap: compteurs de visites par position
 *
 * Le format binaire de la trace est compatible avec BinaryTraceStream.java.
 */

int logger_init(Logger *log, const char *log_path,
                const char *trace_path, const char *heatmap_path) {
    memset(log, 0, sizeof(Logger));

    log->log_file = fopen(log_path, "w");
    if (!log->log_file) return -1;

    if (trace_path) {
        log->trace_file = fopen(trace_path, "wb");
    }

    if (heatmap_path) {
        log->heatmap_file = fopen(heatmap_path, "w");
    }

    /* Allouer la heatmap */
    log->heatmap = calloc(HEATMAP_SIZE, sizeof(uint32_t));
    if (!log->heatmap) {
        fclose(log->log_file);
        if (log->trace_file) fclose(log->trace_file);
        if (log->heatmap_file) fclose(log->heatmap_file);
        return -1;
    }

    log->start_time_ms = (uint64_t)clock() * 1000 / CLOCKS_PER_SEC;

    /* En-tête du log */
    fprintf(log->log_file, "# PiTOP Simulation Log\n");
    fprintf(log->log_file, "# Début: %lu\n", (unsigned long)time(NULL));
    fprintf(log->log_file, "# Format: step | digit | action | map_g | map_n | x | y | dir | mode | rng\n");
    fprintf(log->log_file, "#\n");

    if (log->heatmap_file) {
        fprintf(log->heatmap_file, "# PiTOP Heatmap\n");
        fprintf(log->heatmap_file, "# Format: map_group map_num x y count\n");
    }

    return 0;
}

void logger_log_frame(Logger *log, const GameState *state,
                      int digit, Action action, uint64_t step) {
    if (!log || !log->log_file) return;

    log->actions++;

    /* Log textuel */
    fprintf(log->log_file,
            "%llu | %d | %s | %d | %d | %d | %d | %s | %d | %u\n",
            (unsigned long long)step,
            digit,
            action_name(action),
            state->pos.map_group,
            state->pos.map_num,
            state->pos.x,
            state->pos.y,
            direction_name(state->pos.dir),
            (int)state->mode,
            state->rng);

    /* Trace binaire (format compatible BinaryTraceStream) */
    if (log->trace_file) {
        uint8_t buf[10];
        buf[0] = (uint8_t)action;
        buf[1] = state->pos.map_group;
        buf[2] = state->pos.map_num;
        buf[3] = state->pos.x;
        buf[4] = state->pos.y;
        buf[5] = (uint8_t)state->pos.dir;
        buf[6] = (uint8_t)(state->rng >> 24);
        buf[7] = (uint8_t)(state->rng >> 16);
        buf[8] = (uint8_t)(state->rng >> 8);
        buf[9] = (uint8_t)(state->rng);
        fwrite(buf, 1, 10, log->trace_file);
    }
}

void logger_record_position(Logger *log, const GameState *state) {
    if (!log || !log->heatmap) return;

    /* Clé : map_group << 24 | map_num << 16 | y << 8 | x */
    uint32_t key = (state->pos.map_group << 24)
                 | (state->pos.map_num  << 16)
                 | (state->pos.y        << 8)
                 | state->pos.x;

    uint32_t idx = key % HEATMAP_SIZE;
    log->heatmap[idx]++;
}

void logger_printf(Logger *log, const char *fmt, ...) {
    if (!log || !log->log_file) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(log->log_file, fmt, args);
    va_end(args);
}

void logger_close(Logger *log) {
    if (!log) return;

    /* Écrire la heatmap */
    if (log->heatmap_file && log->heatmap) {
        for (uint32_t i = 0; i < HEATMAP_SIZE; i++) {
            if (log->heatmap[i] > 0) {
                /* Reconstituer approximativement la position */
                uint8_t x  = (uint8_t)(i & 0xFF);
                uint8_t y  = (uint8_t)((i >> 8) & 0xFF);
                uint8_t mn = (uint8_t)((i >> 16) & 0xFF);
                uint8_t mg = (uint8_t)((i >> 24) & 0xFF);
                fprintf(log->heatmap_file,
                        "%d %d %d %d %u\n",
                        mg, mn, x, y, log->heatmap[i]);
            }
        }
    }

    /* Fermer les fichiers */
    if (log->log_file) fclose(log->log_file);
    if (log->trace_file) fclose(log->trace_file);
    if (log->heatmap_file) fclose(log->heatmap_file);

    free(log->heatmap);
    log->heatmap = NULL;
}

void logger_print_summary(const Logger *log) {
    if (!log) return;

    printf("\n=== STATISTIQUES ===\n");
    printf("Actions loggées  : %llu\n", (unsigned long long)log->actions);
    printf("Changements carte: %llu\n", (unsigned long long)log->map_changes);
    printf("Combats          : %llu\n", (unsigned long long)log->battles);
    printf("Pas sur herbe    : %llu\n", (unsigned long long)log->steps_on_grass);
    printf("====================\n");
}
