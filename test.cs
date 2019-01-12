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
    public String name = "yo";

    public String GetName() {
        return name;
    }

    public int TestFn(int a, GameObject b) {
        Console.WriteLine("" + a + "," + b.name);

        return a + 3;
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
        Console.WriteLine("Done");
        return 0;
    }
}
#endif
