#include <doctest/doctest.h>
#include <memory>

#include <lua-cts.hpp>

struct lua_State {
};

TEST_CASE("stack")
{
    auto mock_state = std::make_unique<lua_State>();

    DOCTEST_SUBCASE("Empty stack is empty")
    {
        REQUIRE(lua::StackWrapper<>(mock_state.get()).stack_size == 0);
    }
}
