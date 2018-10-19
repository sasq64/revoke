#pragma once
#include "revoke.h"

#include "sol2/sol.hpp"

struct CSLua
{
    struct CSStaticCall
    {
        sol::variadic_results operator()(const sol::variadic_args va)
        {
            printf("Called with %lu args\n", va.size());
            sol::variadic_results r;
            return r;
        }
    };

    struct CSConstruct
    {
        cs::Obj classObj;

        CSConstruct(void* classPtr) : classObj(classPtr) {}

        cs::Obj operator()()
        {
            printf("Constructor on %p\n", classObj.ptr.get());
            void* obj = Revoke::instance().NamedCall("new", classObj.ptr.get(),
                                                     0, nullptr);
            return cs::Obj(obj);
        }
    };

    struct CSCall
    {
        csptr method;

        CSCall(void* ptr) : method(cs::make_csptr(ptr)) {}

        sol::variadic_results operator()(cs::Obj& thiz,
                                         const sol::variadic_args va)
        {
            printf("MEMBER Called with %lu args\n", va.size());
            int rc;
            int i = 0;
            void** pargs = new void*[va.size()]; // = {convert(args)...};
            for (const auto& v : va) {
                auto t = v.get_type();
                pargs[i] = nullptr;
                switch (t) {
                case sol::type::number:
                    printf("NUMBER\n");
                    rc = (int)v;
                    pargs[i] = Revoke::instance().CreateFrom(ObjType::INT, &rc);
                    break;
                case sol::type::string:
                    pargs[i] = Revoke::instance().CreateFrom(
                        ObjType::STRING, (void*)v.get<std::string>().c_str());
                    break;
                case sol::type::table:
                    if (v.is<cs::Obj>()) {
                        cs::Obj obj = v.get<cs::Obj>();
                        pargs[i] = obj.ptr.get();
                    }
                    break;
                default:
                    break;
                }
                i++;
            }
            Revoke::instance().MethodCall(method.get(), thiz.ptr.get(),
                                          va.size(), pargs);
            sol::variadic_results r;
            return r;
        }
    };

    static sol::state& lua()
    {
        static sol::state lua;
        return lua;
    }

    enum MemberType
    {
        Constructor = 1,
        Property = 2,
        Field = 4,
        Method = 8
    };

    struct Prop
    {
        int operator()(cs::Obj*) { return 9; }
        void operator()(cs::Obj*, int x) {}

    };

    static void init()
    {
        lua().open_libraries(sol::lib::base, sol::lib::package);

        std::string className = "GameObject";
        void* t = Revoke::instance().GetType(className.c_str());
        lua().new_usertype<cs::Obj>(className, "new",
                                    sol::factories(CSConstruct(t)));

        void** target = new void*[100];
        int count = Revoke::instance().GetMembers(t, target);
        printf("Found %d members\n", count);
        for (int i = 0; i < count; i++) {
            auto* name = (const char*)target[i * 3];
            auto mt = (MemberType)(uint64_t)target[i * 3 + 1];
            auto* p = target[i * 3 + 2];

            printf("%s %u\n", name, mt);
            switch (mt) {
            case MemberType::Method:
                lua()[className][name] = CSCall(p);
                break;
            case MemberType::Field:
                //lua()[className][name] = sol::property(x, x);
                break;
            }
        }

        lua().script(R"(
        go = GameObject.new()
        go:TestFn(4, 5)
    )");
        // lua()["GameObject"]["static_call"] = CSStaticCall();
        // lua()["GameObject"]["member_call"] = CSCall();

        //        lua().script("GameObject.static_call(1,2,3)\ngo = "
        //                   "GameObject.new()\ngo:member_call(3)\n");
    }
};
