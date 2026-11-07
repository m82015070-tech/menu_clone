// menu_clone plugin entry. Uses the template headers when present, with safe fallbacks.
#if __has_include(<plugin_common.h>)
#include <plugin_common.h>
#endif
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <new>
#include "ps4_compat.h"
#include "menu.h"
#include "natives.h"

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

// Own operator new/delete (malloc based) so we never depend on the C++ runtime library.
void* operator new(size_t n){ return malloc(n?n:1); }
void* operator new[](size_t n){ return malloc(n?n:1); }
void  operator delete(void* p) noexcept { free(p); }
void  operator delete[](void* p) noexcept { free(p); }
void  operator delete(void* p, size_t) noexcept { free(p); }
void  operator delete[](void* p, size_t) noexcept { free(p); }

// Symbols that the MP3 decoder (dr_mp3, built against a normal libc) expects but Sony libc does not export.
extern "C" int* __errno_location(void){ static int e=0; return &e; }
extern "C" void __assert_fail(const char*, const char*, unsigned, const char*){ __builtin_trap(); }

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

// --- 2) Per-frame callback: call from your game-thread hook (the same place your working menu runs).
void OnScriptTick(){ static bool init=false; if(!init){ M().Init(); init=true; } M().Tick(); }

// EXPERIMENTAL test mode: if /data/16menu/config/thread_tick.txt exists, call OnScriptTick() ~60x/second from our own thread.
// Natives from a foreign thread may flicker or crash the game: use only to test.
static volatile bool g_run=false; static void* g_tt=nullptr;
static void* TickThread(void*){ while(g_run){ OnScriptTick(); ps4::Usleep(16000); } return nullptr; }

extern "C" int attr_module_hidden module_start(long long, const void*){
    final_printf("[menu_clone] loaded\n"); ps4::Log("module_start: plugin loaded");
    if(FILE* f=fopen("/data/16menu/config/thread_tick.txt","r")){ fclose(f); g_run=true; ps4::ThreadCreate(&g_tt,nullptr,TickThread,nullptr,"menu_tick"); final_printf("[menu_clone] thread tick ON\n"); }
    return 0; }
extern "C" int attr_module_hidden module_stop(long long, const void*){ g_run=false; if(g_menuPtr) g_menuPtr->Shutdown(); return 0; }
