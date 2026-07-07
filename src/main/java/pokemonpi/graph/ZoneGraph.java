package pokemonpi.graph;
import pokemonpi.model.Action;
import java.util.*;
public class ZoneGraph {
    private final int mapId;
    private final Map<String, Node> nodes = new HashMap<>();
    private final Map<String, List<Edge>> adjacencyList = new HashMap<>();
    public ZoneGraph(int mapId) { this.mapId = mapId; }
    public int getMapId() { return mapId; }
    public void addNode(Node node) {
        nodes.putIfAbsent(node.getId(), node);
        adjacencyList.putIfAbsent(node.getId(), new ArrayList<>());
    }
    public void addTransition(String sourceId, Action action, String targetId, long frames) {
        Node target = nodes.get(targetId);
        if (target != null && adjacencyList.containsKey(sourceId)) {
            adjacencyList.get(sourceId).add(new Edge(action, target, frames));
        }
    }
    public Optional<Edge> getTransition(String sourceId, Action action) {
        return adjacencyList.getOrDefault(sourceId, Collections.emptyList()).stream().filter(e -> e.getActionRequired() == action).findFirst();
    }
}