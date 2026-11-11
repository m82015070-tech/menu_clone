#pragma once
#include <string>
#include <vector>
#include "script_api.h"        // HostApi for PRX scripts (the older way)
#include "tcc_backend.h"       // C scripts compiled at runtime by TinyCC (the same way the original menus work)
class Menu;

enum { SCRIPT_SLOTS = 8, SCRIPT_MAX_ITEMS = 16 };

struct ScriptItem { char label[48]; int* var; void (*cb)(void); bool shadow; bool isToggle; };

struct ScriptSlot {
    bool used = false;
    std::string name;
    CompiledScript cs;                // C script
    int module = -1;                  // PRX script
    void (*tick)(void) = nullptr;
    void (*stop)(void) = nullptr;
    ScriptItem items[SCRIPT_MAX_ITEMS];
    int nItems = 0;
};

struct ScriptEntry {
    std::string name, path, error;
    bool isC = false;                 // .c source (TinyCC)   /  false = .prx module
    bool on = false;
    int slot = -1;
};

class ScriptManager {
public:
    std::vector<ScriptEntry> list;
    ScriptSlot slots[SCRIPT_SLOTS];
    Menu* menu = nullptr;

    void Init(Menu* m);
    void Scan();                                       // /data/16menu/scripts/*.c  *.prx  *.sprx
    bool Load(int entry, const HostApi* prxHost);      // returns false on error (see list[entry].error)
    void Unload(int entry);
    void UnloadAll();
    void Tick();                                       // call every frame
    int  FreeSlots() const;
};
