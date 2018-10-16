#if (!UNITY_STANDALONE && !UNITY_EDITOR)
using System;
using System.Runtime.InteropServices;

public class TestClass
{
    public static void StaticMethod(int i)
    {
        Console.WriteLine("Called with " + i);
    }
}

public class TestMain
{
    [DllImport("revoke")]
    public static extern void test_start();

    static int Main(string[] args)
    {
        revoke.Revoke.Init("");
        test_start();
        return 0;
    }
}
#endif
