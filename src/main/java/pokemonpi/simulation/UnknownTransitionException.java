package pokemonpi.simulation;
import pokemonpi.model.Action;
import pokemonpi.model.GameState;
public class UnknownTransitionException extends Exception {
    private final GameState state;
    private final Action action;
    public UnknownTransitionException(GameState state, Action action) {
        super("Transition inconnue en (" + state.position().x() + "," + state.position().y() + ") via " + action);
        this.state = state;
        this.action = action;
    }
    public GameState getState() { return state; }
    public Action getAction() { return action; }
}