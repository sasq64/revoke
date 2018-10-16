using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;


public class TestClass
{
    public static void StaticMethod(int i)
    {
        Debug.Log(i);
    }
}

public class RevokeBehaviour : MonoBehaviour {

#if (UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN)
    public delegate void test_start();
#else
    [DllImport("revoke")]
    public static extern void test_start();
#endif

    void Awake ()
    {
#if (UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN)
        revoke.Revoke.LoadLibrary(Application.streamingAssetsPath + "/bin/libwinpthread-1.dll");
        revoke.Revoke.Init(Application.streamingAssetsPath + "/bin/revoke.dll");
        revoke.Revoke.Invoke<test_start>();
#else
        revoke.Revoke.Init("");
        test_start();
#endif
    }

    private void OnDestroy()
    {
        Debug.Log("Exit");
        revoke.Revoke.Exit();
    }
}
