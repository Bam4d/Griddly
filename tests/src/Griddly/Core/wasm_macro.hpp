
#ifndef GRIDDLY_WASM_MACRO_HPP
#define GRIDDLY_WASM_MACRO_HPP


#ifdef WASM
#define GDYFACTORY_ARGS(arg1, arg2) arg1, arg2
#else
#define GDYFACTORY_ARGS(arg1, arg2) arg1, arg2, ResourceConfig{}
#endif


#ifdef WASM
#define WASM_OFF(...)
#else
#define WASM_OFF(...) __VA_ARGS__
#endif


#endif //GRIDDLY_WASM_MACRO_HPP
