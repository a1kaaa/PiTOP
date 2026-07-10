#!/bin/bash
# download_pi.sh — Télécharge les décimales de Pi et les convertit au format
#                  binaire attendu par pitop-sim (entête 4 octets + ASCII).
#
# Usage :
#   ./download_pi.sh              # 1 million (test)
#   ./download_pi.sh 1m           # 1 million
#   ./download_pi.sh 1b           # 1 milliard (via archive.org)
#
# Sources :
#   - 1k, 1m  : https://pi2e.ch (direct download)
#   - 1b      : https://archive.org/details/Math_Constants

set -e

COUNT="${1:-1m}"
OUTPUT="pi_digits.bin"
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

echo "=== Téléchargement des décimales de Pi ==="

case "$COUNT" in
    1k)
        URL="https://pi2e.ch/blog/wp-content/uploads/2017/03/pi_dec_1k.txt"
        TOTAL=1000
        ;;
    1m)
        URL="https://pi2e.ch/blog/wp-content/uploads/2017/03/pi_dec_1m.txt"
        TOTAL=1000000
        ;;
    1b)
        echo "1B: téléchargement depuis archive.org (fichier compressé de ~500 Mo)..."
        echo "  Lien : https://archive.org/details/Math_Constants"
        echo "  Télécharge manuellement, extrais 'pi_dec_1b.txt' et place-le dans $TMPDIR"
        echo "  Puis relance ce script avec le chemin en argument."
        exit 1
        ;;
    *)
        echo "Usage: $0 [1k|1m|1b]"
        exit 1
        ;;
esac

echo "Source : $URL"
echo "Décimales : $TOTAL"

if [ ! -f "$TMPDIR/pi.txt" ]; then
    echo "Téléchargement..."
    if command -v wget &>/dev/null; then
        wget -q "$URL" -O "$TMPDIR/pi.txt"
    elif command -v curl &>/dev/null; then
        curl -sL "$URL" -o "$TMPDIR/pi.txt"
    else
        echo "Erreur : ni wget ni curl trouvé"
        exit 1
    fi
fi

# Compter les chiffres (ignorer le "3." du début)
RAW=$(cat "$TMPDIR/pi.txt" | tr -cd '0-9')
ACTUAL=$(echo -n "$RAW" | wc -c)
echo "Chiffres lus : $ACTUAL"

if [ "$ACTUAL" -lt "$TOTAL" ]; then
    echo "Attention : seulement $ACTUAL chiffres (attendu $TOTAL)"
fi

# Prendre les N premiers et écrire au format binaire
echo "Génération de $OUTPUT..."
python3 << EOF
import struct

with open('$TMPDIR/pi.txt', 'r') as f:
    text = f.read()

# Extraire uniquement les chiffres
digits = ''.join(c for c in text if c.isdigit())

count = min(len(digits), $TOTAL)
digits = digits[:count]

with open('$OUTPUT', 'wb') as f:
    # Entête : uint32_t LE
    f.write(struct.pack('<I', count))
    # Chiffres : 1 octet par chiffre, en ASCII
    f.write(digits.encode('ascii'))

print(f'Écrit {count} chiffres dans pi_digits.bin')
print(f'Taille du fichier : {count + 4} octets')
EOF
