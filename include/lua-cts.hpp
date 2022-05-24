#include <stdexcept>
#include <tuple>
#include <lua.hpp>

namespace lua {
using Number = std::integral_constant<int, LUA_TNUMBER>;
using Nil = std::integral_constant<int, LUA_TNIL>;
using Function = std::integral_constant<int, LUA_TFUNCTION>;
using Table = std::integral_constant<int, LUA_TTABLE>;

template <typename T, typename S>
struct pop_back_impl;

template <template <typename...> class C, typename... Args, std::size_t... Is>
struct pop_back_impl<C<Args...>, std::index_sequence<Is...>> {
    using type = C<std::tuple_element_t<Is, std::tuple<Args...>>...>;
};

template <typename C, int N>
struct pop_back {
    static_assert(C::stack_size - N >= 0, "Can't pop more values than present on the stack");
    using type = typename pop_back_impl<C, std::make_index_sequence<(C::stack_size) - N>>::type;
};

template <typename C, int N>
using pop_back_t = typename pop_back<C, N>::type;

template<typename T>
struct tag {
    using type = T;
};

template<int N, typename... Ts>
struct select_type {
    using type = typename std::tuple_element_t<N, std::tuple<Ts...>>;
};

template <int N>
struct select_type<N> {
    using type = void;
};

template <int N, typename... Ts>
using select_type_t = typename select_type<N, Ts...>::type;

template <int ArgNum, typename ArgType, typename... Rest>
void impl_check_lua_args(lua_State* state)
{
    if (auto type = lua_type(state, ArgNum); type != ArgType::value) {
        using namespace std::string_literals;
        throw std::runtime_error("Stack value #%d should have been of type "s + lua_typename(state, ArgType::value) + " (got `" + lua_typename(state, type) + ")");
    }

    if constexpr (sizeof...(Rest) != 0)  {
        impl_check_lua_args<ArgNum + 1, Rest...>(state);
    }

}

template <typename... ExpectedArgTypes>
void check_lua_args(lua_State* state)
{
    if (auto nargs = lua_gettop(state); nargs != sizeof...(ExpectedArgTypes)) {
        throw std::runtime_error("Expected stack size is " + std::to_string(sizeof...(ExpectedArgTypes)) + " (got " + std::to_string(nargs) + ")");
    }

    if constexpr (sizeof...(ExpectedArgTypes) != 0) {
        impl_check_lua_args<1, ExpectedArgTypes...>(state);
    }
}

template <template <typename...> typename SW, typename ...Types>
class impl_StackWrapper {
public:
    impl_StackWrapper(lua_State* state)
        : m_state(state)
    {
        check_lua_args<Types...>(state);
    }

    template <int N>
    [[nodiscard]] auto pop()
    {
        lua_pop(m_state, N);
        return pop_back_t<SW<Types...>, N>{m_state};
    }

    [[nodiscard]] auto pushinteger(int val)
    {
        lua_pushinteger(m_state, val);
        return SW<Types..., lua::Number>(m_state);
    }

    [[nodiscard]] auto pushnil()
    {
        lua_pushnil(m_state);
        return SW<Types..., lua::Nil>(m_state);
    }

    [[nodiscard]] auto pushcfunction(lua_CFunction func)
    {
        lua_pushcfunction(m_state, func);
        return SW<Types..., lua::Function>(m_state);
    }

    [[nodiscard]] auto newtable()
    {
        lua_newtable(m_state);
        return SW<Types..., lua::Table>(m_state);
    }

    template <int N, typename Callable>
    [[nodiscard]] auto tocfunction(Callable&& callable)
    {
        static_assert(std::is_same_v<ValueType<N>, Function>, "The selected element is not a function.");
        callable(lua_tocfunction(m_state, N));
        return SW<Types...>(m_state);
    }

    template <int N, typename Callable>
    [[nodiscard]] auto tointeger(Callable&& callable)
    {
        static_assert(std::is_same_v<ValueType<N>, Number>, "The selected element is not an int.");
        callable(lua_tointeger(m_state, N));
        return SW<Types...>(m_state);
    }

    template <int N, typename Callable>
    [[nodiscard]] auto type(Callable&& callable)
    {
        callable(ValueType<N>::value);
        return SW<Types...>(m_state);
    }

    static constexpr int stack_size = sizeof...(Types);

private:

    constexpr static int toAbsoluteIndex(int i)
    {
        if (i > 0) {
            return i;
        }

        return stack_size + i + 1;
    }

    template <int N>
    using ValueType = select_type_t<toAbsoluteIndex(N) - 1, Types...>;

    lua_State* m_state;
};

template <typename... Types>
class StackWrapper : public impl_StackWrapper<StackWrapper, Types...> {
    using impl_StackWrapper<StackWrapper, Types...>::impl_StackWrapper;
};

template <>
class StackWrapper<> : public impl_StackWrapper<StackWrapper> {
public:
    using impl_StackWrapper<StackWrapper>::impl_StackWrapper;

    auto pop() = delete; // Can't delete from an empty stack.
    auto tointeger() = delete; // Empty stack has not integers.
    auto type() = delete; // Empty stack has no types.
};
}

