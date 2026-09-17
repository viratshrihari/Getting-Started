import java.util.Scanner;
import java.nio.charset.StandardCharsets;

public class Main {
    public static void main(String[] args) throws Exception {
        final int op = @OP@;
        if (op == -1) { System.out.println("moo"); return; }
        if (op == 12 || op == 15 || op == 16) {
            String chant = new String(System.in.readAllBytes(), StandardCharsets.UTF_8);
            int copies = 1;
            if (op == 16) {
                int newline = chant.indexOf('\n');
                copies = Integer.parseInt(chant.substring(0, newline));
                chant = chant.substring(newline + 1);
                if (chant.endsWith("\n")) chant = chant.substring(0, chant.length() - 1);
            }
            System.out.print(chant.repeat(copies));
            if (op == 16 && copies > 0 && !chant.isEmpty()) System.out.println();
            return;
        }
        Scanner scanner = new Scanner(System.in);
        if (op == 11) {
            long value = scanner.nextLong();
            StringBuilder output = new StringBuilder();
            while (value != 1) {
                output.append(value).append(' ');
                value = value % 2 == 0 ? value / 2 : 3 * value + 1;
            }
            System.out.println(output.append(1));
            return;
        }
        if (op == 13) {
            int nohj = scanner.next().length(), john = scanner.next().length();
            System.out.println(nohj == john ? "-1" : nohj > john ? "nohj" : "john");
            return;
        }
        if (op == 14) {
            int n = scanner.nextInt();
            String longest = "";
            for (int i = 0; i < n; i++) {
                String name = scanner.next();
                if (name.length() > longest.length()) longest = name;
            }
            System.out.println(longest.length());
            System.out.println(longest);
            return;
        }
        boolean many = op == 2 || op == 4 || op == 6 || op == 8 || op == 10;
        int n = many ? scanner.nextInt() : op == 0 ? 1 : 2;
        long[] a = new long[n];
        for (int i = 0; i < n; i++) a[i] = scanner.nextLong();
        if (op == 1 || op == 2) {
            StringBuilder output = new StringBuilder();
            for (int i = n - 1; i >= 0; i--) output.append(a[i]).append(' ');
            System.out.println(output);
        } else {
            long answer = a[0];
            for (int i = 1; i < n; i++) {
                switch (op) {
                    case 3, 4 -> answer += a[i];
                    case 5, 6 -> answer %= a[i];
                    case 7, 8 -> answer = answer * a[i] % 1000000007;
                    case 9, 10 -> answer /= a[i];
                }
            }
            System.out.println(answer);
        }
    }
}
