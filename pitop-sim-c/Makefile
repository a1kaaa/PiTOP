CC       = gcc
CFLAGS   = -Wall -Wextra -std=c99 -O2 -Iinclude
LDFLAGS  = -lm
SRCDIR   = src
INCDIR   = include
BUILDDIR = build
TARGET   = pitop-sim

# Liste de tous les fichiers source
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

# Couleurs pour les messages
GREEN  = \033[0;32m
YELLOW = \033[0;33m
RED    = \033[0;31m
NC     = \033[0m

.PHONY: all clean run help

all: $(TARGET)
	@echo -e "$(GREEN)✓ Build terminé : $(TARGET)$(NC)"

# Édition de liens
$(TARGET): $(OBJS)
	@echo -e "$(YELLOW)Édition de liens...$(NC)"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilation
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	@echo -e "  CC  $<"
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Inclusion automatique des dépendances générées par -MMD
-include $(DEPS)

clean:
	@echo -e "$(YELLOW)Nettoyage...$(NC)"
	rm -rf $(BUILDDIR) $(TARGET)
	@echo -e "$(GREEN)✓ Nettoyé$(NC)"

run: $(TARGET)
	@echo -e "$(GREEN)Lancement de la simulation...$(NC)"
	./$(TARGET) $(ARGS)

# Génération d'un fichier de décimales de Pi via le projet Java
pi_digits.bin:
	@echo -e "$(YELLOW)Génération des décimales de Pi via Java...$(NC)"
	cd .. && mvn -q exec:java -Dexec.mainClass="pokemonpi.tools.GenPiFile" \
		-Dexec.args="10000000" > pitop-sim-c/pi_digits.bin 2>/dev/null || \
		(echo -e "$(RED)Impossible de générer pi_digits.bin. Placez le fichier manuellement.$(NC)"; false)

help:
	@echo "Usage:"
	@echo "  make          Compile le simulateur"
	@echo "  make run      Lance avec les paramètres par défaut"
	@echo "  make run ARGS='-n 1000000 -l mon_run.log'  Paramètres personnalisés"
	@echo "  make clean    Supprime les fichiers compilés"
	@echo "  make pi_digits.bin  Génère les décimales via le projet Java"
	@echo ""
	@echo "Options du simulateur:"
	@echo "  -n N    Nombre d'actions (défaut: 10000000)"
	@echo "  -p F    Fichier de décimales Pi (défaut: streaming api.pi.delivery)"
	@echo "  -m F    Index des cartes CSV"
	@echo "  -l F    Fichier de log"
	@echo "  -h      Aide"
