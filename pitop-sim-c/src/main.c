#include "simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

/*
 * main.c — Point d'entrée du simulateur PiTOP
 *
 * Usage : ./pitop-sim [options]
 *
 * Options :
 *   -n N       Nombre d'actions à simuler (défaut: 10 000 000)
 *   -p FILE    Fichier des décimales de Pi pré-calculées
 *   -m FILE    Index des cartes (CSV)
 *   -l FILE    Fichier de log
 *   -h         Affiche cette aide
 */

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  -n N       Nombre d'actions Pi à simuler (défaut: 10000000)\n"
        "  -p FILE    Fichier de décimales Pi pré-calculées (défaut: pi_digits.bin)\n"
        "  -m FILE    Index des cartes CSV        (défaut: data/index.csv)\n"
        "  -l FILE    Fichier de log              (défaut: simulation.log)\n"
        "  -h         Affiche cette aide\n"
        "\n"
        "Le fichier pi_digits.bin peut être généré depuis le projet Java\n"
        "avec : java pokemonpi.tools.GenPiFile > pi_digits.bin\n",
        prog);
}

int main(int argc, char **argv) {
    SimConfig cfg = SIM_CONFIG_DEFAULT;
    int opt;

    while ((opt = getopt(argc, argv, "n:p:m:l:h")) != -1) {
        switch (opt) {
            case 'n': cfg.total_actions = strtoull(optarg, NULL, 10); break;
            case 'p': cfg.pi_file = optarg; break;
            case 'm': cfg.map_index = optarg; break;
            case 'l': cfg.log_file = optarg; break;
            case 'h':
            default:  print_usage(argv[0]); return (opt == 'h') ? 0 : 1;
        }
    }

    printf("PiTOP Simulation C — v0.1\n");
    printf("Actions : %llu\n", (unsigned long long)cfg.total_actions);
    printf("Pi file : %s\n", cfg.pi_file);
    printf("Map idx : %s\n", cfg.map_index);
    printf("Log     : %s\n\n", cfg.log_file);

    int ret = simulator_run(&cfg);

    if (ret == 0)
        printf("\nSimulation terminée avec succès.\n");
    else
        fprintf(stderr, "Simulation interrompue (code %d).\n", ret);

    return ret;
}
