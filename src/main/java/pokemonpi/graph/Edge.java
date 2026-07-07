package pokemonpi.graph;
import pokemonpi.model.Action;
public class Edge {
    private final Action actionRequired;
    private final Node targetNode;
    private final long weightInFrames;
    public Edge(Action actionRequired, Node targetNode, long weightInFrames) {
        this.actionRequired = actionRequired;
        this.targetNode = targetNode;
        this.weightInFrames = weightInFrames;
    }
    public Action getActionRequired() { return actionRequired; }
    public Node getTargetNode() { return targetNode; }
    public long getWeightInFrames() { return weightInFrames; }
}