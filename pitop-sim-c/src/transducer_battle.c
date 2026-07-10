#include "transducer.h"
#include "map_data.h"
#include "metatile.h"

/*
 * transducer_battle.c — Combats aléatoires
 *
 * Après chaque pas sur de l'herbe haute (MB_TALL_GRASS), le jeu
 * décide si un combat sauvage a lieu en fonction du RNG, du taux
 * de rencontre de la zone, et du pas du joueur.
 *
 * Simulation simplifiée :
 *   - Taux de rencontre fixe ~10% par pas sur l'herbe
 *   - Issue du combat basée sur le RNG : fuite, victoire, défaite
 */

#define ENCOUNTER_RATE 0x0CCCCCCD /* ~10% en Q16.32 */

GameState transducer_check_battle(GameState state) {
    /* Ignorer si on n'est pas en overworld */
    if (state.mode != MODE_OVERWORLD)
        return state;

    MapData *map = map_get(state.pos.map_group, state.pos.map_num);
    if (!map) return state;

    /* Vérifier le métatile sur lequel on se trouve */
    uint16_t tile_id = map_get_metatile(map, state.pos.x, state.pos.y);
    int tileset_id = (tile_id < 512) ? -1 : map->secondary_tileset;

    if (!metatile_triggers_battle(tileset_id, tile_id))
        return state;

    /* Avancer le RNG et tester le taux de rencontre */
    state.rng = lcg_step(state.rng);

    /* Le jeu utilise le RNG pour décider : ici un seuil à 8-12% */
    uint32_t threshold = (uint32_t)(state.rng & 0xFFFFFFFF);
    if (threshold >= ENCOUNTER_RATE * (state.rng >> 4))
        return state;  /* pas de rencontre */

    /* Combat déclenché */
    state.mode = MODE_BATTLE;
    state.total_battles++;

    /* Résolution simplifiée du combat basée sur le RNG suivant.
     * Dans le jeu réel, le joueur doit sélectionner des actions
     * via les menus. Ici on modèle le résultat par probabilité. */
    state.rng = lcg_step(state.rng);
    uint32_t outcome = state.rng % 100;

    if (outcome < 60) {
        /* Fuite réussie */
        state.mode = MODE_OVERWORLD;
    } else if (outcome < 90) {
        /* Victoire (le Pokémon adverse est K.O.) */
        state.mode = MODE_OVERWORLD;
        state.total_faints++;
    } else {
        /* Défaite : perte d'argent, retour au centre Pokémon.
         * Dans une simulation simplifiée : respawn à la position
         * de départ (Bourg-en-Vol, map 0-0, centre 7,7). */
        state.mode = MODE_OVERWORLD;
        state.total_deaths++;
        state.pos.map_group = 0;
        state.pos.map_num   = 0;
        state.pos.x = 7;
        state.pos.y = 7;
        state.pos.dir = DIR_DOWN;
    }

    return state;
}
