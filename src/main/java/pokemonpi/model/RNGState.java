package pokemonpi.model;
public record RNGState(int value) {
    public RNGState next() {
        int nextValue = (value * 0x41C64E6D) + 0x00003039;
        return new RNGState(nextValue);
    }
}