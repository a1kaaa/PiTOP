#ifndef LOGGER_H
#define LOGGER_H

#include "game_state.h"
#include <stdio.h>
#include <stdint.h>

/*
 * logger.h — Journalisation et statistiques
 *
 * Enregistre la progression de la simulation : positions visitées,
 * changements de carte, événements. Produit des fichiers exploitables
 * par les modules d'analyse existants (TraceAnalyzer, ZoneExtractor).
 */

typedef struct {
    FILE *log_file;
    FILE *trace_file;
    FILE *heatmap_file;
    uint64_t frames;
    uint64_t actions;
    uint64_t map_changes;
    uint64_t battles;
    uint64_t steps_on_grass;
    uint64_t start_time_ms;
    /* Heatmap : nombre de visites par position (map_id << 16 | y << 8 | x) */
#define HEATMAP_SIZE (16 * 256 * 256)
    uint32_t *heatmap;
} Logger;

/* Ouvre les fichiers de log. Retourne -1 en cas d'erreur. */
int logger_init(Logger *log, const char *log_path, const char *trace_path,
                const char *heatmap_path);

/* Enregistre une frame complète (état après action). */
void logger_log_frame(Logger *log, const GameState *state,
                      int digit, Action action, uint64_t step);

/* Incrémente le compteur de visites pour une position. */
void logger_record_position(Logger *log, const GameState *state);

/* Écrit un message texte dans le log principal. */
void logger_printf(Logger *log, const char *fmt, ...);

/* Ferme les fichiers et écrit le résumé final. */
void logger_close(Logger *log);

/* Affiche un résumé textuel des statistiques. */
void logger_print_summary(const Logger *log);

#endif
