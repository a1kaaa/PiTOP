package pokemonpi.extraction;
import pokemonpi.graph.ZoneGraph;
import pokemonpi.model.Transition;
public interface GraphBuilder { void integrateTransition(ZoneGraph graph, Transition transition); }