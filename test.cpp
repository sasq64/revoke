//#include "resol.h"
#include "revoke.h"

extern "C" API void test_start()
{
    auto* t = Revoke::instance().GetType("TestClass");
    MemberInfo* target = new MemberInfo[100];
    int count = Revoke::instance().GetMembers(t, target);
    printf("Found %d members\n", count);
    for(int i=0; i<count; i++) {
        const char* name = target[i].name;
        printf("%s\n", name);
    }

    //CSLua::init();
    auto TestClass = cs::Obj("TestClass");
    auto field = TestClass["StaticMethod"];
    field(33);
    printf("test_start done\n");
    fflush(stdout);
}

extern "C" API void unity_start()
{
    auto GameObject = cs::Obj("UnityEngine.GameObject, UnityEngine");
    auto go = GameObject["FindGameObjectWithTag"]("cube");
    auto meshRenderer =
        go["GetComponent"].bind("UnityEngine.MeshRenderer, UnityEngine")();
    meshRenderer["material"]["color"] =
        cs::Class("UnityEngine.Color, UnityEngine").New(0.0f, 1.0f, 1.0f, 1.0f);
    meshRenderer["transform"]["position"] =
        cs::Class("UnityEngine.Vector3, UnityEngine").New(5.0f, 0.0f, 0.0f);
    // GameObject["CreatePrimitive"]();
}
