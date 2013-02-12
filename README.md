LuaJIT is a fantastic project. However it has been built ground up with floating-point in mind. Unfortunately
some embedded devices and restricted programming environments dont support floating-point. 

These environments would benefit from a high level languages like Lua. But speed or memory is often an issue.
This project aims to address those issues by writing a very basic JIT for Lua 5.2. 

There are no optimisations at all other than the pure machine language. Register allocation is extremely
trivial. Data structs will be designed to reduce memory footprint. A simple example is separating a variables
tag from its value ( gives nearly 50% memory saving immediately ).
