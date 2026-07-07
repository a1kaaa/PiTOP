
# PiTOP - Pi Takes Over Pokemon

**PiTOP** est une expérience empirique basée sur le théorème du singe savant. Le but est de vérifier si les décimales de $\pi$, utilisées comme un flux infini et déterministe d'inputs, peuvent s'aligner par pur hasard pour terminer *Pokémon Version Saphir (USA/Europe v1.1 / Rev 1)*.

Ce dépôt contient le pipeline de données permettant de suivre et cartographier la progression de $\pi$ frame par frame.

---

## Architecture & Fonctionnement
```
[ BizHawk / GBA ] ──( Lecture RAM )──> [ Script Lua ] ──( TCP: 4444 )──> [ Serveur Java ]
```

1. **Le Serveur Java** reçoit, décode et stocke les données télémétriques pour analyser le comportement de $\pi$ (génération de heatmaps, détection de soft-locks).
2. **Le Script Lua (BizHawk)** tourne à 60 fps, lit l'état interne de la console et envoie un paquet compact de 10 octets par socket TCP.

---

## Structure du Paquet Réseau (10 Octets)

Le script Lua pousse en continu un flux d'octets bruts structuré ainsi :
- Octet 0 : Action/Touche générée par $\pi$ (0 à 6)
- Octet 1 : Groupe de la carte actuelle
- Octet 2 : Numéro de la carte actuelle
- Octet 3 : Position X du dresseur 
- Octet 4 : Position Y du dresseur
- Octet 5 : Direction/Orientation sécurisée (0 à 3)
- Octet 6-9 : Horloge interne / État du RNG (32-bits) 

--- 
## Cartographie RAM validée (Saphir v1.1 / Rev 1)

Ces adresses physiques spécifiques ont été isolées dans l'EWRAM et l'IWRAM pour cette révision précise du jeu :

```
local x_addr   = 0x25734  -- Position Horizontale
local y_addr   = 0x25736  -- Position Verticale
local map_group = 0x2572F -- Groupe de zone
local map_num   = 0x25730 -- Numéro de salle/route
local rng_addr = 0x48C4   -- Horloge RNG (IWRAM)
```