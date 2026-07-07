package pokemonpi.extraction;
import pokemonpi.graph.ZoneGraph;
import pokemonpi.model.Transition;
import java.util.List;
public interface ZoneExtractor { ZoneGraph buildZoneFromTransitions(int mapId, List<Transition> transitions); }