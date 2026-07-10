#ifndef PI_GENERATOR_H
#define PI_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>

/*
 * pi_generator.h — Générateur infini des décimales de Pi
 *
 * Implémente l'algorithme de Gibbons (spigot algorithm) pour produire
 * les décimales de π les unes après les autres. L'itérateur est infini.
 *
 * Deux modes de fonctionnement :
 *   1. GMP : utilise GNU MP pour les grands entiers (nécessite -lgmp)
 *   2. Fichier pré-calculé : lit depuis un fichier binaire (défaut)
 *
 * Le fichier pré-calculé peut être généré via la classe Java PiGenerator
 * existante dans src/main/java/pokemonpi/simulation/PiGenerator.java.
 */

/* ------------------------------------------------------------------ */
/*  PiGenerator — opaque                                              */
/* ------------------------------------------------------------------ */

typedef struct PiGenerator PiGenerator;

/* Crée un nouveau générateur. Mode fichier : path = "pi_digits.bin".
 * Si path est NULL, tente d'utiliser GMP (si disponible).
 * Retourne NULL en cas d'échec. */
PiGenerator *pi_generator_new(const char *path);

/* Retourne le chiffre décimal suivant de Pi (0-9). */
int pi_generator_next(PiGenerator *pg);

/* Réinitialise le générateur au début. */
void pi_generator_reset(PiGenerator *pg);

/* Détruit le générateur et libère la mémoire. */
void pi_generator_free(PiGenerator *pg);

/* ------------------------------------------------------------------ */
/*  Mapping décimale → Action (identique au Java Main.java)           */
/* ------------------------------------------------------------------ */

/* Traduit un chiffre 0-9 en action GBA selon le mapping du projet.
 *
 *   1 → UP      2 → DOWN    3 → LEFT    4 → RIGHT
 *   5 → A       6 → B       7 → START
 *   8 → B       9 → B       0 → A
 */
static inline int pi_digit_to_action(int digit) {
    switch (digit) {
        case 1: return 0; /* ACTION_UP */
        case 2: return 1; /* ACTION_DOWN */
        case 3: return 2; /* ACTION_LEFT */
        case 4: return 3; /* ACTION_RIGHT */
        case 5: return 4; /* ACTION_A */
        case 6: return 5; /* ACTION_B */
        case 7: return 6; /* ACTION_START */
        case 8: return 5; /* ACTION_B */
        case 9: return 5; /* ACTION_B */
        case 0: return 4; /* ACTION_A */
        default: return 0;
    }
}

#endif
