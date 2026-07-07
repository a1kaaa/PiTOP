package pokemonpi.capture;

import pokemonpi.model.*;
import java.io.*;

public class BinaryTraceStream {

    // Écriture d'une frame en binaire pur (10 octets)
    public static void writeFrame(DataOutputStream out, Action action, GameState state) throws IOException {
        out.writeByte(action.ordinal());
        out.writeShort(state.position().mapId());
        out.writeByte(state.position().x());
        out.writeByte(state.position().y());
        out.writeByte(state.position().direction().ordinal());
        out.writeInt(state.rngState().value());
    }

    // Lecture d'une frame (Reconstruction instantanée)
    public static Transition readNextFrame(DataInputStream in) throws IOException {
        Action action = Action.values()[in.readByte()];
        int mapId = in.readShort();
        int x = in.readByte();
        int y = in.readByte();
        Direction dir = Direction.values()[in.readByte()];
        int rng = in.readInt();

        GameState before = new GameState(
                new Position(x, y, mapId, dir),
                new RNGState(rng),
                null, null, null // Événements/Inventaire ignorés pour la phase 1 (Bourg-en-Vol)
        );

        return new Transition(before, action, null, 1);
    }
}