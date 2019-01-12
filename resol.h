#pragma once
#include "revoke.h"

#include "sol2/sol.hpp"

#include <map>
#include <string>

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
        cs::csptr classObj;

        struct Member
        {
            Member(std::string const& name, cs::csptr mi, int typ)
                : name(name), memberInfo(mi), type(typ)
            {}
            std::string name;
            cs::csptr memberInfo;
            int type;
        };


        std::map<std::string, Member>
            memberMap;

        CSConstruct(void* classPtr) : classObj(classPtr)
        {
#if 0
            void** target = new void*[100];
            int count = Revoke::instance().GetMembers(classPtr, target);
            printf("Found %d members\n", count);
            for (int i = 0; i < count; i++) {
                auto* name = (const char*)target[i * 3];
                auto mt = (MemberType)(uint64_t)target[i * 3 + 1];
                auto* p = target[i * 3 + 2];

                printf("%s %u\n", name, mt);
                Member member(name, cs::make_csptr(p), mt);
                memberMap.try_emplace(name, member);
            }
#endif
        }

        cs::Obj operator()()
        {
            printf("Constructor on %p\n", classObj.get());
            void* obj = Revoke::instance().NamedCall("new", classObj.get(),
                                                     0, nullptr);
            return cs::Obj(cs::make_csptr(obj), classObj);
        }
    };

    struct CSCall
    {
        cs::csptr method;

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
                printf("TYPE %d\n", (int)t);
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
                case sol::type::userdata:
                    printf("TABLE\n");
                    if (v.is<cs::Obj>()) {
                        printf("OBJ\n");
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

    static void my_setter(const cs::Obj& obj, const char*, sol::object,
                          sol::this_state s)
    {
        printf("In setter\n");
    }

    static sol::object my_getter(const cs::Obj& obj, std::string const& name,
                                 sol::this_state s)
    {
        printf("In getter\n");

        Revoke::instance().GetMember();

        if (name == "TestFn") {
            return sol::make_object(
                s, [](cs::Obj& z, int x, cs::Obj& y) -> int { return 3; });
        }
        return sol::make_object(s, 3);
    }

    static void init()
    {
        lua().open_libraries(sol::lib::base, sol::lib::package);

        std::string className = "GameObject";
        void* t = Revoke::instance().GetType(className.c_str());
        lua().new_usertype<cs::Obj>(
            className, "new", sol::factories(CSConstruct(t)),
            sol::meta_function::index, &my_getter,
            sol::meta_function::new_index, &my_setter
            //,sol::meta_function::new_index, &cs::Obj::set_field
        );

        lua()["println"] = &puts;

        printf("Running script\n");

        lua().script(R"(
        go = GameObject.new()
        go:TestFn(4, go)
        println("Test called")
        y = go.transform
        print(y)
        println("here")
    )");
        // lua()["GameObject"]["static_call"] = CSStaticCall();
        // lua()["GameObject"]["member_call"] = CSCall();

        //        lua().script("GameObject.static_call(1,2,3)\ngo = "
        //                   "GameObject.new()\ngo:member_call(3)\n");
    }
};
