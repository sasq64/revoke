#include "resol.h"
#include "revoke.h"

extern "C" API void test_start()
{

    void* t = Revoke::instance().GetType("TestClass");
    void** target = new void*[100];
    int count = Revoke::instance().GetMembers(t, target);
    printf("Found %d members\n", count);
    for(int i=0; i<count; i++) {
        const char* name = (const char*)target[i*3];
        printf("%s\n", name);
    }

    CSLua::init();
    auto TestClass = cs::Obj("TestClass");
    TestClass["StaticMethod"](33);
}

extern "C" API void unity_start()
{
    auto GameObject = cs::Obj("UnityEngine.GameObject, UnityEngine");
    auto go = GameObject["FindGameObjectWithTag"]("cube");
    auto meshRenderer =
        go["GetComponent"].type("UnityEngine.MeshRenderer, UnityEngine")();
    meshRenderer["material"]["color"] =
        cs::Class("UnityEngine.Color, UnityEngine").New(0.0f, 1.0f, 1.0f, 1.0f);
    meshRenderer["transform"]["position"] =
        cs::Class("UnityEngine.Vector3, UnityEngine").New(5.0f, 0.0f, 0.0f);
    // GameObject["CreatePrimitive"]();
}
