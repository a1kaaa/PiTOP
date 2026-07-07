package pokemonpi.simulation;
import pokemonpi.model.Action;
import pokemonpi.model.GameState;
public class Simulator {
    private final Transducer transducer;
    private final PiProvider piProvider;
    public Simulator(Transducer transducer, PiProvider piProvider) {
        this.transducer = transducer;
        this.piProvider = piProvider;
    }
    public void execute(GameState initialState, long totalDigits) {
        GameState currentState = initialState;
        long step = 0;
        try {
            for (step = 0; step < totalDigits; step++) {
                Action action = piProvider.getActionAt(step);
                currentState = transducer.transition(currentState, action);
            }
            System.out.println("Simulation terminée avec succès !");
        } catch (UnknownTransitionException e) {
            System.err.printf("[STOP] Transition manquante à la décimale %d (Action: %s)%n", step, e.getAction());
        }
    }
}