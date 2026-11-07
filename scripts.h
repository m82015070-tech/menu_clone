#pragma once
#include <string>
#include <vector>
#include "script_api.h"
struct ScriptEntry { std::string name, path; bool on=false; int handle=-1; void(*tick)()=nullptr; void(*stop)()=nullptr; };
class ScriptManager {
public:
    std::vector<ScriptEntry> list;
    void Scan();                       // /data/16menu/scripts/*.prx|*.sprx
    bool Load(int i, const HostApi* host);
    void Unload(int i);
    void UnloadAll();
    void Tick();                       // call every frame
};
