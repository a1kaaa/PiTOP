package pokemonpi.simulation;


import java.math.BigInteger;
import java.util.Iterator;

public class PiGenerator {
    // Algorithme de Gibbons pour générer Pi chiffre par chiffre à l'infini
    public static Iterator<Integer> getPiStream() {
        return new Iterator<>() {
            BigInteger q = BigInteger.ONE, r = BigInteger.ZERO, t = BigInteger.ONE,
                    k = BigInteger.ONE, n = BigInteger.valueOf(3), l = BigInteger.valueOf(3);

            @Override
            public boolean hasNext() { return true; } // Infini !

            @Override
            public Integer next() {
                while (true) {
                    if (q.multiply(BigInteger.valueOf(4)).add(r).subtract(t).compareTo(n.multiply(t)) < 0) {
                        int digit = n.intValue();
                        BigInteger nr = BigInteger.valueOf(10).multiply(r.subtract(n.multiply(t)));
                        n = BigInteger.valueOf(10).multiply(BigInteger.valueOf(3).multiply(q).add(r)).divide(t).subtract(BigInteger.valueOf(10).multiply(n));
                        q = q.multiply(BigInteger.valueOf(10));
                        r = nr;
                        return digit;
                    } else {
                        BigInteger nr = q.multiply(BigInteger.valueOf(2)).add(r).multiply(l);
                        BigInteger nn = q.multiply(BigInteger.valueOf(7).multiply(k).add(BigInteger.valueOf(2))).add(r.multiply(l)).divide(t.multiply(l));
                        q = q.multiply(k);
                        t = t.multiply(l);
                        l = l.add(BigInteger.valueOf(2));
                        k = k.add(BigInteger.ONE);
                        n = nn;
                        r = nr;
                    }
                }
            }
        };
    }
}