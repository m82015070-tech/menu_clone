#pragma once
#include <string>
#include <vector>
#include <functional>
#include "config.h"
#include "scripts.h"
#include "addons.h"
#include "music.h"
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
    void Init(); void Tick(); void Shutdown();
    void Notify(const char* m){ msg=m; msgFrames=240; }
private:
    bool texReady=false; uint32_t movie=0;
    void Input(); void Draw(); void DrawFooter(); void DrawWidget(); void BuildPages(); void BuildScripts(); void BuildAddons(); void BuildMusic();
    void Rect(float x,float y,float w,float h,int r,int g,int b,int a);
    bool Sprite(const char* dict,const char* name,float x,float y,float w,float h,float rot=0,int r=255,int g=255,int b=255,int a=255);
    void Text(const std::string& s,float x,float y,float scale,int r,int g,int b,int a,int font=0,int justify=1);
};
