package pokemonpi;

import pokemonpi.simulation.PiGenerator;
import java.io.*;
import java.net.*;
import java.util.Iterator;

public class Main {
    public static void main(String[] args) {
        System.out.println("En attente de connexion de l'émulateur sur le port 4444...");

        try (ServerSocket serverSocket = new ServerSocket(4444);
             Socket clientSocket = serverSocket.accept();
             DataOutputStream out = new DataOutputStream(clientSocket.getOutputStream());
             DataInputStream in = new DataInputStream(clientSocket.getInputStream())) {

            System.out.println("Émulateur connecté avec succès ! Capture en cours...");

            // Initialisation de ton générateur infini de Pi
            Iterator<Integer> piIterator = PiGenerator.getPiStream();
            byte[] buffer = new byte[10];
            long frameCounter = 0;
            int dec = 1;
            long startTime = System.currentTimeMillis();
            while (true) {

                // 1. Obtenir la décimale suivante de Pi
                int piDigit = piIterator.next();

                // 2. Traduire la décimale en Action ID
                int actionId = mapDigitToAction(piDigit);

                // 3. Envoyer l'ordre au script Lua
                out.writeByte(actionId);
                out.flush();

                // 4. Attendre la réponse de la télémétrie de l'émulateur (10 octets)
                in.readFully(buffer);
                frameCounter += 12; // On avance de 12 frames à chaque décision

                // 5. Décoder la télémétrie reçue
                int recvAction = buffer[0] & 0xFF;
                int mapGroup   = buffer[1] & 0xFF;
                int mapNum     = buffer[2] & 0xFF;
                int x          = buffer[3] & 0xFF;
                int y          = buffer[4] & 0xFF;

                if (dec % 5000 == 0) {
                    long currentTime = System.currentTimeMillis();

                    double secondsElapsed = (currentTime - startTime) / 1000.0;

                    // Calcul de la vitesse : (Nombre de décimales) / (Temps en secondes)
                    double speed = (secondsElapsed > 0) ? (dec / secondsElapsed) : 0;

                    System.out.printf("[Frames: %d] Pi Digit: %d -> Action: %d | Map: %d-%d | Pos: (%d, %d) | Progress: %d | Speed: %.2f dec/s %n",
                            frameCounter, piDigit, recvAction, mapGroup, mapNum, x, y, dec, speed);
                }
                dec++;
            }


        } catch (IOException e) {
            System.err.println("Déconnexion de l'émulateur ou erreur réseau : " + e.getMessage());
        }
    }

    // Mapping des chiffres (0-9) vers les actions (0-7)
    private static int mapDigitToAction(int digit) {
        switch (digit) {
            case 1: return 0; // Up
            case 2: return 1; // Down
            case 3: return 2; // Left
            case 4: return 3; // Right
            case 5: return 4; // Bouton A (Valider)
            case 6: return 5; // Bouton B (Annuler / Courir)
            case 7: return 6; // Start (Menu)
            case 8: return 5; // Bouton B (Annuler / Courir)
            case 9: return 5; // Bouton B (Annuler / Courir)
            case 0: return 4; // Bouton A (Valider)
            default: return 0;
        }
    }
}