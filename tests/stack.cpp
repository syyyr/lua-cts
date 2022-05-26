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
        auto s = lua::StackWrapper<>(mock_state.get());
        REQUIRE_STACK(s,);
    }

    DOCTEST_SUBCASE("Runtime stack checking")
    {
        auto s = lua::StackWrapper<>(mock_state.get()).pushinteger(1);
        REQUIRE_STACK(s, lua::Number);
        REQUIRE_THROWS(lua::StackWrapper<>(mock_state.get()));
        REQUIRE_THROWS(lua::StackWrapper<lua::Nil>(mock_state.get()));
        auto s2 = lua::StackWrapper<lua::Number>(mock_state.get());
        REQUIRE_STACK(s2, lua::Number);
    }

    DOCTEST_SUBCASE("Unknown types")
    {
        DOCTEST_SUBCASE("Initializing")
        {
            auto s = lua::StackWrapper<>(mock_state.get()).pushinteger(1);
            REQUIRE_STACK(s, lua::Number);
            auto s2 = lua::StackWrapper<lua::Unknown>(mock_state.get());
            REQUIRE_STACK(s2, lua::Unknown);
            REQUIRE_THROWS(lua::StackWrapper<lua::Unknown, lua::Unknown>(mock_state.get()));
        }

        DOCTEST_SUBCASE("Asserting types")
        {
            auto s = lua::StackWrapper<>(mock_state.get()).pushinteger(1);
            REQUIRE_STACK(s, lua::Number);
            auto s2 = lua::StackWrapper<lua::Unknown>(mock_state.get());
            REQUIRE_STACK(s2, lua::Unknown);
            auto s3 = s.tointeger<-1>([] (int x) { REQUIRE(x == 1); } );
            REQUIRE_STACK(s3, lua::Number);
        }
    }

    DOCTEST_SUBCASE("Pushing values")
    {
        auto s = lua::StackWrapper<>(mock_state.get());
        DOCTEST_SUBCASE("Integer")
        {
            auto s2 = s.pushinteger(1);
            REQUIRE_STACK(s2, lua::Number);
            auto s3 = s2.tointeger<1>([] (int x) {REQUIRE(x == 1);} );
            REQUIRE_STACK(s3, lua::Number);
            auto s4 = s3.tointeger<-1>([] (int x) {REQUIRE(x == 1);} );
            REQUIRE_STACK(s4, lua::Number);
        }

        DOCTEST_SUBCASE("C Function")
        {
            auto s2 = s.pushcfunction(some_function);
            REQUIRE_STACK(s2, lua::Function);
            auto s3 = s2.tocfunction<-1>([] (int (*x)(lua_State*)) { REQUIRE(x(nullptr) == SOME_MAGIC_NUMBER); });
            REQUIRE_STACK(s3, lua::Function);
        }

        DOCTEST_SUBCASE("Table")
        {
            auto s2 = s.newtable();
            REQUIRE_STACK(s2, lua::Table);
            auto s3 = s2.type<-1>([] (int type) { REQUIRE(type == LUA_TTABLE); });
            REQUIRE_STACK(s3, lua::Table);
        }

    }

    DOCTEST_SUBCASE("Querying type")
    {
        auto s = lua::StackWrapper<>(mock_state.get());
        REQUIRE_STACK(s,);

        DOCTEST_SUBCASE("number")
        {
            auto s2 = s.pushinteger(1);
            REQUIRE_STACK(s2, lua::Number);
            auto s3 = s2.type<1>([] (int type) {REQUIRE(type == LUA_TNUMBER);});
            REQUIRE_STACK(s3, lua::Number);
        }

        DOCTEST_SUBCASE("nil")
        {
            auto s2 = s.pushnil();
            REQUIRE_STACK(s2, lua::Nil);
            auto s3 = s2.type<1>([] (int type) {REQUIRE(type == LUA_TNIL);});
            REQUIRE_STACK(s3, lua::Nil);
        }

        DOCTEST_SUBCASE("function")
        {
            auto s2 = s.pushcfunction(some_function);
            REQUIRE_STACK(s2, lua::Function);
            auto s3 = s2.type<1>([] (int type) {REQUIRE(type == LUA_TFUNCTION);});
            REQUIRE_STACK(s3, lua::Function);
        }
    }

    DOCTEST_SUBCASE("Popping elements")
    {
        auto s = lua::StackWrapper<>(mock_state.get()).pushinteger(1);
        REQUIRE_STACK(s, lua::Number);

        DOCTEST_SUBCASE("Popping one")
        {
            auto s2 = s.pop<1>();
            REQUIRE_STACK(s2,);
        }

        DOCTEST_SUBCASE("Popping two")
        {
            auto s2 = s.pushinteger(1).pop<2>();
            REQUIRE_STACK(s2,);
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

        auto s5 = lua::StackWrapper<lua::Unknown, lua::Number>(mock_state.get());
        REQUIRE_STACK(s5, lua::Unknown, lua::Number);

        auto s6 = s5.setfield<1>("setfield");
        REQUIRE_STACK(s6, lua::Table);

        auto s7 = lua::StackWrapper<lua::Unknown>(mock_state.get());
        REQUIRE_STACK(s7, lua::Unknown);

        auto s8 = s7.getfield<1>("some_field");
        REQUIRE_STACK(s8, lua::Table, lua::Unknown);

        auto s9 = s8.tointeger<-1>([] (int x) { REQUIRE(x == 1); });
        REQUIRE_STACK(s9, lua::Table, lua::Number);

        auto s10 = s9.pop<2>();
        REQUIRE_STACK(s10,);
    }
}
