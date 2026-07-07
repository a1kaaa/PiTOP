package pokemonpi.simulation;
import pokemonpi.model.Action;
import pokemonpi.model.GameState;
public interface Transducer { GameState transition(GameState currentState, Action action) throws UnknownTransitionException; }