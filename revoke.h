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

// using csptr = std::unique_ptr<void, void (*)(void*)>;
using csptr = std::shared_ptr<void>;

template <typename... ARGS> struct SharpFunc;

inline static std::map<std::string, void**>* funcList;
template <typename R, typename... ARGS> struct SharpFunc<R(ARGS...)>
{
    std::string name;
    R (*fptr)(ARGS...);
    SharpFunc(std::string const& name) : name(name)
    {
        if (funcList == nullptr)
            funcList = new std::map<std::string, void**>();
        (*funcList)[name] = (void**)&fptr;
    }

    R operator()(ARGS... args) { return fptr(args...); }

    SharpFunc& operator=(void* ptr) { fptr = (R(*)(ARGS...))ptr; }
};

static void setFn(std::string const& name, void* fptr)
{
    auto* ptr = (*funcList)[name];
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
        printf("Setup\n");

        char** names = (char**)namesPtr;
        for (int i = 0; i < count; i++) {
            printf("CALLBACK:'%s' %p\n", names[i], callbacks[i]);
            setFn(names[i], callbacks[i]);
        }
    }
};

static Revoke revokeInstance;

extern "C" API void revoke_callbacks(int count, void** callbacks, void** names)
{
    printf("%d callbacks\n", count);
    revokeInstance.setup(count, callbacks, names);
}

struct CSField;

struct CSObj
{
    csptr ptr;

    static inline void* convert(float const& a)
    {
        float v = a;
        return revokeInstance.CreateFrom(FLOAT, &v);
    }

    static inline void* convert(int const& a)
    {
        int v = a;
        return revokeInstance.CreateFrom(INT, &v);
    }

    static inline void* convert(std::string const& a)
    {
        return revokeInstance.CreateFrom(STRING, (void*)a.c_str());
    }

    static inline void* convert(CSObj const& a) { return a.ptr.get(); }

    CSObj(void* p) : ptr(p, revokeInstance.Free.fptr) {}

    CSObj(csptr const& p) : ptr(p) {}

    // Create static object
    CSObj(std::string const& name)
    {
        ptr = csptr(revokeInstance.GetType(name.c_str()),
                    revokeInstance.Free.fptr);
    }

    template <typename... ARGS>
    CSObj call(std::string const& method, ARGS... args)
    {
        void* pargs[] = {convert(args)...};
        return CSObj(revokeInstance.NamedCall(method.c_str(), ptr.get(),
                                              sizeof...(args), pargs));
    }

    template <typename... ARGS> CSObj call(void* method, ARGS... args)
    {
        void* pargs[] = {convert(args)...};
        return CSObj(revokeInstance.MethodCall(method, ptr.get(),
                                               sizeof...(args), pargs));
    }

    template <typename T> void set(std::string const& name, T const& v)
    {
        revokeInstance.SetProperty(name.c_str(), ptr.get(), convert(v));
    }

    CSObj get(std::string const& name)
    {
        return CSObj(revokeInstance.GetProperty(name.c_str(), ptr.get()));
    }

    operator float()
    {
        float rc = -1;
        revokeInstance.CastTo(FLOAT, ptr.get(), &rc);
        return rc;
    }

    operator int()
    {
        int rc = -1;
        revokeInstance.CastTo(INT, ptr.get(), &rc);
        return rc;
    }

    operator std::string()
    {
        int size;
        int t = revokeInstance.GetTypeId(ptr.get(), &size);
        if (t == STRING) {
            auto target = std::make_unique<char[]>(size + 1);
            revokeInstance.CastTo(STRING, ptr.get(), target.get());
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
            revokeInstance.SetArrayValueInt(ptr, index, f);
            return *this;
        }

        const Item& operator=(const CSObj& co) const
        {
            revokeInstance.SetArrayValue(ptr, index, co.ptr.get());
            return *this;
        }

        const Item& operator=(const float& f) const
        {
            revokeInstance.SetArrayValueFloat(ptr, index, f);
            return *this;
        }

        operator int() const
        {
            return revokeInstance.GetArrayValueInt(ptr, index);
        }
        operator float() const
        {
            return revokeInstance.GetArrayValueFloat(ptr, index);
        }
    };
    CSField operator[](char const* name);
    CSField operator[](std::string const& name);
    Item operator[](int i) { return Item(ptr.get(), i); }
};

struct CSMethod
{
    csptr methodPtr;
    csptr ptr;
    CSMethod(csptr const& p, csptr const& mp) : ptr(p), methodPtr(mp) {}
    template <typename... ARGS> CSObj operator()(ARGS const&... args)
    {
        printf("%p\n", ptr.get());
        void* pargs[] = {CSObj::convert(args)...};
        return CSObj(revokeInstance.MethodCall(methodPtr.get(), ptr.get(),
                                               sizeof...(args), pargs));
    }
};

// The field 'name' of object or type 'ptr'
struct CSField
{
    std::string name;
    csptr ptr;
    CSField(csptr const& ptr, std::string const& name) : name(name), ptr(ptr) {}
    template <typename... ARGS> CSObj operator()(ARGS const&... args)
    {
        void* pargs[] = {CSObj::convert(args)...};
        return CSObj(revokeInstance.NamedCall(name.c_str(), ptr.get(),
                                              sizeof...(args), pargs));
    }

    CSField operator[](char const* name)
    {
        void* newPtr =
            revokeInstance.GetProperty(this->name.c_str(), ptr.get());
        return CSField(csptr(newPtr, revokeInstance.Free.fptr), name);
    }

    operator int()
    {
        int rc = -1;
        void* newPtr =
            revokeInstance.GetProperty(this->name.c_str(), ptr.get());
        revokeInstance.CastTo(INT, newPtr, &rc);
        return rc;
    }

    operator float()
    {
        float rc = -1;
        void* newPtr =
            revokeInstance.GetProperty(this->name.c_str(), ptr.get());
        revokeInstance.CastTo(FLOAT, newPtr, &rc);
        return rc;
    }

    template <typename T> void operator=(T const& v)
    {
        revokeInstance.SetProperty(name.c_str(), ptr.get(), CSObj::convert(v));
    }

    const char* to_charp(const char* p) { return p; }

    template <typename... ARGS> CSMethod type(ARGS const&... types)
    {
        const char* pargs[] = {to_charp(types)...};
        void* tp = revokeInstance.GetTypes(sizeof...(types), pargs);
        void* mptr = revokeInstance.ResolveMethod(name.c_str(), ptr.get(), tp);
        revokeInstance.Free(tp);
        return CSMethod(ptr, csptr(mptr, revokeInstance.Free.fptr));
    }
};

struct CSClass
{
    csptr classPtr;
    CSClass(std::string const& name)
    {
        classPtr = csptr(revokeInstance.GetType(name.c_str()),
                         revokeInstance.Free.fptr);
    }

    template <typename... ARGS> CSObj New(ARGS const&... args)
    {

        void* pargs[] = {CSObj::convert(args)...};
        return CSObj(revokeInstance.NamedCall("new", classPtr.get(),
                                              sizeof...(args), pargs));
    }
};

template <typename T> struct CSArray;

template <> struct CSArray<int>
{
    csptr ptr;

    static CSArray New(int size)
    {
        void* classPtr = revokeInstance.GetType("System.Int32[]");
        ptr = CSObj(revokeInstance.NamedCall("new", classPtr.get(), size));
    }

    const int operator[](int i) const
    {
        return revokeInstance.GetArrayValueInt(ptr.get(), i);
    }
};

template <> struct CSArray<float>
{
    csptr ptr;
    const float operator[](int i) const
    {
        return revokeInstance.GetArrayValueFloat(ptr.get(), i);
    }
};

CSField CSObj::operator[](char const* name)
{
    return CSField(ptr, name);
}
CSField CSObj::operator[](std::string const& name)
{
    return CSField(ptr, name);
}
