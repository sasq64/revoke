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

template <typename... ARGS> struct SharpFunc;
template <typename R, typename... ARGS> struct SharpFunc<R(ARGS...)>
{
    // std::string name;
    R (*fptr)(ARGS...);
    SharpFunc(std::string const& name) //: name(name)
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

struct MemberInfo
{
    const char* name;
    int64_t count;
    void* memberInfo;
};

template <class Tag, typename T> class strong_typedef
{
public:
    strong_typedef() : value_() {}

    explicit strong_typedef(const T& value) : value_(value) {}

    explicit strong_typedef(T&& value) noexcept(
        std::is_nothrow_move_constructible<T>::value)
        : value_(std::move(value))
    {}

    explicit operator T&() noexcept { return value_; }

    explicit operator const T&() const noexcept { return value_; }

    friend void swap(strong_typedef& a, strong_typedef& b) noexcept
    {
        using std::swap;
        swap(static_cast<T&>(a), static_cast<T&>(b));
    }

private:
    T value_;
};

//struct ObjectRef : strong_typedef<ObjectRef, void*> {};

using ObjectRef = void*;
using ObjectArrayRef = void*;
using TypeArrayRef = void*;
using TypeRef = void*;
using MethodInfoRef = void*;

struct Revoke
{
    SharpFunc<ObjectRef(ObjectArrayRef, int)> GetArrayValue{"GetArrayValue"};
    SharpFunc<void(ObjectArrayRef, int, ObjectRef)> SetArrayValue{
        "SetArrayValue"};
    SharpFunc<float(ObjectArrayRef, int)> GetArrayValueFloat{
        "GetArrayValueFloat"};
    SharpFunc<void(ObjectArrayRef, int, float)> SetArrayValueFloat{
        "SetArrayValueFloat"};
    SharpFunc<int(ObjectArrayRef, int)> GetArrayValueInt{"GetArrayValueInt"};
    SharpFunc<void(ObjectArrayRef, int, int)> SetArrayValueInt{
        "SetArrayValueInt"};

    SharpFunc<void(ObjectArrayRef, int, int, void*)> GetManyVal{"GetManyVal"};
    SharpFunc<void(ObjectArrayRef, int, int, void*)> SetManyVal{"SetManyVal"};

    SharpFunc<void(const char*, ObjectRef, ObjectRef)> SetProperty{
        "SetProperty"};
    SharpFunc<void*(const char*, ObjectRef)> GetProperty{"GetProperty"};

    SharpFunc<ObjectRef(MethodInfoRef, ObjectRef, int, void*)> MethodCall{
        "MethodCall"};
    SharpFunc<ObjectRef(const char*, ObjectRef, int, void*)> NamedCall{
        "NamedCall"};

    SharpFunc<TypeRef(const char*)> GetType{"GetType"};
    SharpFunc<TypeArrayRef(int, const char**)> GetTypes{"GetTypes"};

    SharpFunc<MethodInfoRef(const char*, ObjectRef, TypeArrayRef)>
        ResolveMethod{"ResolveMethod"};
    SharpFunc<int(ObjectRef, void*)> GetTypeId{"GetTypeID"};

    SharpFunc<int(ObjType, ObjectRef, void*)> CastTo{"CastTo"};
    SharpFunc<int(ObjectRef, MemberInfo*)> GetMembers{"GetMembers"};
    SharpFunc<ObjectRef(int, void*)> CreateFrom{"CreateFrom"};
    SharpFunc<void(ObjectRef)> Free{"Free"};

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
using csptr = std::shared_ptr<void>;

csptr make_csptr(void* p)
{
    return std::shared_ptr<void>(p, Revoke::instance().Free.fptr);
}

struct Obj
{
    csptr ptr;
    csptr classPtr;

    static inline ObjectRef convert(float const& a)
    {
        float v = a;
        return Revoke::instance().CreateFrom(FLOAT, &v);
    }

    static inline ObjectRef convert(int const& a)
    {
        int v = a;
        return Revoke::instance().CreateFrom(INT, &v);
    }

    static inline ObjectRef convert(std::string const& a)
    {
        return Revoke::instance().CreateFrom(STRING, (void*)a.c_str());
    }

    static inline void* convert(Obj const& a) { return a.ptr.get(); }

    Obj(void* p, void* classp = nullptr)
        : ptr(make_csptr(p)), classPtr(make_csptr(classp))
    {}

    Obj(csptr& p, csptr& c) : ptr(p), classPtr(c) {}

    Obj() = default;

    // Create static object
    Obj(std::string const& name)
    {
        classPtr = nullptr;
        ptr = make_csptr(Revoke::instance().GetType(name.c_str()));
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
    int get_field(const char*)
    {
        printf("In get field %p\n", ptr.get());
        return 4;
    }
    void set_field(const char*, Obj& x) {}

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
        return Field(make_csptr(newPtr), name);
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

    template <typename... ARGS> Method bind(ARGS const&... types)
    {
        const char* pargs[] = {to_charp(types)...};
        void* tp = Revoke::instance().GetTypes(sizeof...(types), pargs);
        void* mptr =
            Revoke::instance().ResolveMethod(name.c_str(), ptr.get(), tp);
        Revoke::instance().Free(tp);
        return Method(ptr, make_csptr(mptr));
    }
};

struct Class
{
    csptr classPtr;
    Class(std::string const& name)
    {
        classPtr = make_csptr(Revoke::instance().GetType(name.c_str()));
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
