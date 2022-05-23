#include <doctest/doctest.h>
#include <memory>

#include <lua-cts.hpp>

const int SOME_MAGIC_NUMBER = 134;

extern "C" {
static int some_function(lua_State*)
{
    return SOME_MAGIC_NUMBER;
}
}

TEST_CASE("stack")
{
    auto mock_state = std::unique_ptr<lua_State, decltype(&lua_close)>(luaL_newstate(), lua_close);

    DOCTEST_SUBCASE("Empty stack is empty")
    {
        REQUIRE(lua::StackWrapper<>(mock_state.get()).stack_size == 0);
    }

    DOCTEST_SUBCASE("Runtime stack checking")
    {
        (void)lua::StackWrapper<>(mock_state.get()).pushinteger(1);
        REQUIRE_THROWS(lua::StackWrapper<>(mock_state.get()));
        REQUIRE_THROWS(lua::StackWrapper<lua::Nil>(mock_state.get()));
        (void)lua::StackWrapper<lua::Number>(mock_state.get());
    }

    DOCTEST_SUBCASE("Pushing values")
    {
        auto s = lua::StackWrapper<>(mock_state.get()).pushinteger(1);
        REQUIRE(s.stack_size == 1);
        (void) s.tointeger<1>([] (int x) {REQUIRE(x == 1);} );
        (void) s.tointeger<-1>([] (int x) {REQUIRE(x == 1);} );
        (void) s.pushcfunction(some_function).tocfunction<-1>([] (int (*x)(lua_State*)) { REQUIRE(x(nullptr) == SOME_MAGIC_NUMBER); });
    }

    DOCTEST_SUBCASE("Querying type")
    {
        auto s = lua::StackWrapper<>(mock_state.get());

        DOCTEST_SUBCASE("number")
        {
            (void) s.pushinteger(1).type<1>([] (int type) {REQUIRE(type == LUA_TNUMBER);});
        }

        DOCTEST_SUBCASE("nil")
        {
            (void) s.pushnil().type<1>([] (int type) {REQUIRE(type == LUA_TNIL);});
        }

        DOCTEST_SUBCASE("function")
        {
            (void) s.pushcfunction(some_function).type<1>([] (int type) {REQUIRE(type == LUA_TFUNCTION);});
        }
    }

    DOCTEST_SUBCASE("Popping elements")
    {
        auto s = lua::StackWrapper<>(mock_state.get()).pushinteger(1);
        REQUIRE(s.stack_size == 1);

        DOCTEST_SUBCASE("Popping one")
        {
            REQUIRE(s.pop<1>().stack_size == 0);
        }

        DOCTEST_SUBCASE("Popping two")
        {
            REQUIRE(s.pushinteger(1).pop<2>().stack_size == 0);
        }
    }
}
