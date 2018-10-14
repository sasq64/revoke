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

    [DllImport("revoke")]
    public static extern void test_start();
	// Use this for initialization
	void Start () {
        revoke.Revoke.Init();
        test_start();
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}
