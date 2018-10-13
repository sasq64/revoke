# Revoke

_Call C# from C++_

How to use

* Include "revoke.h" in your C++ shared library
* Add "revoke.cs" to your C# project.
* Edit revoke.cs to DllImport the name of your DLL
* Call revoke.Revoke.Init() from C#

Now your C++ side code have access to C#;
```c++
    auto testObj = CSClass("SomeCSClass").New("hello", 3.14f);
    testObj["position"] = CSClass("Vector3").New(1.2f, 1.3f, 1.4f);
    float x = testObj["position"]["x"];
    auto vec3 = testObj["AddComponent"].type("Vector3")("hey");
    vec3["z"] = 19.0f;

```

For reference, the corresponding C# code:
```c#
    var testObj = new SomeCSClass("hello", 3.14f);
    testObj.position = new Vector3(1.2f, 1.3f, 1.4f);
    float x = testObj.position.x;
    var vec3 = testObj.AddComponent<Vector3>("hey");
    vec3.z = 19.0f;
```

