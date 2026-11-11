#include "tcc_backend.h"
#include "script_api_text.h"
#include "ps4_compat.h"
#include "have_tcc.h"
#include <cstdio>
#include <cstring>

#ifdef HAVE_TCC
extern "C" {
#include "libtcc.h"
}

static void ErrFunc(void* opaque, const char* msg){
    std::string* e = (std::string*)opaque;
    if(e->size() < 2000){ e->append(msg); e->append("\n"); }
}

struct CompileJob { const char* name; const std::string* src; CompiledScript* out; std::string* err; bool ok; };

static void DoCompile(CompileJob* j){
    j->ok = false;
    TCCState* s = tcc_new();
    if(!s){ *j->err = "tcc_new failed"; return; }
    tcc_set_error_func(s, j->err, ErrFunc);
    tcc_set_options(s, "-nostdlib -nostdinc -w");
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    int n = 0; const HostSym* syms = ScriptHostSymbols(&n);
    for(int i=0;i<n;i++) tcc_add_symbol(s, syms[i].name, syms[i].addr);

    std::string full; full.reserve(sizeof(kScriptApiPrelude) + j->src->size() + 64);
    full += kScriptApiPrelude; full += "\n#line 1 \""; full += j->name; full += "\"\n"; full += *j->src;
    if(tcc_compile_string(s, full.c_str()) < 0){ tcc_delete(s); return; }

    int sz = tcc_relocate(s, nullptr);                       // size needed
    if(sz <= 0){ j->err->append("relocate: could not compute size\n"); tcc_delete(s); return; }
    void* mem = ps4::ExecAlloc((size_t)sz);
    if(!mem){ j->err->append("cannot allocate executable memory\n"); tcc_delete(s); return; }
    if(tcc_relocate(s, mem) < 0){ j->err->append("relocate failed (undefined symbol?)\n"); ps4::ExecFree(mem,(size_t)sz); tcc_delete(s); return; }

    j->out->state = s; j->out->mem = mem; j->out->memSize = (size_t)sz;
    j->out->init = (void(*)(void))tcc_get_symbol(s, "script_init");
    j->out->tick = (void(*)(void))tcc_get_symbol(s, "script_tick");
    j->out->stop = (void(*)(void))tcc_get_symbol(s, "script_stop");
    j->ok = true;
}
static void* JobThread(void* p){ DoCompile((CompileJob*)p); return nullptr; }

bool TccAvailable(){ return true; }

bool TccCompile(const char* name, const std::string& source, CompiledScript& out, std::string& err){
    CompileJob j{ name, &source, &out, &err, false };
    ps4::RunBigStack(JobThread, &j, 2u*1024u*1024u);      // 2 MB stack for the compiler
    return j.ok;
}
void TccFree(CompiledScript& s){
    if(s.state) tcc_delete((TCCState*)s.state);
    ps4::ExecFree(s.mem, s.memSize);
    s = CompiledScript();
}

#else  // ---------------- TinyCC not built into this plugin ----------------
bool TccAvailable(){ return false; }
bool TccCompile(const char*, const std::string&, CompiledScript&, std::string& err){ err = "TinyCC is not built into this plugin (C scripts disabled)"; return false; }
void TccFree(CompiledScript& s){ s = CompiledScript(); }
#endif
