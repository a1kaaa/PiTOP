package pokemonpi.capture;
import pokemonpi.model.Action;
import pokemonpi.model.GameState;
public interface TraceRecorder {
    void startRecording(String runId);
    void recordFrame(Action action, GameState before, GameState after);
    void saveTrace();
}