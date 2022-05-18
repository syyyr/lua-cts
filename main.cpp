#include <iostream>
#include "lua-cts.hpp"
int main(int argc, char* argv[])
{
	auto state = luaL_newstate();
	luaL_openlibs(state);
	int aha;
	auto s = lua::StackWrapper<>(state).pushinteger(1).pushinteger(1).pushinteger(1).pushinteger(1);
	std::cerr << "s.stack_size = " << s.stack_size << "\n";
	auto s1 = s.pop<4>();

	return 0;
}
