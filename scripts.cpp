#include "scripts.h"
#include "menu.h"
#include "natives.h"
#include "notify.h"
#include "ps4_compat.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

static const char* kDir = "/data/16menu/scripts";
static ScriptManager* g_sm = nullptr;
static int g_loadingSlot = -1;          // slot whose script_init() is running (menu_toggle/menu_action attach to it)

// ------------------------------------------------------------------ functions a C script can call
static void H_log(const char* m){ ps4::Log(m ? m : "(null)"); }
static void H_notify(const char* m){ PopupNotify(m ? m : ""); }
static void H_rect(float x,float y,float w,float h,int r,int g,int b,int a){ if(g_sm && g_sm->menu) g_sm->menu->ApiRect(x,y,w,h,r,g,b,a); }
static void H_text(const char* s,float x,float y,float sc,int r,int g,int b,int a){ if(g_sm && g_sm->menu && s) g_sm->menu->ApiText(s,x,y,sc,r,g,b,a); }
static int  H_padDown(int b){ return (g_sm && g_sm->menu && b>=0 && b<CTRL_N && g_sm->menu->pad.down[b]) ? 1 : 0; }
static int  H_padPressed(int b){ return (g_sm && g_sm->menu && b>=0 && b<CTRL_N && g_sm->menu->pad.edge[b]) ? 1 : 0; }
static unsigned long long H_time(void){ return ps4::TimeUs()/1000ull; }
static void H_menuToggle(const char* label, int* var){
    if(!g_sm || g_loadingSlot<0 || !var) return; ScriptSlot& s = g_sm->slots[g_loadingSlot]; if(s.nItems >= SCRIPT_MAX_ITEMS) return;
    ScriptItem& it = s.items[s.nItems++]; snprintf(it.label,sizeof it.label,"%s",label?label:"?"); it.var=var; it.cb=nullptr; it.isToggle=true; it.shadow=(*var)!=0; }
static void H_menuAction(const char* label, void (*cb)(void)){
    if(!g_sm || g_loadingSlot<0 || !cb) return; ScriptSlot& s = g_sm->slots[g_loadingSlot]; if(s.nItems >= SCRIPT_MAX_ITEMS) return;
    ScriptItem& it = s.items[s.nItems++]; snprintf(it.label,sizeof it.label,"%s",label?label:"?"); it.var=nullptr; it.cb=cb; it.isToggle=false; it.shadow=false; }
static unsigned long long H_native(unsigned o156, unsigned o157, const unsigned long long* args, int n){ NativeOff o{o156,o157}; return NativeInvoke(o,(const uint64_t*)args,n); }
static unsigned long long H_argF(float f){ return A(f); }
static unsigned long long H_argI(int i){ return A(i); }
static unsigned long long H_argP(const void* p){ return (unsigned long long)p; }

const HostSym* ScriptHostSymbols(int* count){
    static const HostSym table[] = {
        {"host_log",(void*)&H_log},{"host_notify",(void*)&H_notify},{"draw_rect",(void*)&H_rect},{"draw_text",(void*)&H_text},
        {"pad_down",(void*)&H_padDown},{"pad_pressed",(void*)&H_padPressed},{"time_ms",(void*)&H_time},
        {"menu_toggle",(void*)&H_menuToggle},{"menu_action",(void*)&H_menuAction},
        {"native_call",(void*)&H_native},{"arg_f",(void*)&H_argF},{"arg_i",(void*)&H_argI},{"arg_p",(void*)&H_argP},
        {"memcpy",(void*)&memcpy},{"memset",(void*)&memset},{"strlen",(void*)&strlen},{"strcmp",(void*)&strcmp},{"snprintf",(void*)&snprintf}
    };
    *count = (int)(sizeof table / sizeof table[0]);
    return table;
}

// ------------------------------------------------------------------ manager
static bool ReadFile(const char* path, std::string& out, size_t maxBytes){
    FILE* f = fopen(path,"rb"); if(!f) return false; char buf[4096]; size_t n;
    while((n = fread(buf,1,sizeof buf,f)) > 0){ out.append(buf,n); if(out.size() > maxBytes) break; }
    fclose(f); return !out.empty();
}

void ScriptManager::Init(Menu* m){ menu = m; g_sm = this; }

void ScriptManager::Scan(){
    std::vector<ScriptEntry> old = list; list.clear();
    std::vector<std::string> names; if(!ps4::ListDir(kDir,names)) return;
    for(auto& n : names){
        bool c = ps4::EndsWithI(n,".c"), p = ps4::EndsWithI(n,".prx") || ps4::EndsWithI(n,".sprx");
        if(!c && !p) continue;
        size_t dot = n.rfind('.'); ScriptEntry e; e.name = n.substr(0,dot); e.path = std::string(kDir)+"/"+n; e.isC = c;
        for(auto& o : old) if(o.path == e.path){ e = o; break; }          // keep state across refreshes
        list.push_back(e);
    }
    std::sort(list.begin(), list.end(), [](const ScriptEntry& a, const ScriptEntry& b){ return a.name < b.name; });
}

int ScriptManager::FreeSlots() const { int n = 0; for(int i=0;i<SCRIPT_SLOTS;i++) if(!slots[i].used) n++; return n; }

bool ScriptManager::Load(int idx, const HostApi* prxHost){
    if(idx < 0 || idx >= (int)list.size()) return false;
    ScriptEntry& e = list[idx];
    if(e.on) return true;
    e.error.clear();
    int slot = -1; for(int i=0;i<SCRIPT_SLOTS;i++) if(!slots[i].used){ slot = i; break; }
    if(slot < 0){ e.error = "No free slots"; ps4::Log("script: no free slots"); return false; }
    ScriptSlot& s = slots[slot]; s = ScriptSlot(); s.name = e.name;

    if(e.isC){
        std::string src; if(!ReadFile(e.path.c_str(), src, 256*1024)){ e.error = "cannot read file"; return false; }
        CompiledScript cs; std::string err;
        char b[200]; snprintf(b,sizeof b,"script: compiling %s (%u bytes)",e.name.c_str(),(unsigned)src.size()); ps4::Log(b);
        if(!TccCompile(e.name.c_str(), src, cs, err)){
            e.error = err.empty() ? "compile failed" : err;
            ps4::Log("script: COMPILE ERROR:"); ps4::Log(e.error.c_str());
            return false;
        }
        s.used = true; s.cs = cs; s.tick = cs.tick; s.stop = cs.stop; e.slot = slot; e.on = true;
        ps4::Log("script: compiled, running script_init");
        if(cs.init){ g_loadingSlot = slot; cs.init(); g_loadingSlot = -1; }
        snprintf(b,sizeof b,"script: %s running in slot %d",e.name.c_str(),slot); ps4::Log(b);
        return true;
    }

    // ---- .prx module (the older way)
    int res = 0; int h = ps4::LoadStartModule(e.path.c_str(),0,nullptr,0,nullptr,&res);
    { char b[200]; snprintf(b,sizeof b,"script: LoadStartModule(%s) -> %d (res %d)",e.name.c_str(),h,res); ps4::Log(b); }
    if(h < 0){ e.error = "cannot load module"; return false; }
    s.used = true; s.module = h; e.slot = slot; e.on = true;
    void* a = nullptr;
    if(ps4::Dlsym(h,"script_init",&a)==0 && a){ g_loadingSlot = slot; ((void(*)(const HostApi*))a)(prxHost); g_loadingSlot = -1; }
    a = nullptr; s.tick = (ps4::Dlsym(h,"script_tick",&a)==0) ? (void(*)(void))a : nullptr;
    a = nullptr; s.stop = (ps4::Dlsym(h,"script_stop",&a)==0) ? (void(*)(void))a : nullptr;
    return true;
}

void ScriptManager::Unload(int idx){
    if(idx < 0 || idx >= (int)list.size()) return;
    ScriptEntry& e = list[idx]; if(!e.on || e.slot < 0) return;
    ScriptSlot& s = slots[e.slot];
    if(s.stop) s.stop();
    if(e.isC) TccFree(s.cs); else { int res=0; ps4::StopUnloadModule(s.module,0,nullptr,0,nullptr,&res); }
    s = ScriptSlot(); e.on = false; e.slot = -1;
    ps4::Log("script: unloaded");
}
void ScriptManager::UnloadAll(){ for(int i=0;i<(int)list.size();i++) Unload(i); }
void ScriptManager::Tick(){ for(int i=0;i<SCRIPT_SLOTS;i++) if(slots[i].used && slots[i].tick) slots[i].tick(); }
