#pragma once
#include <string>
#include <vector>
#include <functional>
#include "menu_config.h"
#include "scripts.h"
#include "addons.h"
#include "music.h"
#include "pad.h"
enum class RowType { Action, Toggle, List, Submenu };
struct Row {
    std::string label; RowType type=RowType::Action;
    const char* icon=nullptr;                    // sprite in "commonmenu"
    bool* toggle=nullptr;                        // Toggle
    std::vector<std::string> options; int* optIndex=nullptr;   // List -> "< %s >"
    int submenu=-1; std::function<void()> onSelect;
};
struct Page { std::string title; std::vector<Row> rows; int parent=-1; };
class Menu {
public:
    MenuConfig cfg; std::vector<Page> pages; int page=0, sel=0, scroll=0; bool open=false;
    ScriptManager scripts; AddonManager addons; MusicPlayer music;
    bool scriptOn[64]={}, addonOn[64]={}; int vol=7; std::string msg; int msgFrames=0; int pendingRebuild=0;
    PadTracker pad;        // button reader (PAD_A = just pressed, PAD_B = held)
    OpenGesture gesture;   // L1 + Circle within 700 ms, any order
    bool safeMode=false;   // true when ticking from a foreign thread: only DRAW_RECT + text natives (no textures / scaleform)
    void Init(); void Tick(); void Shutdown();
    void Notify(const char* m){ msg=m; msgFrames=240; }
    // used by C scripts (see scripts.cpp)
    void ApiRect(float x,float y,float w,float h,int r,int g,int b,int a){ Rect(x,y,w,h,r,g,b,a); }
    void ApiText(const char* s,float x,float y,float sc,int r,int g,int b,int a){ Text(s,x,y,sc,r,g,b,a,0,1); }
private:
    bool texReady=false; uint32_t movie=0;
    void Input(); void Draw(); void DrawFooter(); void DrawWidget(); void BuildPages(); void BuildScripts(); void BuildScriptPages(); void BuildAddons(); void BuildMusic();
    void Rect(float x,float y,float w,float h,int r,int g,int b,int a);
    bool Sprite(const char* dict,const char* name,float x,float y,float w,float h,float rot=0,int r=255,int g=255,int b=255,int a=255);
    void Text(const std::string& s,float x,float y,float scale,int r,int g,int b,int a,int font=0,int justify=1);
};
