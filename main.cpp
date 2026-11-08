// menu_clone plugin entry. Uses the template headers when present, with safe fallbacks.
#if __has_include(<plugin_common.h>)
#include <plugin_common.h>
#endif
#if __has_include(<Detour.h>)
#include <Detour.h>
#define HAVE_DETOUR 1
#else
#define HAVE_DETOUR 0
#endif
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <new>
#include "ps4_compat.h"
#include "menu.h"
#include "natives.h"
#include "notify.h"

#ifndef attr_public
#define attr_public __attribute__((visibility("default")))
#endif
#ifndef attr_module_hidden
#define attr_module_hidden __attribute__((weak)) __attribute__((visibility("hidden")))
#endif
#ifndef final_printf
#define final_printf printf
#endif

extern "C" {
attr_public const char* g_pluginName = "menu_clone";
attr_public const char* g_pluginDesc = "GTA V menu: scripts, add-ons/RPF, music widget";
attr_public const char* g_pluginAuth = "you";
attr_public unsigned int g_pluginVersion = 0x00000100;
}

// Own operator new/delete (malloc based). They must NOT be exported to the game process (they would replace the
// game allocator). The build keeps them local with a linker version script (see build.yml), not with attributes.
#define HID __attribute__((visibility("hidden")))
void* operator new(size_t n){ return malloc(n?n:1); }
void* operator new[](size_t n){ return malloc(n?n:1); }
void  operator delete(void* p) noexcept { free(p); }
void  operator delete[](void* p) noexcept { free(p); }
void  operator delete(void* p, size_t) noexcept { free(p); }
void  operator delete[](void* p, size_t) noexcept { free(p); }

// Symbols that the MP3 decoder (dr_mp3, built against a normal libc) expects but Sony libc does not export. Hidden too.
extern "C" HID int* __errno_location(void){ static int e=0; return &e; }
extern "C" HID void __assert_fail(const char*, const char*, unsigned, const char*){ __builtin_trap(); }

// Created on first use: static constructors are not guaranteed to run inside a PRX.
static Menu* g_menuPtr=nullptr;
static Menu& M(){ if(!g_menuPtr) g_menuPtr=new Menu(); return *g_menuPtr; }

// --- 1) File redirect: call from your sceKernelOpen (or game open) hook. Handles (a) the menu own scaleform .rpf
//        and (b) every enabled add-on .rpf from /data/16menu/addons/.
const char* RedirectPath(const char* path){
    if(!path) return path;
    static const char* kName="scaleform_platform_ps4.rpf"; static const char* kNew="/data/16menu/data/scaleform_platform_ps4.rpf";
    size_t n=strlen(path), m=strlen(kName);
    if(n>=m && ps4::StrCaseCmp(path+n-m,kName)==0){ FILE* f=fopen(kNew,"rb"); if(f){ fclose(f); return kNew; } }
    return M().addons.Redirect(path);
}

// --- 2) Per-frame callback.
void OnScriptTick(){ static bool init=false; if(!init){ M().Init(); init=true; } M().Tick(); }

// --- 3) HOOK MODE (uses the real GoldHEN SDK Detour). If /data/16menu/config/hook_tick.txt exists, we put a detour on
//     one GAME NATIVE HANDLER that the game scripts call every frame. Our tick then runs INSIDE the game's own script
//     thread (no flicker, correct context). The file may contain the native name (default DRAW_RECT; others: PAD_A,
//     PAD_B, PAD_C, REQUEST_TXD, HAS_TXD, SET_TEXT_FONT, SET_TEXT_SCALE, SET_TEXT_COLOUR, BEGIN_TEXT, END_TEXT, DRAW_SPRITE).
#if HAVE_DETOUR
typedef void (*NativeFn)(void*);
static Detour g_det;
static volatile int g_inHook=0, g_hookFired=0;
static unsigned long long g_lastUs=0;
static void FrameNativeHook(void* ctx){
    Detour_Stub(&g_det, NativeFn, ctx);                  // run the real native first
    if(g_inHook) return;                                  // our own native calls go through here too: do not recurse
    if(!g_hookFired){ g_hookFired=1; ps4::Log("hook: first call from the game - hook is alive"); }
    unsigned long long now=ps4::TimeUs();
    if(now-g_lastUs<8000ull) return;                      // at most ~120 ticks/s, the game calls natives many times per frame
    g_lastUs=now; g_inHook=1; OnScriptTick(); g_inHook=0;
}
static bool InstallFrameHook(const char* name){
    NativeOff off{0,0};
    if(!NativeByName(name,off)){ ps4::Log("hook: unknown native name in hook_tick.txt"); return false; }
    uint64_t addr=NativeAddr(off);
    char b[128]; snprintf(b,sizeof b,"hook: installing on %s at 0x%llX",name,(unsigned long long)addr); ps4::Log(b);
    Detour_Construct(&g_det, DetourMode_x64);
    void* stub=Detour_DetourFunction(&g_det,(uint64_t)addr,(void*)FrameNativeHook);
    ps4::Log(stub ? "hook: installed (waiting for the game to call it)" : "hook: Detour_DetourFunction returned NULL");
    return stub!=nullptr;
}
#endif

// --- 4) Boot thread. module_start itself does NOTHING risky: it only starts this thread.
//     After 20 s (game finished booting): life sign in the log + a PS4 popup, then the chosen tick mode:
//       hook_tick.txt   -> detour on a game native (preferred)
//       thread_tick.txt -> tick from this thread (works but may flicker)
//       neither         -> idle
static volatile bool g_run=false; static void* g_tt=nullptr;
static void* BootThread(void*){
    ps4::Usleep(20u*1000u*1000u);
    if(!g_run) return nullptr;
    ps4::Log("boot thread: 20 s after start, plugin is alive");
#if HAVE_DETOUR
    if(FILE* f=fopen("/data/16menu/config/hook_tick.txt","r")){
        char name[48]="DRAW_RECT", line[48]={0};
        if(fgets(line,sizeof line,f)){ size_t n=strlen(line); while(n&&(line[n-1]=='\n'||line[n-1]=='\r'||line[n-1]==' ')) line[--n]=0; if(n) strncpy(name,line,sizeof name-1); }
        fclose(f);
        PopupNotify("menu_clone loaded: hook mode - L1 + Circle opens the menu");
        if(!InstallFrameHook(name)) PopupNotify("menu_clone: hook FAILED (see menu_clone.log)");
        return nullptr;
    }
#endif
    FILE* f=fopen("/data/16menu/config/thread_tick.txt","r");
    if(!f){ ps4::Log("no hook_tick.txt / thread_tick.txt: idle"); PopupNotify("menu_clone loaded (idle: create hook_tick.txt or thread_tick.txt)"); return nullptr; }
    fclose(f);
    ps4::Log("thread_tick mode ON: calling OnScriptTick() from this thread");
    PopupNotify("menu_clone loaded: thread mode - L1 + Circle opens the menu");
    while(g_run){ OnScriptTick(); ps4::Usleep(16000); }
    return nullptr;
}

extern "C" int attr_module_hidden module_start(long long, const void*){
    final_printf("[menu_clone] loaded\n");
    g_run=true;
    ps4::ThreadCreate(&g_tt,nullptr,BootThread,nullptr,"menu_boot");
    return 0;
}
extern "C" int attr_module_hidden module_stop(long long, const void*){
    g_run=false;
    if(g_menuPtr) g_menuPtr->Shutdown();
    return 0;
}
