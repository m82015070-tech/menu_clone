#include "scripts.h"
#include <cstring>
#include <algorithm>
#include "ps4_compat.h"
static const char* kDir="/data/16menu/scripts";
void ScriptManager::Scan(){
    std::vector<ScriptEntry> old=list; list.clear();
    std::vector<std::string> names; if(!ps4::ListDir(kDir,names)) return;
    for(auto& n:names){
        if(!ps4::EndsWithI(n,".prx") && !ps4::EndsWithI(n,".sprx")) continue;
        size_t p=n.rfind('.'); ScriptEntry s; s.name=n.substr(0,p); s.path=std::string(kDir)+"/"+n;
        for(auto& o:old) if(o.path==s.path){ s=o; break; }   // keep loaded state across rescans
        list.push_back(s);
    }
    std::sort(list.begin(),list.end(),[](const ScriptEntry&a,const ScriptEntry&b){return a.name<b.name;});
}
bool ScriptManager::Load(int i,const HostApi* host){
    if(i<0||i>=(int)list.size()||list[i].on) return false; ScriptEntry& s=list[i];
    int res=0; int h=ps4::LoadStartModule(s.path.c_str(),0,nullptr,0,nullptr,&res);
    if(h<0) return false;
    s.handle=h;
    void* a=nullptr;
    if(ps4::Dlsym(h,"script_init",&a)==0&&a) ((void(*)(const HostApi*))a)(host);
    a=nullptr; s.tick=(ps4::Dlsym(h,"script_tick",&a)==0)?(void(*)())a:nullptr;
    a=nullptr; s.stop=(ps4::Dlsym(h,"script_stop",&a)==0)?(void(*)())a:nullptr;
    s.on=true; return true;
}
void ScriptManager::Unload(int i){
    if(i<0||i>=(int)list.size()||!list[i].on) return; ScriptEntry& s=list[i];
    if(s.stop) s.stop();
    int res=0; ps4::StopUnloadModule(s.handle,0,nullptr,0,nullptr,&res);
    s.on=false; s.handle=-1; s.tick=nullptr; s.stop=nullptr;
}
void ScriptManager::UnloadAll(){ for(int i=0;i<(int)list.size();i++) Unload(i); }
void ScriptManager::Tick(){ for(auto& s:list) if(s.on&&s.tick) s.tick(); }
