from pathlib import Path
import csv

INPUT_DIR = Path("map_csv")
OUTPUT_DIR = Path("map_txt")

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

for csv_file in INPUT_DIR.rglob("*.csv"):
    with open(csv_file, newline="", encoding="utf-8") as f:
        reader = csv.reader(f)

        # Première ligne : métadonnées
        metadata = next(reader)

        meta = {}
        for i in range(0, len(metadata), 2):
            if i + 1 < len(metadata):
                meta[metadata[i].strip()] = metadata[i + 1].strip()

        map_name = meta.get("map", csv_file.stem)
        width = meta.get("width", "?")
        height = meta.get("height", "?")

        # Ignore la ligne des coordonnées
        next(reader)

        rows = []

        for row in reader:
            # Ignore la première colonne (indice de ligne)
            values = row[1:]

            converted = []
            for value in values:
                value = value.strip()

                if not value:
                    continue

                tile = int(value, 16) & 0x03FF
                converted.append(str(tile))

            rows.append(" ".join(converted))

    output_file = OUTPUT_DIR / (csv_file.stem.replace("map_grid_", "").replace("_Layout", "") + ".txt")

    with open(output_file, "w", encoding="utf-8") as out:
        out.write(f"{map_name} ({width}x{height})\n")
        out.write("\n".join(rows))

    print(f"✔ {csv_file.name} -> {output_file.name}")

print("Conversion terminée.")