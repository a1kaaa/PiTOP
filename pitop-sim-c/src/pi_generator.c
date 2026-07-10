#define _POSIX_C_SOURCE 200809L
#include "pi_generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * pi_generator.c — Générateur de décimales de Pi
 *
 * Deux modes :
 *
 * MODE FICHIER (path != NULL) :
 *   Fichier binaire avec entête 4 octets (uint32_t LE = nb digits)
 *   puis les N chiffres en ASCII. Boucle automatiquement à la fin.
 *
 * MODE API (path == NULL) :
 *   Utilise l'API REST https://api.pi.delivery/v1/pi pour streamer
 *   les 100 000 000 000 000 digits du record Google Cloud.
 *   Les digits sont récupérés par paquets de 1000 et mis en cache.
 *   Nécessite curl (appelé via popen).
 */

/* ------------------------------------------------------------------ */
/*  Constantes                                                        */
/* ------------------------------------------------------------------ */

#define API_CHUNK_SIZE 1000
#define API_URL_FMT \
    "https://api.pi.delivery/v1/pi?start=%llu&numberOfDigits=%d"

/* ------------------------------------------------------------------ */
/*  Structures internes                                               */
/* ------------------------------------------------------------------ */

typedef enum {
    PG_MODE_FILE,
    PG_MODE_API
} PiGeneratorMode;

struct PiGenerator {
    PiGeneratorMode mode;

    /* Mode fichier */
    FILE *file;
    uint32_t total_digits;
    uint32_t cursor;

    /* Mode API */
    uint64_t api_position;       /* position globale dans Pi */
    char     chunk_buffer[API_CHUNK_SIZE];
    int      chunk_len;          /* nb de digits dans le buffer */
    int      chunk_pos;          /* position dans le buffer */
    uint64_t total_fetched;

    bool is_loaded;
};

/* ------------------------------------------------------------------ */
/*  Helper : fetch un chunk depuis l'API                              */
/* ------------------------------------------------------------------ */

static int fetch_api_chunk(PiGenerator *pg) {
    char url[256];
    snprintf(url, sizeof(url), API_URL_FMT,
             (unsigned long long)pg->api_position, API_CHUNK_SIZE);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -s \"%s\" 2>/dev/null", url);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "pi_generator: popen(curl) échoué\n");
        return -1;
    }

    /* Lire la réponse JSON : {"content":"314159..."} */
    char json_buf[API_CHUNK_SIZE + 64];
    size_t n = fread(json_buf, 1, sizeof(json_buf) - 1, fp);
    int exit_code = pclose(fp);

    if (exit_code != 0 || n == 0) {
        fprintf(stderr, "pi_generator: curl a échoué (code %d)\n", exit_code);
        return -1;
    }

    json_buf[n] = '\0';

    /* Chercher le début des chiffres après "content":" */
    char *start = strstr(json_buf, "\"content\":\"");
    if (!start) {
        fprintf(stderr, "pi_generator: réponse API invalide\n");
        return -1;
    }
    start += 11; /* saute "content":" */

    /* Copier les chiffres jusqu'au guillemet fermant */
    int count = 0;
    while (*start && *start != '"' && count < API_CHUNK_SIZE) {
        if (*start >= '0' && *start <= '9') {
            pg->chunk_buffer[count++] = *start;
        }
        start++;
    }

    pg->chunk_len = count;
    pg->chunk_pos = 0;
    pg->api_position += count;
    pg->total_fetched += count;

    return count;
}

/* ------------------------------------------------------------------ */
/*  Constructeur / Destructeur                                        */
/* ------------------------------------------------------------------ */

PiGenerator *pi_generator_new(const char *path) {
    PiGenerator *pg = calloc(1, sizeof(PiGenerator));
    if (!pg) return NULL;

    if (path) {
        /* MODE FICHIER */
        pg->file = fopen(path, "rb");
        if (!pg->file) {
            fprintf(stderr, "pi_generator: impossible d'ouvrir %s : %s\n",
                    path, strerror(errno));
            free(pg);
            return NULL;
        }

        uint32_t total = 0;
        if (fread(&total, sizeof(uint32_t), 1, pg->file) != 1) {
            fprintf(stderr, "pi_generator: format invalide (%s)\n", path);
            fclose(pg->file);
            free(pg);
            return NULL;
        }

        pg->mode = PG_MODE_FILE;
        pg->total_digits = total;
        pg->cursor = 0;
        pg->is_loaded = true;

        printf("PiGenerator: %u décimales chargées depuis %s\n", total, path);

    } else {
        /* MODE API */
        pg->mode = PG_MODE_API;
        pg->api_position = 0;
        pg->chunk_len = 0;
        pg->chunk_pos = 0;
        pg->total_fetched = 0;
        pg->is_loaded = true;

        /* Vérifier que curl est disponible */
        FILE *test = popen("curl --version 2>/dev/null", "r");
        if (!test) {
            fprintf(stderr, "pi_generator: curl n'est pas disponible\n");
            free(pg);
            return NULL;
        }
        pclose(test);

        printf("PiGenerator: streaming depuis api.pi.delivery\n");
    }

    return pg;
}

/* ------------------------------------------------------------------ */
/*  next / reset / free                                               */
/* ------------------------------------------------------------------ */

int pi_generator_next(PiGenerator *pg) {
    if (!pg || !pg->is_loaded) return 0;

    switch (pg->mode) {

    case PG_MODE_FILE: {
        if (pg->cursor >= pg->total_digits) {
            /* Boucler */
            fseek(pg->file, 4, SEEK_SET);
            pg->cursor = 0;
        }
        int digit = fgetc(pg->file);
        if (digit == EOF) {
            fseek(pg->file, 4, SEEK_SET);
            pg->cursor = 0;
            digit = fgetc(pg->file);
        }
        pg->cursor++;
        return digit - '0';
    }

    case PG_MODE_API: {
        if (pg->chunk_pos >= pg->chunk_len) {
            /* Épuisé → fetch le prochain chunk */
            int n = fetch_api_chunk(pg);
            if (n <= 0) {
                fprintf(stderr, "pi_generator: erreur API à la position %llu\n",
                        (unsigned long long)pg->api_position);
                return 0;
            }
        }
        int digit = pg->chunk_buffer[pg->chunk_pos++] - '0';
        return digit;
    }}

    return 0;
}

void pi_generator_reset(PiGenerator *pg) {
    if (!pg) return;

    switch (pg->mode) {
    case PG_MODE_FILE:
        if (pg->file) {
            fseek(pg->file, 4, SEEK_SET);
            pg->cursor = 0;
        }
        break;
    case PG_MODE_API:
        pg->api_position = 0;
        pg->chunk_len = 0;
        pg->chunk_pos = 0;
        pg->total_fetched = 0;
        break;
    }
}

void pi_generator_free(PiGenerator *pg) {
    if (pg) {
        if (pg->file) fclose(pg->file);
        free(pg);
    }
}
