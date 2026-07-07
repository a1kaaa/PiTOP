package pokemonpi.capture;
import pokemonpi.model.GameState;
public interface MemoryReader {
    GameState readCurrentState();
    int readByte(long address);
    int readWord(long address);
}