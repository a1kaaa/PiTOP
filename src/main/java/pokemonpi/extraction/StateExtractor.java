package pokemonpi.extraction;
import pokemonpi.model.GameState;
public interface StateExtractor { GameState extractFromRawBytes(byte[] ramDump); }