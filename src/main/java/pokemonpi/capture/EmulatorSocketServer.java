package pokemonpi.capture;

import pokemonpi.model.*;
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;

public class EmulatorSocketServer implements EmulatorConnector {
    private final int port;
    private ServerSocket serverSocket;
    private Socket clientSocket;
    private DataInputStream dataIn;
    private boolean isRunning = false;

    public EmulatorSocketServer(int port) {
        this.port = port;
    }

    @Override
    public boolean connect() {
        try {
            System.out.println("En attente de la connexion de l'émulateur sur le port " + port + "...");
            this.serverSocket = new ServerSocket(port);
            this.clientSocket = serverSocket.accept(); // Bloquant jusqu'à ce que le script Lua se connecte
            this.dataIn = new DataInputStream(new BufferedInputStream(clientSocket.getInputStream()));
            this.isRunning = true;
            System.out.println("Émulateur connecté avec succès ! Capture en cours...");
            return true;
        } catch (IOException e) {
            System.err.println("Échec du démarrage du serveur socket : " + e.getMessage());
            return false;
        }
    }

    /**
     * Lit la prochaine frame envoyée en streaming par l'émulateur.
     * Cette méthode bloque jusqu'à ce que l'émulateur envoie ses 10 octets.
     */
    public Transition readFrame() throws IOException {
        if (!isRunning) throw new IllegalStateException("Le serveur n'est pas connecté.");

        // Lecture stricte des 10 octets configurés au format compact
        Action action   = Action.values()[dataIn.readByte()];
        int mapId       = dataIn.readShort();
        int x           = dataIn.readByte();
        int y           = dataIn.readByte();
        Direction dir   = Direction.values()[dataIn.readByte()];
        int rng         = dataIn.readInt();

        // Reconstruction de l'objet métier Java
        GameState state = new GameState(
                new Position(x, y, mapId, dir),
                new RNGState(rng),
                null, null, null
        );

        return new Transition(state, action, null, 1);
    }

    @Override
    public boolean isRunning() {
        return isRunning && clientSocket != null && !clientSocket.isClosed();
    }

    @Override
    public void sendInput(Action action) {
        // Optionnel : Si tu veux que Java pilote l'émulateur à distance,
        // tu pourrais utiliser un DataOutputStream ici.
    }

    @Override
    public void close() throws Exception {
        isRunning = false;
        if (dataIn != null) dataIn.close();
        if (clientSocket != null) clientSocket.close();
        if (serverSocket != null) serverSocket.close();
        System.out.println("Serveur socket arrêté.");
    }
}