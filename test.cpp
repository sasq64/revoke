#include "revoke.h"

extern "C" API void test_start()
{
    auto TestClass = cs::Obj("TestClass");
    TestClass["StaticMethod"](13);
}
