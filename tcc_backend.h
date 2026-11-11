#pragma once
// C scripts are compiled at runtime by TinyCC (like the original menus do). Everything is behind HAVE_TCC (have_tcc.h):
// when TinyCC could not be built into the plugin, TccAvailable() is false and compiling reports a clear error.
#include <string>
#include <cstddef>

struct CompiledScript {
    void* state = nullptr;            // TCCState*
    void* mem = nullptr;              // executable memory holding the script
    size_t memSize = 0;
    void (*init)(void) = nullptr;
    void (*tick)(void) = nullptr;
    void (*stop)(void) = nullptr;
};
struct HostSym { const char* name; void* addr; };
const HostSym* ScriptHostSymbols(int* count);     // implemented in scripts.cpp (functions a script may call)

bool TccAvailable();
bool TccCompile(const char* name, const std::string& source, CompiledScript& out, std::string& err);
void TccFree(CompiledScript& s);
