#pragma once
#include "revoke.h"

#include "sol2/sol.hpp"

struct CSLua
{
    struct CSCall
    {
        sol::variadic_results operator()(const sol::variadic_args va)
        {
            printf("Called with %d args\n", va.size());
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

        lua()["test_call"] = sol::as_function(CSCall());

        lua().script("test_call(1,2,3)\n");

    }
};
