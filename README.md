# Revoke

_Call C# from C++ and Lua_

### Test (Linux)

*Requires*: mono, modern g++

`make run`

### How to use in your project

* Include "revoke.h" in your C++ shared library
* Add "revoke.cs" to your C# project.
* Edit revoke.cs to DllImport the name of your DLL
* Call revoke.Revoke.Init() from C#

Now your C++ side code have access to C#;
```c++
    auto testObj = cs::Class("SomeCSClass").New("hello", 3.14f);
    testObj["position"] = cs::Obj("Vector3")["New"](1.2f, 1.3f, 1.4f);
    float x = testObj["position"]["x"];
    auto vec3 = testObj["AddComponent"].type("Vector3")("hey");
    vec3["z"] = 19.0f;

```

For reference, the corresponding C# code:
```cs
    var testObj = new SomeCSClass("hello", 3.14f);
    testObj.position = new Vector3(1.2f, 1.3f, 1.4f);
    float x = testObj.position.x;
    var vec3 = testObj.AddComponent<Vector3>("hey");
    vec3.z = 19.0f;
```

### Details

* A cs::Obj can represent a c# object instance or a c# class.
* Use `cs::Field operator[](string)` to access fields
* Fields can be treated as properties, functions or objects.
* Use `cs::Obj operator(...)` on a field to treat it as a method and call it.
* Cast a field to a basic type to read it as a property
* Assign a field to write it as a property.
* User `cs::Method type(string...)` on a field representing an unbound
  generic method to turn it into a bound method.

