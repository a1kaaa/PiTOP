package pokemonpi.simulation;
import pokemonpi.model.Action;
public interface PiProvider {
    int getDigitAt(long index);
    default Action getActionAt(long index) {
        return switch (getDigitAt(index)) {
            case 8 -> Action.UP;
            case 2 -> Action.DOWN;
            case 4 -> Action.LEFT;
            case 6 -> Action.RIGHT;
            case 0, 3, 9 -> Action.B;
            case 1, 7 -> Action.A;
            case 5 -> Action.START;
            default -> throw new IllegalArgumentException("Chiffre invalide");
        };
    }
}