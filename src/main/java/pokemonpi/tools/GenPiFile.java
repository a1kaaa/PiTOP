package pokemonpi.tools;

import pokemonpi.simulation.PiGenerator;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Iterator;

/**
 * GenPiFile — Exporte les décimales de Pi dans un fichier binaire
 *             lisible par le simulateur C (pitop-sim).
 *
 * Usage : java pokemonpi.tools.GenPiFile <nb_digits> [output_file]
 *
 * Si output_file est omis, écrit sur stdout.
 *
 * Format du fichier :
 *   [entête: 4 octets = nombre de décimales (uint32_t, little-endian)]
 *   [décimales: N octets, chaque octet = chiffre '0'-'9']
 */
public class GenPiFile {

    public static void main(String[] args) throws IOException {
        if (args.length < 1) {
            System.err.println("Usage: java pokemonpi.tools.GenPiFile <nb_digits> [output_file]");
            System.err.println("  nb_digits   : nombre de décimales à générer");
            System.err.println("  output_file : fichier de sortie (défaut: stdout)");
            System.exit(1);
        }

        long count = Long.parseLong(args[0]);
        if (count <= 0) {
            System.err.println("Le nombre de décimales doit être > 0");
            System.exit(1);
        }

        Iterator<Integer> pi = PiGenerator.getPiStream();

        if (args.length >= 2) {
            try (FileOutputStream fos = new FileOutputStream(args[1])) {
                writePiToStream(pi, count, fos);
            }
            System.err.println("Généré : " + count + " décimales dans " + args[1]);
        } else {
            writePiToStream(pi, count, System.out);
            System.err.println("Généré : " + count + " décimales sur stdout");
        }
    }

    private static void writePiToStream(Iterator<Integer> pi, long count, java.io.OutputStream out)
            throws IOException {

        /* Entête : nombre de décimales (uint32_t LE) */
        int total = (int) Math.min(count, 0xFFFFFFFFL);
        out.write(total & 0xFF);
        out.write((total >> 8) & 0xFF);
        out.write((total >> 16) & 0xFF);
        out.write((total >> 24) & 0xFF);

        /* Décimales */
        long progress = 0;
        long lastReport = 0;
        long start = System.currentTimeMillis();

        for (long i = 0; i < total; i++) {
            int digit = pi.next();
            out.write('0' + digit);
            progress++;

            if (progress - lastReport >= 1_000_000) {
                lastReport = progress;
                long elapsed = System.currentTimeMillis() - start;
                double speed = elapsed > 0 ? (double) progress / elapsed * 1000 : 0;
                System.err.printf("Progression : %d / %d (%.1f digits/s)%n",
                        progress, total, speed);
            }
        }

        long elapsed = System.currentTimeMillis() - start;
        double speed = elapsed > 0 ? (double) total / elapsed * 1000 : 0;
        System.err.printf("Terminé : %d décimales en %.2f s (%.0f digits/s)%n",
                total, elapsed / 1000.0, speed);
    }
}
