#ifndef PI_GENERATOR_H
#define PI_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>

/*
 * pi_generator.h — Générateur infini des décimales de Pi
 *
 * Deux modes :
 *   1. Fichier pré-calculé : pi_generator_new("pi_digits.bin")
 *   2. Streaming API      : pi_generator_new(NULL)  ← utilise api.pi.delivery
 *
 * Le mode API stream les décimales depuis le record Google Cloud
 * (100 000 000 000 000 digits) via https://api.pi.delivery/v1/pi,
 * par paquets de 1000 digits. Nécessite curl.
 */

/* ------------------------------------------------------------------ */
/*  PiGenerator — opaque                                              */
/* ------------------------------------------------------------------ */

typedef struct PiGenerator PiGenerator;

/* Crée un générateur.
 *   path != NULL : lit depuis un fichier binaire (format: entête 4 octets
 *                  uint32_t LE = nb digits, puis digits en ASCII).
 *   path == NULL : streaming via l'API pi.delivery (curl nécessaire).
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

static inline int pi_digit_to_action(int digit) {
    switch (digit) {
        case 1: return 0;
        case 2: return 1;
        case 3: return 2;
        case 4: return 3;
        case 5: return 4;
        case 6: return 5;
        case 7: return 6;
        case 8: return 5;
        case 9: return 5;
        case 0: return 4;
        default: return 0;
    }
}

#endif
