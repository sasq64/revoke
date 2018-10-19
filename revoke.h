#pragma once

#ifdef _WIN32
#    define API __declspec(dllexport)
#    define STDCALL __stdcall
#else
#    define STDCALL
#    define API
#endif

#include <map>
#include <memory>
#include <stdint.h>
#include <string.h>
#include <string>

extern "C" API void StringCopy(char* dest, char* src)
{
    strcpy(dest, src);
}

enum ObjType
{
    VOID,
    INT,
    FLOAT,
    DOUBLE,
    STRING,
    OBJECT,

    BYTE_ARRAY,
    INT_ARRAY,
    FLOAT_ARRAY,
    DOUBLE_ARRAY,
    OBJECT_ARRAY
};

using csptr = std::shared_ptr<void>;



template <typename... ARGS> struct SharpFunc;

struct FuncList
{
    static std::map<std::string, void**>& get()
    {
        static std::map<std::string, void**>* funcList;
        if (funcList == nullptr)
            funcList = new std::map<std::string, void**>();
        return *funcList;
    }
};

template <typename R, typename... ARGS> struct SharpFunc<R(ARGS...)>
{
    std::string name;
    R (*fptr)(ARGS...);
    SharpFunc(std::string const& name) : name(name)
    {
        FuncList::get()[name] = (void**)&fptr;
    }

    R operator()(ARGS... args) { return fptr(args...); }

    SharpFunc& operator=(void* ptr) { fptr = (R(*)(ARGS...))ptr; }
};

static void setFn(std::string const& name, void* fptr)
{
    auto* ptr = FuncList::get()[name];
    if (ptr != nullptr)
        *ptr = fptr;
}

struct Revoke
{
    SharpFunc<void*(int, const char**)> GetTypes{"GetTypes"};
    SharpFunc<void*(const char*)> GetType{"GetType"};
    SharpFunc<void*(const char*, void*, void*)> ResolveMethod{"ResolveMethod"};
    SharpFunc<void*(void*, void*, int, void*)> MethodCall{"MethodCall"};
    SharpFunc<void*(const char*, void*, int, void*)> NamedCall{"NamedCall"};
    SharpFunc<void(const char*, void*, void*)> SetProperty{"SetProperty"};
    SharpFunc<void*(const char*, void*)> GetProperty{"GetProperty"};
    SharpFunc<int(void*, void*)> GetTypeId{"GetTypeID"};
    SharpFunc<int(void*, void*)> GetMembers{"GetMembers"};
    SharpFunc<int(int, void*, void*)> CastTo{"CastTo"};
    SharpFunc<void*(int, void*)> CreateFrom{"CreateFrom"};
    SharpFunc<void(void*)> Free{"Free"};
    SharpFunc<void(void*, int, void*)> SetArrayValue{"SetArrayValue"};
    SharpFunc<void*(void*, int, void*)> GetArrayValue{"GetArrayValue"};
    SharpFunc<void(void*, int, int, void*)> SetManyVal{"SetManyVal"};
    SharpFunc<void(void*, int, int, void*)> GetManyVal{"GetManyVal"};
    SharpFunc<void(void*, int, int)> SetArrayValueInt{"SetArrayValueInt"};
    SharpFunc<int(void*, int)> GetArrayValueInt{"GetArrayValueInt"};
    SharpFunc<void(void*, int, float)> SetArrayValueFloat{"SetArrayValueFloat"};
    SharpFunc<float(void*, int)> GetArrayValueFloat{"GetArrayValueFloat"};

    void setup(int count, void** callbacks, void** namesPtr)
    {
        char** names = (char**)namesPtr;
        for (int i = 0; i < count; i++) {
            setFn(names[i], callbacks[i]);
        }
    }

    static Revoke& instance()
    {
        static Revoke revoke;
        return revoke;
    }
};

extern "C" API void revoke_callbacks(int count, void** callbacks, void** names)
{
    Revoke::instance().setup(count, callbacks, names);
}

namespace cs {

struct Field;

csptr make_csptr(void* p) { return std::shared_ptr<void>(p, Revoke::instance().Free.fptr); }

struct Obj
{
    csptr ptr;

    static inline void* convert(float const& a)
    {
        float v = a;
        return Revoke::instance().CreateFrom(FLOAT, &v);
    }

    static inline void* convert(int const& a)
    {
        int v = a;
        return Revoke::instance().CreateFrom(INT, &v);
    }

    static inline void* convert(std::string const& a)
    {
        return Revoke::instance().CreateFrom(STRING, (void*)a.c_str());
    }

    static inline void* convert(Obj const& a) { return a.ptr.get(); }

    Obj(void* p) : ptr(make_csptr(p)) {}

    Obj(csptr const& p) : ptr(p) {}

    Obj()
    {
        printf("XXX\n");
    }

    // Create static object
    Obj(std::string const& name)
    {
        ptr = csptr(Revoke::instance().GetType(name.c_str()),
                    Revoke::instance().Free.fptr);
    }

    template <typename... ARGS>
    Obj call(std::string const& method, ARGS... args)
    {
        void* pargs[] = {convert(args)...};
        return Obj(Revoke::instance().NamedCall(method.c_str(), ptr.get(),
                                                sizeof...(args), pargs));
    }

    template <typename... ARGS> Obj call(void* method, ARGS... args)
    {
        void* pargs[] = {convert(args)...};
        return Obj(Revoke::instance().MethodCall(method, ptr.get(),
                                                 sizeof...(args), pargs));
    }

    template <typename T> void set(std::string const& name, T const& v)
    {
        Revoke::instance().SetProperty(name.c_str(), ptr.get(), convert(v));
    }

    Obj get(std::string const& name)
    {
        return Obj(Revoke::instance().GetProperty(name.c_str(), ptr.get()));
    }

    operator float()
    {
        float rc = -1;
        Revoke::instance().CastTo(FLOAT, ptr.get(), &rc);
        return rc;
    }

    operator int()
    {
        int rc = -1;
        Revoke::instance().CastTo(INT, ptr.get(), &rc);
        return rc;
    }

    operator std::string()
    {
        int size;
        int t = Revoke::instance().GetTypeId(ptr.get(), &size);
        if (t == STRING) {
            auto target = std::make_unique<char[]>(size + 1);
            Revoke::instance().CastTo(STRING, ptr.get(), target.get());
            return std::string(target.get());
        }
        return "";
    }

    struct Item
    {
        Item(void* ptr, int index) : ptr(ptr), index(index) {}
        void* ptr;
        int index;

        const Item& operator=(const int& f) const
        {
            Revoke::instance().SetArrayValueInt(ptr, index, f);
            return *this;
        }

        const Item& operator=(const Obj& co) const
        {
            Revoke::instance().SetArrayValue(ptr, index, co.ptr.get());
            return *this;
        }

        const Item& operator=(const float& f) const
        {
            Revoke::instance().SetArrayValueFloat(ptr, index, f);
            return *this;
        }

        operator int() const
        {
            return Revoke::instance().GetArrayValueInt(ptr, index);
        }
        operator float() const
        {
            return Revoke::instance().GetArrayValueFloat(ptr, index);
        }
    };
    Field operator[](char const* name);
    Field operator[](std::string const& name);
    Item operator[](int i) { return Item(ptr.get(), i); }
};

struct Method
{
    csptr methodPtr;
    csptr ptr;
    Method(csptr const& p, csptr const& mp) : ptr(p), methodPtr(mp) {}
    template <typename... ARGS> Obj operator()(ARGS const&... args)
    {
        void* pargs[] = {Obj::convert(args)...};
        return Obj(Revoke::instance().MethodCall(methodPtr.get(), ptr.get(),
                                                 sizeof...(args), pargs));
    }
};

// The field 'name' of object or type 'ptr'
struct Field
{
    std::string name;
    csptr ptr;
    Field(csptr const& ptr, std::string const& name) : name(name), ptr(ptr) {}
    template <typename... ARGS> Obj operator()(ARGS const&... args)
    {
        void* pargs[] = {Obj::convert(args)...};
        return Obj(Revoke::instance().NamedCall(name.c_str(), ptr.get(),
                                                sizeof...(args), pargs));
    }

    Field operator[](char const* name)
    {
        void* newPtr =
            Revoke::instance().GetProperty(this->name.c_str(), ptr.get());
        return Field(csptr(newPtr, Revoke::instance().Free.fptr), name);
    }

    operator int()
    {
        int rc = -1;
        void* newPtr =
            Revoke::instance().GetProperty(this->name.c_str(), ptr.get());
        Revoke::instance().CastTo(INT, newPtr, &rc);
        return rc;
    }

    operator float()
    {
        float rc = -1;
        void* newPtr =
            Revoke::instance().GetProperty(this->name.c_str(), ptr.get());
        Revoke::instance().CastTo(FLOAT, newPtr, &rc);
        return rc;
    }

    template <typename T> void operator=(T const& v)
    {
        Revoke::instance().SetProperty(name.c_str(), ptr.get(),
                                       Obj::convert(v));
    }

    const char* to_charp(const char* p) { return p; }

    template <typename... ARGS> Method type(ARGS const&... types)
    {
        const char* pargs[] = {to_charp(types)...};
        void* tp = Revoke::instance().GetTypes(sizeof...(types), pargs);
        void* mptr =
            Revoke::instance().ResolveMethod(name.c_str(), ptr.get(), tp);
        Revoke::instance().Free(tp);
        return Method(ptr, csptr(mptr, Revoke::instance().Free.fptr));
    }
};

struct Class
{
    csptr classPtr;
    Class(std::string const& name)
    {
        classPtr = csptr(Revoke::instance().GetType(name.c_str()),
                         Revoke::instance().Free.fptr);
    }

    template <typename... ARGS> Obj New(ARGS const&... args)
    {

        void* pargs[] = {Obj::convert(args)...};
        return Obj(Revoke::instance().NamedCall("new", classPtr.get(),
                                                sizeof...(args), pargs));
    }
};

template <typename T> struct Array;

template <> struct Array<int>
{
    csptr ptr;

    Array(void* ptr) : ptr(ptr, Revoke::instance().Free.fptr) {}

    static Array New(int size)
    {
        void* classPtr = Revoke::instance().GetType("System.Int32[]");
        return Array(Revoke::instance().NamedCall("new", classPtr, 1, &size));
    }

    const int operator[](int i) const
    {
        return Revoke::instance().GetArrayValueInt(ptr.get(), i);
    }
};

template <> struct Array<float>
{
    csptr ptr;
    const float operator[](int i) const
    {
        return Revoke::instance().GetArrayValueFloat(ptr.get(), i);
    }
};

Field Obj::operator[](char const* name)
{
    return Field(ptr, name);
}
Field Obj::operator[](std::string const& name)
{
    return Field(ptr, name);
}

} // namespace cs
