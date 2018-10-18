#pragma once
#include "revoke.h"

#include "sol2/sol.hpp"

struct CSLua
{
    struct CSStaticCall
    {
        sol::variadic_results operator()(const sol::variadic_args va)
        {
            printf("Called with %d args\n", va.size());
            sol::variadic_results r;
            return r;
        }
    };

    struct CSCall
    {
        sol::variadic_results operator()(cs::Obj& thiz, const sol::variadic_args va)
        {
            printf("MEMBER Called with %d args\n", va.size());
            sol::variadic_results r;
            return r;
        }
    };

    static sol::state& lua()
    {
        static sol::state lua;
        return lua;
    }

    static void init()
    {
        lua().open_libraries(sol::lib::base, sol::lib::package);
        lua().new_usertype<cs::Obj>("GameObject");

        lua()["GameObject"]["static_call"] = CSStaticCall();
        lua()["GameObject"]["member_call"] = CSCall();

        lua().script("GameObject.static_call(1,2,3)\ngo = GameObject.new()\ngo:member_call(3)\n");

    }
};
