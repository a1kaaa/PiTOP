package pokemonpi.model;
public record Transition(GameState beforeState, Action action, GameState afterState, long frameDelta) {}