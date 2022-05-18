#include <iostream>
#include "lua-cts.hpp"
int main(int argc, char* argv[])
{
	auto state = luaL_newstate();
	luaL_openlibs(state);
	int aha;
	auto s = lua::StackWrapper<>(state).pushinteger(1);
	s.toint<1>([] (auto lol){
			std::cerr << "lol" << " = " << lol << "\n";
	});
	std::cerr << "s.stack_size = " << s.stack_size << "\n";

	return 0;
}
