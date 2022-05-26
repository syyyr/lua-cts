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

static_assert(std::is_same_v<lua::StackWrapper<lua::Number>, lua::pop_front_t<lua::StackWrapper<lua::Number>, 0>>);
static_assert(std::is_same_v<lua::StackWrapper<>, lua::pop_front_t<lua::StackWrapper<lua::Number>, 1>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil>, lua::pop_front_t<lua::StackWrapper<lua::Number, lua::Nil>, 1>>);

static_assert(std::is_same_v<lua::StackWrapper<>, lua::concat_types_t<lua::StackWrapper<>, lua::StackWrapper<>>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil>, lua::concat_types_t<lua::StackWrapper<lua::Nil>, lua::StackWrapper<>>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil>, lua::concat_types_t<lua::StackWrapper<>, lua::StackWrapper<lua::Nil>>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil, lua::Number>, lua::concat_types_t<lua::StackWrapper<>, lua::StackWrapper<lua::Nil, lua::Number>>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil, lua::Number>, lua::concat_types_t<lua::StackWrapper<lua::Nil>, lua::StackWrapper<lua::Number>>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Number, lua::Nil>, lua::concat_types_t<lua::StackWrapper<lua::Number>, lua::StackWrapper<lua::Nil>>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Number, lua::Nil, lua::Number>, lua::concat_types_t<lua::StackWrapper<lua::Number, lua::Nil>, lua::StackWrapper<lua::Number>>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Number, lua::Nil, lua::Number>, lua::concat_types_t<lua::StackWrapper<lua::Number>, lua::StackWrapper<lua::Nil, lua::Number>>>);

static_assert(std::is_same_v<lua::StackWrapper<lua::Number>, lua::replace_type_t<lua::StackWrapper<lua::Nil>, 1, lua::Number>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Number, lua::Number>, lua::replace_type_t<lua::StackWrapper<lua::Number, lua::Nil>, 2, lua::Number>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Number, lua::Nil, lua::Nil>, lua::replace_type_t<lua::StackWrapper<lua::Nil, lua::Nil, lua::Nil>, 1, lua::Number>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil, lua::Number, lua::Nil>, lua::replace_type_t<lua::StackWrapper<lua::Nil, lua::Nil, lua::Nil>, 2, lua::Number>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil, lua::Nil, lua::Number>, lua::replace_type_t<lua::StackWrapper<lua::Nil, lua::Nil, lua::Nil>, 3, lua::Number>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil, lua::Nil, lua::Number>, lua::replace_type_t<lua::StackWrapper<lua::Nil, lua::Nil, lua::Nil>, -1, lua::Number>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil, lua::Number, lua::Nil>, lua::replace_type_t<lua::StackWrapper<lua::Nil, lua::Nil, lua::Nil>, -2, lua::Number>>);
static_assert(std::is_same_v<lua::StackWrapper<lua::Nil, lua::Number, lua::Nil>, lua::replace_type_t<lua::StackWrapper<lua::Nil, lua::Number, lua::Nil>, -2, lua::Number>>);

#define REQUIRE_STACK(toCheck, ...) static_assert(std::is_same_v<decltype(toCheck), lua::StackWrapper<__VA_ARGS__>>)

TEST_CASE("stack")
{
    auto mock_state = std::unique_ptr<lua_State, decltype(&lua_close)>(luaL_newstate(), lua_close);

    DOCTEST_SUBCASE("Empty stack is empty")
    {
        REQUIRE(lua::StackWrapper<>(mock_state.get()).stack_size == 0);
        REQUIRE_STACK(lua::StackWrapper<>(mock_state.get()),);
    }

    DOCTEST_SUBCASE("Runtime stack checking")
    {
        (void)lua::StackWrapper<>(mock_state.get()).pushinteger(1);
        REQUIRE_THROWS(lua::StackWrapper<>(mock_state.get()));
        REQUIRE_THROWS(lua::StackWrapper<lua::Nil>(mock_state.get()));
        (void)lua::StackWrapper<lua::Number>(mock_state.get());
    }

    DOCTEST_SUBCASE("Unknown types")
    {
        DOCTEST_SUBCASE("Initializing")
        {
            (void)lua::StackWrapper<>(mock_state.get()).pushinteger(1);
            (void)lua::StackWrapper<lua::Unknown>(mock_state.get());
            REQUIRE_THROWS(lua::StackWrapper<lua::Unknown, lua::Unknown>(mock_state.get()));
        }

        DOCTEST_SUBCASE("Asserting types")
        {
            (void)lua::StackWrapper<>(mock_state.get()).pushinteger(1);
            auto s = lua::StackWrapper<lua::Unknown>(mock_state.get());
            REQUIRE_STACK(s, lua::Unknown);
            auto s2 = s.tointeger<-1>([] (int x) { REQUIRE(x == 1); } );
            REQUIRE_STACK(s2, lua::Number);
        }
    }

    DOCTEST_SUBCASE("Pushing values")
    {
        auto s = lua::StackWrapper<>(mock_state.get());
        DOCTEST_SUBCASE("Integer")
        {
            auto s2 = s.pushinteger(1);
            REQUIRE_STACK(s2, lua::Number);
            (void) s2.tointeger<1>([] (int x) {REQUIRE(x == 1);} );
            (void) s2.tointeger<-1>([] (int x) {REQUIRE(x == 1);} );
        }

        DOCTEST_SUBCASE("C Function")
        {
            (void) s.pushcfunction(some_function).tocfunction<-1>([] (int (*x)(lua_State*)) { REQUIRE(x(nullptr) == SOME_MAGIC_NUMBER); });
        }

        DOCTEST_SUBCASE("Table")
        {
            auto s2 = s.newtable().type<-1>([] (int type) { REQUIRE(type == LUA_TTABLE); });
            REQUIRE_STACK(s2, lua::Table);
        }

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
        REQUIRE_STACK(s, lua::Number);

        DOCTEST_SUBCASE("Popping one")
        {
            REQUIRE_STACK(s.pop<1>(),);
        }

        DOCTEST_SUBCASE("Popping two")
        {
            REQUIRE_STACK(s.pushinteger(1).pop<2>(),);
        }
    }

    DOCTEST_SUBCASE("Table manipulation")
    {
        auto s = lua::StackWrapper<>(mock_state.get()).newtable().pushinteger(1);
        REQUIRE_STACK(s, lua::Table, lua::Number);
        auto s2 = s.setfield<1>("some_field");
        REQUIRE_STACK(s2, lua::Table);
        auto s3 = s2.getfield<1>("some_field");
        REQUIRE_STACK(s3, lua::Table, lua::Unknown);
        auto s4 = s3.tointeger<-1>([] (int x) { REQUIRE(x == 1); });
        REQUIRE_STACK(s4, lua::Table, lua::Number);
        auto s5 = s4.pop<2>();
        REQUIRE_STACK(s5,);
    }
}
