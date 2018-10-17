#include "revoke.h"
#include "resol.h"


extern "C" API void test_start()
{

    CSLua::init();
    auto TestClass = cs::Obj("TestClass");
    TestClass["StaticMethod"](33);
}

extern "C" API void unity_start()
{
    auto GameObject = cs::Obj("UnityEngine.GameObject, UnityEngine");
    auto go = GameObject["FindGameObjectWithTag"]("cube");
    auto meshRenderer = go["GetComponent"].type("UnityEngine.MeshRenderer, UnityEngine")();
    meshRenderer["material"]["color"] = cs::Class("UnityEngine.Color, UnityEngine").New(0.0f, 1.0f, 1.0f, 1.0f);
    meshRenderer["transform"]["position"] = cs::Class("UnityEngine.Vector3, UnityEngine").New(5.0f, 0.0f, 0.0f);
    //GameObject["CreatePrimitive"]();

}
