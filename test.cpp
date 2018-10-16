#include "revoke.h"

extern "C" API void test_start()
{
    auto TestClass = cs::Obj("TestClass");
    TestClass["StaticMethod"](33);

    auto GameObject = cs::Obj("UnityEngine.GameObject, UnityEngine");
    TestClass["StaticMethod"](1);

    auto go = GameObject["FindGameObjectWithTag"]("cube");
    TestClass["StaticMethod"](2);

    auto meshRenderer = go["GetComponent"].type("UnityEngine.MeshRenderer, UnityEngine")();
    TestClass["StaticMethod"](3);
    
    meshRenderer["material"]["color"] = cs::Class("UnityEngine.Color, UnityEngine").New(0.0f, 1.0f, 1.0f, 1.0f);
    TestClass["StaticMethod"](4);

    meshRenderer["transform"]["position"] = cs::Class("UnityEngine.Vector3, UnityEngine").New(5.0f, 0.0f, 0.0f);

    TestClass["StaticMethod"](5);
    //GameObject["CreatePrimitive"]();

}
