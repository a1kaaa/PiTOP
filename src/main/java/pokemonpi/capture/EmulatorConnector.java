package pokemonpi.capture;
import pokemonpi.model.Action;
public interface EmulatorConnector extends AutoCloseable {
    boolean connect();
    boolean isRunning();
    void sendInput(Action action);
}