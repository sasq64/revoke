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

public struct Vector3
{
    public float x;
    public float y;
    public float z;
}

public class Transform
{
    public Vector3 position;
    public GameObject parent;
    public GameObject[] children;
}

public class GameObject
{
    public Transform transform;
    public String name;

    public String GetName() {
        return name;
    }

    public int TestFn(int a, int b) {
        Console.WriteLine("" + a + "," + b);
        return a + b;
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
