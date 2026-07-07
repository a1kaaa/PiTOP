package pokemonpi.extraction;
import pokemonpi.model.Transition;
import java.util.List;
public interface TraceAnalyzer {
    List<Transition> filterRedundantFrames(List<Transition> rawTrace);
    void identifyKeyDecisionPoints(List<Transition> compressedTrace);
}