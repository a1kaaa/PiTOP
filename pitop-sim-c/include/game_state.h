#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>
#include <stdbool.h>

/*
 * game_state.h — Modèle de données de l'état du jeu
 *
 * Traduit les records Java (GameState, Position, Action, Direction, RNGState)
 * en structures C. Utilisé par le transducer pour représenter l'état courant
 * de la simulation à chaque frame.
 */

/* ------------------------------------------------------------------ */
/*  Énumérations                                                       */
/* ------------------------------------------------------------------ */

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef enum {
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_A,
    ACTION_B,
    ACTION_START
} Action;

typedef enum {
    MODE_OVERWORLD,
    MODE_DIALOG,
    MODE_MENU,
    MODE_BATTLE,
    MODE_PARTY,
    MODE_BAG,
    MODE_POKENAV,
    MODE_UNKNOWN
} GameMode;

/* ------------------------------------------------------------------ */
/*  Structures principales                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t map_group;
    uint8_t map_num;
    uint8_t x;
    uint8_t y;
    Direction dir;
} Position;

typedef struct {
    Position pos;
    uint32_t rng;
    uint8_t event_flags[1024];
    GameMode mode;
    uint32_t dialog_counter;
    bool running_shoes;
    uint64_t total_steps;
    uint64_t total_battles;
    uint64_t total_faints;
    uint64_t total_deaths;
} GameState;

/* ------------------------------------------------------------------ */
/*  Helpers inline                                                    */
/* ------------------------------------------------------------------ */

static inline uint32_t lcg_step(uint32_t seed) {
    return seed * 0x41C64E6D + 0x00003039;
}

static inline const char *action_name(Action a) {
    switch (a) {
        case ACTION_UP:     return "UP";
        case ACTION_DOWN:   return "DOWN";
        case ACTION_LEFT:   return "LEFT";
        case ACTION_RIGHT:  return "RIGHT";
        case ACTION_A:      return "A";
        case ACTION_B:      return "B";
        case ACTION_START:  return "START";
    }
    return "?";
}

static inline const char *direction_name(Direction d) {
    switch (d) {
        case DIR_UP:    return "UP";
        case DIR_DOWN:  return "DOWN";
        case DIR_LEFT:  return "LEFT";
        case DIR_RIGHT: return "RIGHT";
    }
    return "?";
}

#endif
