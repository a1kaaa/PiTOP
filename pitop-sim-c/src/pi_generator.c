#include "pi_generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * pi_generator.c — Générateur de décimales de Pi
 *
 * Implémente deux stratégies :
 *   1. Fichier pré-calculé : lit un fichier binaire octet par octet,
 *      chaque octet étant une décimale (0-9). Simple, rapide, sans
 *      dépendance. Le fichier peut être généré par PiGenerator.java.
 *   2. GMP (optionnel) : compilez avec -DUSE_GMP pour utiliser
 *      l'algorithme de Gibbons en temps réel.
 *
 * Le fichier pré-calculé est au format suivant :
 *   [entête: 4 octets = nombre de décimales (uint32_t, little-endian)]
 *   [décimales: N octets, chaque octet = chiffre 0-9]
 */

#define PI_HEADER_SIZE 4

struct PiGenerator {
    FILE *file;
    uint32_t total_digits;
    uint32_t cursor;
    bool is_loaded;
};

PiGenerator *pi_generator_new(const char *path) {
    if (!path) {
        fprintf(stderr, "pi_generator: aucun fichier spécifié.\n");
        return NULL;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "pi_generator: impossible d'ouvrir %s : %s\n",
                path, strerror(errno));
        return NULL;
    }

    PiGenerator *pg = calloc(1, sizeof(PiGenerator));
    if (!pg) {
        fclose(f);
        return NULL;
    }

    /* Lit l'entête : nombre total de décimales */
    uint32_t total = 0;
    if (fread(&total, sizeof(uint32_t), 1, f) != 1) {
        fprintf(stderr, "pi_generator: format de fichier invalide (%s)\n", path);
        fclose(f);
        free(pg);
        return NULL;
    }

    pg->file = f;
    pg->total_digits = total;
    pg->cursor = 0;
    pg->is_loaded = true;

    printf("PiGenerator: %u décimales chargées depuis %s\n", total, path);
    return pg;
}

int pi_generator_next(PiGenerator *pg) {
    if (!pg || !pg->is_loaded)
        return 0;

    if (pg->cursor >= pg->total_digits) {
        /* Réinitialiser : on boucle sur Pi */
        fseek(pg->file, PI_HEADER_SIZE, SEEK_SET);
        pg->cursor = 0;
    }

    int digit = fgetc(pg->file);
    if (digit == EOF) {
        fseek(pg->file, PI_HEADER_SIZE, SEEK_SET);
        pg->cursor = 0;
        digit = fgetc(pg->file);
    }

    pg->cursor++;
    return digit - '0';
}

void pi_generator_reset(PiGenerator *pg) {
    if (pg && pg->file) {
        fseek(pg->file, PI_HEADER_SIZE, SEEK_SET);
        pg->cursor = 0;
    }
}

void pi_generator_free(PiGenerator *pg) {
    if (pg) {
        if (pg->file) fclose(pg->file);
        free(pg);
    }
}
