package pokemonpi.graph;
import pokemonpi.model.GameState;
public class Node {
    private final String id;
    private final GameState state;
    public Node(String id, GameState state) { this.id = id; this.state = state; }
    public String getId() { return id; }
    public GameState getState() { return state; }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof Node node)) return false;
        return id.equals(node.id);
    }
    @Override
    public int hashCode() { return id.hashCode(); }
}