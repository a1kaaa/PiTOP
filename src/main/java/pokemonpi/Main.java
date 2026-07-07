package pokemonpi;

import pokemonpi.capture.EmulatorSocketServer;
import pokemonpi.model.Transition;

public class Main {
    public static void main(String[] args) {
        // On lance le serveur sur le port 4444
        try (EmulatorSocketServer server = new EmulatorSocketServer(4444)) {

            if (server.connect()) {
                System.out.println("Stream démarré ! Bouge dans le jeu pour voir les frames.");

                // On capture les 300 premières frames (environ 5 secondes de jeu)
                for (int i = 0; i < 300; i++) {
                    Transition frame = server.readFrame();

                    // CORRECTION : frame.action() au lieu de frame.beforeState().action()
                    System.out.printf("[Frame %d] Action: %s | Map: %d | Pos: (%d, %d) | RNG: %08X%n",
                            i,
                            frame.action(),
                            frame.beforeState().position().mapId(),
                            frame.beforeState().position().x(),
                            frame.beforeState().position().y(),
                            frame.beforeState().rngState().value()
                    );
                }
                System.out.println("Capture de test terminée avec succès !");
            }

        } catch (Exception e) {
            System.err.println("Erreur durant la capture : " + e.getMessage());
            e.printStackTrace();
        }
    }
}