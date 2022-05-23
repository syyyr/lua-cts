# Lua compile time stack
This library will be a C++ wrapper for `lua_State`, that handles stack operations in compile time.

## Motivation
The Lua C API is fairly concise. In my experience, every time I tried to make an abstraction over it, I came up with
code that looked almost the same except being able to name somethng (for example, wrapping `lua_next` with a function
called `lua__for_each`). However, I did sometimes run into problems with managing the stack. The main problem is keeping
track of what's on the stack and at what position. This library keeps track of whatever is on the in compile time.

## Examples

```cpp
auto s = lua::StackWrapper<>(state).pushinteger(1);
// decltype(s) -> lua::StackWrapper<lua::Int>

auto s2 = s.pop<1>();
// decltype(s2) -> lua::StackWrapper<>

s2.pop<1>(); // Doesn't compile, the stack is empty.
```

## Caveats
- the user must know, prior to creating a `lua::StackWrapper`, what value types are on the stack
- the user must make sure to save the return values of expressions that return a `lua::StackWrapper`. C++ being a
  statically typed language, it is not possible to save different stack states into the same variable. One option is
  making eveything a single expression:
  ```cpp
  lua::StackWrapper<>(state)
    .pushinteger(1)
    .tointeger([] (auto x) { std::cout << "We pushed " << x << "!"; })
    .pop<1>();
  ```
