#include <stdexcept>
#include <tuple>
#include <lua.hpp>

namespace lua {
struct Number {
    constexpr static int value = LUA_TNUMBER;
    constexpr static auto name = "a number";
};
struct Nil {
    constexpr static int value = LUA_TNIL;
    constexpr static auto name = "nil";
};
struct Function {
    constexpr static int value = LUA_TFUNCTION;
    constexpr static auto name = "a function";
};
struct Table {
    constexpr static int value = LUA_TTABLE;
    constexpr static auto name = "a table";
};
struct Unknown {
    constexpr static auto name = "an unknown type";
};

constexpr int toAbsoluteIndex(int stack_size, int i)
{
    if (i > 0) {
        return i;
    }

    return stack_size + i + 1;
}

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

template <typename SW, typename... NewType>
struct push_types;

template <template <typename...> class C, typename... Args, typename... NewType>
struct push_types<C<Args...>, NewType...> {
    using type = C<Args..., NewType...>;
};

template <typename SW, typename... NewType>
using push_type_t = typename push_types<SW, NewType...>::type;

template <typename S, int N>
struct pop_front;

template <typename S, int N>
using pop_front_t = typename pop_front<S, N>::type;

template <template<typename...> typename SW, typename Arg, typename... Args, int N>
struct pop_front<SW<Arg, Args...>, N> {
    using type = pop_front_t<SW<Args...>, N - 1>;
};

template <template<typename...> typename SW>
struct pop_front<SW<>, 0> {
    using type = SW<>;
};

template <template<typename...> typename SW, typename Arg, typename... Args>
struct pop_front<SW<Arg, Args...>, 0> {
    using type = SW<Arg, Args...>;
};

template <typename T, typename S>
struct concat_types;

template <template <typename...> typename SW1, typename... Args1, template <typename...> typename SW2, typename... Args2>
struct concat_types<SW1<Args1...>, SW2<Args2...>> {
    using type = SW1<Args1..., Args2...>;
};

template <typename T, typename S>
using concat_types_t = typename concat_types<T, S>::type;

template <typename SW, int N, typename NewType>
struct replace_type;

template <template <typename...> class C, typename... Args, int N, typename NewType>
struct replace_type<C<Args...>, N, NewType> {
    static_assert(sizeof...(Args) != 0, "Can't replace type of 0 size stack.");
    // Example:
    // If stack size is 2 and we want to replace the second element, N will be 2. So we first pop one element from the
    // back.
    constexpr static auto absolute_index = toAbsoluteIndex(sizeof...(Args), N);
    constexpr static auto args_to_pop = sizeof...(Args) - absolute_index + 1;
    using head = pop_back_t<C<Args...>, args_to_pop>;
    // The original stack size is 2. We we need all the elements after the to-be-replaced element.
    // So, if the replaced element is the second one, we need to pop two from the front.
    using tail = pop_front_t<C<Args...>, absolute_index>;

    using with_replaced_element = concat_types_t<head, C<NewType>>;
    using with_tail = concat_types_t<with_replaced_element, tail>;

    using type = with_tail;
};

template <typename SW, int N, typename NewType>
using replace_type_t = typename replace_type<SW, N, NewType>::type;

template <typename T, int N>
struct select_type;

template <template <typename...> class C, typename... Args, int N>
struct select_type<C<Args...>, N> {
    using type = typename std::tuple_element_t<N - 1, std::tuple<Args...>>;
};

template <typename T, int N>
using select_type_t = typename select_type<T, N>::type;

template <typename T, int IDX, int N>
struct rotate;

template <typename T, int IDX, int N>
using rotate_t = typename rotate<T, IDX, N>::type;

template <template <typename...> class C, typename... Args, int IDX>
struct rotate<C<Args...>, IDX, 0> {
    using type = C<Args...>;
};

template <template <typename...> class C, typename... Args, int IDX, int N>
struct rotate<C<Args...>, IDX, N> {
    constexpr static auto absolute_index = toAbsoluteIndex(sizeof...(Args), IDX);

    // We're rotating <A, B, C, D>
    // Let's say we want to rotate C and D.
    constexpr static auto args_to_pop = sizeof...(Args) - absolute_index + 1;

    // This is the part we don't want to rotate.
    // head = <A, B>
    using head = pop_back_t<C<Args...>, args_to_pop>;

    // This is the part we will rotate.
    // to_rotate = <C, D>
    using to_rotate = pop_front_t<C<Args...>, absolute_index - 1>;

    // We'll take the last element, append it to the front, and then pop the last element.
    using rotated = pop_back_t<concat_types_t<C<select_type_t<to_rotate, toAbsoluteIndex(to_rotate::stack_size, -1)>>, to_rotate>, 1>;

    // Lastly we'll prepend the head again.
    using rotated_with_head = concat_types_t<head, rotated>;

    using type = rotate_t<rotated_with_head, IDX, N - 1>;
};

template <int ArgNum, typename ArgType, typename... Rest>
void impl_check_lua_args(lua_State* state)
{
    if constexpr (!std::is_same_v<ArgType, Unknown>) {
        if (auto type = lua_type(state, ArgNum); type != ArgType::value) {
            using namespace std::string_literals;
            throw std::runtime_error("Stack value #%d should have been of type "s + lua_typename(state, ArgType::value) + " (got `" + lua_typename(state, type) + ")");
        }
    }

    if constexpr (sizeof...(Rest) != 0)  {
        impl_check_lua_args<ArgNum + 1, Rest...>(state);
    }

}

template <typename SW, typename What, int N>
struct append_times;

template <typename SW, typename What, int N>
using append_times_t = typename append_times<SW, What, N>::type;

template <template <typename...> class SW, typename... Args, typename What>
struct append_times<SW<Args...>, What, 0> {
    using type = SW<Args...>;
};

template <template <typename...> class SW, typename... Args, typename What, int N>
struct append_times<SW<Args...>, What, N> {
    using type = append_times_t<SW<Args..., What>, What, N - 1>;
};

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

template <typename ToCheck, typename Type>
struct is_same_or_unknown {
    constexpr static auto value = std::is_same_v<ToCheck, Type> || std::is_same_v<ToCheck, Unknown>;
};

template <typename ToCheck, typename Type>
const auto is_same_or_unknown_v = is_same_or_unknown<ToCheck, Type>::value;

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

    template <int NArgs, int NResults>
    [[nodiscard]] auto call()
    {
        static_assert(stack_size >= NArgs + 1, "Not enough elements on the stack for a function call");
        static_assert(is_same_or_unknown_v<ValueType<-1 - NArgs>, Function>, "The called element is not a function.");
        lua_call(m_state, NArgs, NResults);
        return append_times_t<pop_back_t<SW<Types...>, NArgs + 1>, Unknown, NResults>(m_state);
    }

    template <int IDX, int N>
    [[nodiscard]] auto rotate()
    {
        lua_rotate(m_state, IDX, N);
        return rotate_t<SW<Types...>, IDX, N>{m_state};
    }

    template <int N>
    [[nodiscard]] auto setfield(const char* key)
    {
        static_assert(toAbsoluteIndex(stack_size, N) != stack_size, "Can't use setfield with a table on top of the stack");
        static_assert(is_same_or_unknown_v<ValueType<N>, Table>, "The selected element is not a table.");
        check_unknown<N, Table>();
        lua_setfield(m_state, N, key);
        return pop_back_t<replace_type_t<SW<Types...>, N, Table>, 1>{m_state};
    }

    template <int N>
    [[nodiscard]] auto getfield(const char* key)
    {
        static_assert(is_same_or_unknown_v<ValueType<N>, Table>, "The selected element is not a table.");
        check_unknown<N, Table>();
        lua_getfield(m_state, N, key);
        return concat_types_t<replace_type_t<SW<Types...>, N, Table>, SW<lua::Unknown>>(m_state);
    }

    template <int N, typename Callable>
    [[nodiscard]] auto tocfunction(Callable&& callable)
    {
        static_assert(is_same_or_unknown_v<ValueType<N>, Function>, "The selected element is not a function.");
        check_unknown<N, Function>();
        callable(lua_tocfunction(m_state, N));
        return replace_type_t<SW<Types...>, N, Function>(m_state);
    }

    template <int N, typename Callable>
    [[nodiscard]] auto tointeger(Callable&& callable)
    {
        static_assert(is_same_or_unknown_v<ValueType<N>, Number>, "The selected element is not an int.");
        check_unknown<N, Number>();
        callable(lua_tointeger(m_state, N));
        return replace_type_t<SW<Types...>, N, Number>(m_state);
    }

    template <int N, typename Callable>
    [[nodiscard]] auto type(Callable&& callable)
    {
        callable(ValueType<N>::value);
        return SW<Types...>(m_state);
    }

    static constexpr int stack_size = sizeof...(Types);

private:

    template<int N, typename Type>
    void check_unknown()
    {
        if constexpr (std::is_same_v<ValueType<N>, Unknown>) {
            if (lua_type(m_state, N) != Type::value) {
                throw std::logic_error(std::string("The selected element is not ") + ValueType<N>::name);
            }
        }
    }

    template <int N>
    using ValueType = select_type_t<SW<Types...>, toAbsoluteIndex(stack_size, N)>;

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
    auto tointeger() = delete; // Empty stack has no integers.
    auto tocfunction() = delete; // Empty stack has no cfunctions.
    auto type() = delete; // Empty stack has no types.
    auto setfield() = delete; // Can't set field is the stack is empty.
    auto call() = delete; // Can't call if the stack is empty.
};
}

