import java.util.HashMap;
import java.util.Map;

public class HashTableBenchmark {
    public static void main(String[] args) {
        Map<Long, Long> map = new HashMap<>();
        long startTime = System.nanoTime();
        for (int i = 0; i < 1000000; i++) {
            long key = ((long) i << 32) | i;
            map.put(key, (long)i);
        }
        for (int i = 0; i < 1000000; i++) {
            long key = ((long) i << 32) | i;
            long value = map.get(key);
            map.remove(key);
            long newKey = ((long) i << 32) | (i + 1);
            map.put(newKey, value);
        }
        long endTime = System.nanoTime();
        long duration = (endTime - startTime);
        System.out.println("Time taken to insert, retrieve, delete, and add 1 million entries: " + duration + " nanoseconds");
    }
}
