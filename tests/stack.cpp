#include <doctest/doctest.h>
#include <memory>

#include <lua-cts.hpp>

TEST_CASE("stack")
{
    auto mock_state = std::unique_ptr<lua_State, decltype(&lua_close)>(luaL_newstate(), lua_close);

    DOCTEST_SUBCASE("Empty stack is empty")
    {
        REQUIRE(lua::StackWrapper<>(mock_state.get()).stack_size == 0);
    }

    DOCTEST_SUBCASE("Creating a stack with one int")
    {
        REQUIRE(lua::StackWrapper<lua::Int>(mock_state.get()).stack_size == 1);
    }

    DOCTEST_SUBCASE("Pushing values")
    {
        auto s = lua::StackWrapper<>(mock_state.get()).pushinteger(1);
        REQUIRE(s.stack_size == 1);
        (void) s.tointeger<1>([] (int x) {REQUIRE(x == 1);} );
    }
}
