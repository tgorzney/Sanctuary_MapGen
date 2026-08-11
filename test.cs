using System;
public class Test {
    public static void Main() {
        double dx = 1.0;
        double dy = 0.0;
        double invSqrt = 1.0 / Math.Sqrt(dx*dx + dy*dy + 1.0);
        double acos = Math.Acos(invSqrt);
        Console.WriteLine(acos * 57.2957795131);
    }
}
