# Lua compile time stack
This library will be a C++ wrapper for `lua_State`, that handles stack operations in compile time.

## Examples

```cpp
auto s = lua::StackWrapper<>(state).pushinteger(1);
// decltype(s) -> lua::StackWrapper<lua::Int>

auto s2 = s.pop<1>();
// decltype(s2) -> lua::StackWrapper<>

s2.pop<1>(); // Doesn't compile, the stack is empty.
