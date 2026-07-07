package pokemonpi.model;
import java.util.Map;
import java.util.Set;
public record GameState(
    Position position,
    RNGState rngState,
    Set<Integer> activeEventFlags,
    Map<String, Integer> inventory,
    String partyHash
) {}