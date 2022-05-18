#include <tuple>
#include <lua.hpp>

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

namespace lua {
using Int = std::integral_constant<int, LUA_TNUMBER>;
using Nil = std::integral_constant<int, LUA_TNIL>;

template <template <typename...> typename SW, typename ...Types>
class impl_StackWrapper {
public:
	impl_StackWrapper(lua_State* state)
		: m_state(state)
	{
	}

	template <int N>
	auto pop()
	{
		lua_pop(m_state, N);
		return pop_back_t<SW<Types...>, N>{m_state};
	}

	auto pushinteger(int val)
	{
		lua_pushinteger(m_state, val);
		return SW<Types..., lua::Int>(m_state);
	}

	auto pushnil()
	{
		lua_pushnil(m_state);
		return SW<Types..., lua::Nil>(m_state);
	}

	static constexpr int stack_size = sizeof...(Types);

private:
	lua_State* m_state;
};

template <typename... Types>
class StackWrapper : public impl_StackWrapper<StackWrapper, Types...> {
	using impl_StackWrapper<StackWrapper, Types...>::impl_StackWrapper;
};

template <>
class StackWrapper<> : public impl_StackWrapper<StackWrapper> {
	using impl_StackWrapper<StackWrapper>::impl_StackWrapper;
	public:

	template <int N> auto pop() = delete; // Can't delete from an empty stack.

};
}

