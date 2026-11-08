#include "menu.h"
#include "natives.h"
#include <cstdio>
#include <cstring>
#include "ps4_compat.h"
#include "notify.h"

// ---------- primitives (same native sequences the original uses) ----------
void Menu::Rect(float x,float y,float w,float h,int r,int g,int b,int a){
    Call(NAT::DRAW_RECT,x,y,w,h,r,g,b,a);
}
// Original sprite helper (VA 0xb430): REQUEST dict -> HAS_LOADED -> DRAW_SPRITE
bool Menu::Sprite(const char* d,const char* n,float x,float y,float w,float h,float rot,int r,int g,int b,int a){
    if(safeMode) return false;      // texture natives need the game's own script thread
    Call(NAT::REQUEST_STREAMED_TEXTURE_DICT,d,0);
    if(!Call(NAT::HAS_STREAMED_TEXTURE_DICT_LOADED,d)) return false;
    Call(NAT::DRAW_SPRITE,d,n,x,y,w,h,rot,r,g,b,a); return true;
}
// Original text helper (VA 0xe1f0..0xe560): font, scale, colour, wrap, justify, shadow, BEGIN "STRING", ADD, END(x,y)
void Menu::Text(const std::string& s,float x,float y,float sc,int r,int g,int b,int a,int font,int justify){
    Call(NAT::SET_TEXT_FONT,font);
    Call(NAT::SET_TEXT_SCALE,sc,sc);
    Call(NAT::SET_TEXT_COLOUR,r,g,b,a);
    Call(NAT::SET_TEXT_WRAP,0.0f,1.0f);
    Call(NAT::SET_TEXT_JUSTIFICATION,justify);
    Call(NAT::SET_TEXT_RIGHT_JUSTIFY,0);
    Call(NAT::SET_TEXT_DROPSHADOW,0,0,0,0,0);
    Call(NAT::BEGIN_TEXT_COMMAND_DISPLAY_TEXT,"STRING");
    Call(NAT::ADD_TEXT_COMPONENT_SUBSTRING,s.c_str());
    Call(NAT::END_TEXT_COMMAND_DISPLAY_TEXT,x,y);
}

// ---------- pages ----------
static Menu* g_m=nullptr;
static uint64_t HostNative(uint32_t o156,uint32_t o157,const uint64_t* a,int n){ NativeOff o{o156,o157}; return NativeInvoke(o,a,n); }
static HostApi g_host;

static Row Sub(const char* l,const char* ic,int page){ Row r; r.label=l; r.icon=ic; r.type=RowType::Submenu; r.submenu=page; return r; }

void Menu::Init(){
    g_m=this; NativesInit(); LoadConfig(cfg);
    g_host={1,HostNative,[](float x,float y,float w,float h,int r,int g,int b,int a){ g_m->Rect(x,y,w,h,r,g,b,a); },
            [](const char* s,float x,float y,float sc,int r,int g,int b,int a){ g_m->Text(s,x,y,sc,r,g,b,a,0,1); },
            [](const char* m){ g_m->Notify(m); }};
    scripts.Scan(); addons.Scan(); music.Init();
    BuildPages();
    ps4::Log("Menu::Init done");
    Notify("menu_clone loaded - press L1 + Circle");
    msgFrames=420;
}
void Menu::Shutdown(){ scripts.UnloadAll(); music.Shutdown(); }

// 0 root | 1 Scripts | 2 Add-ons | 3 Music | 4 Settings
void Menu::BuildPages(){
    pages.clear(); pages.resize(5);
    pages[0].title="16 MENU";
    pages[0].rows={ Sub("Scripts","shop_makeup_icon_a",1), Sub("Add-ons / RPF","shop_garage_icon_a",2), Sub("Music Player","shop_franklin_icon_a",3), Sub("Settings","shop_mask_icon_a",4) };
    BuildScripts(); BuildAddons(); BuildMusic();
    pages[4].title="SETTINGS"; pages[4].parent=0;
    { Row r; r.label="Show music widget"; r.type=RowType::Toggle; r.toggle=&music.showWidget; pages[4].rows.push_back(r); }
    { Row r; r.label="Reload config"; r.onSelect=[this]{ LoadConfig(cfg); Notify("Config reloaded"); }; pages[4].rows.push_back(r); }
}
void Menu::BuildScripts(){
    Page& p=pages[1]; p=Page(); p.title="SCRIPTS"; p.parent=0;
    { Row r; r.label="Rescan folder"; r.onSelect=[this]{ pendingRebuild=1; Notify("Scripts rescanned"); }; p.rows.push_back(r); }
    for(int i=0;i<(int)scripts.list.size()&&i<60;i++){ scriptOn[i]=scripts.list[i].on;
        Row r; r.label=scripts.list[i].name; r.type=RowType::Toggle; r.toggle=&scriptOn[i];
        r.onSelect=[this,i]{ // toggle already flipped by Input()
            if(scriptOn[i]){ if(!scripts.Load(i,&g_host)){ scriptOn[i]=false; Notify("Script failed to load"); } else Notify("Script loaded"); }
            else { scripts.Unload(i); Notify("Script unloaded"); } };
        p.rows.push_back(r); }
    if(scripts.list.empty()){ Row r; r.label="(no scripts in /data/16menu/scripts)"; p.rows.push_back(r); }
}
void Menu::BuildAddons(){
    Page& p=pages[2]; p=Page(); p.title="ADD-ONS / RPF"; p.parent=0;
    { Row r; r.label="Rescan"; r.onSelect=[this]{ pendingRebuild=2; Notify("Add-ons rescanned"); }; p.rows.push_back(r); }
    for(int i=0;i<(int)addons.rpfs.size()&&i<60;i++){ addonOn[i]=addons.rpfs[i].on;
        Row r; r.label=addons.rpfs[i].name; r.icon="shop_garage_icon_a"; r.type=RowType::Toggle; r.toggle=&addonOn[i];
        r.onSelect=[this,i]{ addons.rpfs[i].on=addonOn[i]; addons.SaveState(); Notify(addonOn[i]?"RPF ON (restart game to apply)":"RPF OFF (restart game)"); };
        p.rows.push_back(r); }
    for(auto& d:addons.dlcPacks){ Row r; r.label="DLC pack: "+d; p.rows.push_back(r); }   // read from /data/orbx/gta/dlclist.xml
    if(addons.rpfs.empty()&&addons.dlcPacks.empty()){ Row r; r.label="(drop .rpf in /data/16menu/addons)"; p.rows.push_back(r); }
}
void Menu::BuildMusic(){
    Page& p=pages[3]; p=Page(); p.title="MUSIC PLAYER"; p.parent=0;
    { Row r; r.label="Play / Pause"; r.onSelect=[this]{ music.TogglePause(); }; p.rows.push_back(r); }
    { Row r; r.label="Next";          r.onSelect=[this]{ music.Next(); };        p.rows.push_back(r); }
    { Row r; r.label="Previous";      r.onSelect=[this]{ music.Prev(); };        p.rows.push_back(r); }
    { Row r; r.label="Stop";          r.onSelect=[this]{ music.Stop(); };        p.rows.push_back(r); }
    { Row r; r.label="Volume"; r.type=RowType::List; r.options={"0","10","20","30","40","50","60","70","80","90","100"}; r.optIndex=&vol;
      r.onSelect=[this]{ music.SetVolume(vol*10); }; p.rows.push_back(r); }
    { Row r; r.label="Shuffle"; r.type=RowType::Toggle; r.toggle=&music.shuffle; p.rows.push_back(r); }
    { Row r; r.label="Repeat all"; r.type=RowType::Toggle; r.toggle=&music.repeatAll; p.rows.push_back(r); }
    { Row r; r.label="Rescan tracks"; r.onSelect=[this]{ pendingRebuild=3; Notify("Tracks rescanned"); }; p.rows.push_back(r); }
    auto t=music.Titles(); for(int i=0;i<(int)t.size()&&i<80;i++){ Row r; r.label=t[i]; r.icon="shop_tick_icon"; r.onSelect=[this,i]{ music.Play(i); }; p.rows.push_back(r); }
}

// ---------- input ----------
// Pad natives are called as (0, control). 187=DOWN 188=UP 189=LEFT 190=RIGHT 201=ACCEPT(X) 202=CANCEL(Circle) 205=LB(L1).
// We have three such natives (PAD_A/B/C) and are not sure which one means "held": PadTracker (pad.h) learns it while you play
// (hold any button for a moment). Open/close: hold L1 and press Circle. While L1 is held the menu ignores navigation.
void Menu::Input(){
    static const int ids[CTRL_N] = { 201, 202, 187, 188, 189, 190, 205 };       // order = enum in pad.h
    static const NativeOff* nat[PAD_N] = { &NAT::PAD_A, &NAT::PAD_B, &NAT::PAD_C };
    static int cooldown = 0;
    if(!pad.log) pad.log = ps4::Log;

    bool raw[PAD_N][CTRL_N];
    for(int n=0;n<PAD_N;n++) for(int c=0;c<CTRL_N;c++) raw[n][c] = Call(*nat[n],0,ids[c])!=0;
    pad.Update(ps4::TimeUs(), raw, ids);
    const bool lb = pad.lb;

    // open / close with L1 + Circle (the same press must NOT also act as "Back")
    if(lb && pad.edge[C_BACK]){
        open = !open; cooldown = 20;
        ps4::Log(open ? "menu opened" : "menu closed");
        PopupNotify(open ? "menu_clone: menu OPENED" : "menu_clone: menu closed");
        Notify(open ? "Menu OPENED - menu_clone is working" : "Menu closed"); msgFrames = 300;
    }
    if(cooldown > 0){ cooldown--; return; }

    // Music hotkeys with the menu closed: L1 + D-pad  (Right = next, Left = previous, Down = play/pause)
    if(lb && !open){
        if(pad.edge[C_RIGHT]) music.Next(); else if(pad.edge[C_LEFT]) music.Prev(); else if(pad.edge[C_DOWN]) music.TogglePause();
        return;
    }
    if(!open || lb) return;

    Page& p = pages[page]; int n = (int)p.rows.size(); if(n == 0) return;
    if(sel >= n) sel = 0;
    if(pad.edge[C_UP])   sel = (sel+n-1)%n;
    if(pad.edge[C_DOWN]) sel = (sel+1)%n;
    Row& r = p.rows[sel];
    if(pad.edge[C_ACCEPT]){
        if(r.type==RowType::Submenu && r.submenu>=0){ page=r.submenu; sel=0; scroll=0; }
        else if(r.type==RowType::Toggle && r.toggle) *r.toggle=!*r.toggle;
        if(r.onSelect && r.type!=RowType::List) r.onSelect();
        return;   // callbacks may schedule a page rebuild; never touch 'r' after them
    }
    if(r.type==RowType::List && r.optIndex){ int m=(int)r.options.size(); bool ch=false;
        if(pad.edge[C_LEFT])  { *r.optIndex=(*r.optIndex+m-1)%m; ch=true; }
        if(pad.edge[C_RIGHT]) { *r.optIndex=(*r.optIndex+1)%m;   ch=true; }
        if(ch && r.onSelect) r.onSelect(); }
    if(pad.edge[C_BACK]){ if(p.parent>=0){ page=p.parent; sel=0; scroll=0; } else { open=false; Notify("Menu closed"); msgFrames=120; } }
}

// ---------- drawing ----------
void Menu::Draw(){
    static bool first=true; if(first) ps4::Log("draw: first Draw() start");
    Page& p=pages[page]; float x=cfg.x,w=cfg.width,rh=cfg.rowHeight,top=0.10f;
    if(safeMode) Rect(x,top+cfg.bannerHeight/2,w,cfg.bannerHeight,190,30,110,235);      // plain banner (no texture)
    else         Sprite("commonmenu","header_gradient_script",x,top+cfg.bannerHeight/2,w,cfg.bannerHeight,0);
    Text(p.title,x-w/2+0.01f,top+cfg.bannerHeight*0.30f,0.9f,255,255,255,255,1);
    if(first) ps4::Log("draw: banner done");
    float y=top+cfg.bannerHeight+cfg.titleGap; int n=(int)p.rows.size(), vis=n<cfg.maxVisible?n:cfg.maxVisible;
    if(sel<scroll)scroll=sel; if(sel>=scroll+vis)scroll=sel-vis+1;
    for(int i=0;i<vis;i++){
        int idx=scroll+i; Row& r=p.rows[idx]; float cy=y+i*rh+rh/2; bool s=(idx==sel);
        if(s) Rect(x,cy,w,rh,255,255,255,255); else Rect(x,cy,w,rh,6,4,20,105);    // panel colour (6,4,20,105) from the original
        int tc=s?0:255;
        if(r.icon) Sprite("commonmenu",r.icon,x-w/2+0.014f,cy,cfg.iconW,cfg.iconH,0,tc,tc,tc,255);
        Text(r.label,x-w/2+(safeMode?0.010f:0.030f),cy+cfg.textYOffset,cfg.textScale,tc,tc,tc,255,0);
        if(r.type==RowType::Submenu){
            if(safeMode) Text(">",x+w/2-0.012f,cy+cfg.textYOffset,cfg.textScale,tc,tc,tc,255,0);
            else Sprite("commonmenu","arrowright",x+w/2-0.012f,cy,0.012f,0.02f,0,tc,tc,tc,255);
        }
        if(r.type==RowType::Toggle&&r.toggle){
            if(safeMode) Text(*r.toggle?"ON":"OFF",x+w/2-0.032f,cy+cfg.textYOffset,cfg.textScale,tc,tc,tc,255,0);
            else if(*r.toggle) Sprite("commonmenu","shop_tick_icon",x+w/2-0.014f,cy,cfg.iconW,cfg.iconH,0,tc,tc,tc,255);
        }
        if(r.type==RowType::List&&r.optIndex){ char b[96]; snprintf(b,sizeof b,"< %s >",r.options[*r.optIndex].c_str()); Text(b,x+w/2-0.08f,cy+cfg.textYOffset,cfg.textScale,tc,tc,tc,255,0);}
    }
    if(first) ps4::Log("draw: rows done");
    float fy=y+vis*rh+rh/2; Rect(x,fy,w,rh,6,4,20,105);
    char cnt[24]; snprintf(cnt,sizeof cnt,"%i / %i",sel+1,n); Text(cnt,x+w/2-0.06f,fy+cfg.textYOffset,cfg.textScale,255,255,255,255,0);
    Sprite("commonmenu","shop_arrows_upanddown",x,fy,0.02f,0.03f,0);   // 0.02 x 0.03 from the original (skipped in safe mode)
    if(safeMode){                                                        // text footer instead of the scaleform buttons
        Rect(x,fy+rh,w,rh,6,4,20,105);
        Text("X: Select    D-pad: Scroll    O: Back",x-w/2+0.010f,fy+rh+cfg.textYOffset,0.28f,255,255,255,255,0);
    }
    if(first) ps4::Log("draw: footer next");
    DrawFooter();
    if(first){ ps4::Log("draw: first Draw() finished without crash"); first=false; }
}

// Footer = scaleform "instructional_buttons". Sequence taken from VA 0x11320-0x11120:
//   REQUEST_SCALEFORM_MOVIE -> BEGIN "CLEAR_ALL" END -> BEGIN "SET_CLEAR_SPACE"(200) END -> SET_DATA_SLOT x3
//   -> BEGIN "DRAW_INSTRUCTIONAL_BUTTONS" END -> BEGIN "SET_BACKGROUND_COLOUR"(0,0,0,..) END -> DRAW_SCALEFORM_MOVIE_FULLSCREEN
void Menu::DrawFooter(){
    if(safeMode) return;            // scaleform natives need the game's own script thread
    if(!movie) movie=(uint32_t)Call(NAT::REQUEST_SCALEFORM_MOVIE,"instructional_buttons");
    auto begin=[&](const char* m){ Call(NAT::BEGIN_SCALEFORM_MOVIE_METHOD,movie,m); };
    auto end  =[&]{ Call(NAT::END_SCALEFORM_MOVIE_METHOD); };
    auto num  =[&](int v){ Call(NAT::SCALEFORM_PARAM_INT,v); };
    auto str  =[&](const char* s){ Call(NAT::BEGIN_TEXT_COMMAND_SCALEFORM_STR,"STRING"); Call(NAT::ADD_TEXT_COMPONENT_SUBSTRING,s); Call(NAT::END_TEXT_COMMAND_SCALEFORM_STR); };
    begin("CLEAR_ALL"); end();
    begin("SET_CLEAR_SPACE"); num(200); end();
    const struct{int slot,btn;const char* lbl;} S[]={{0,201,"Select"},{1,188,"Scroll"},{2,202,"Back"}};   // btn ids = pad controls (UNVERIFIED: see README)
    for(auto& s:S){ begin("SET_DATA_SLOT"); num(s.slot); num(s.btn); str(s.lbl); end(); }
    begin("DRAW_INSTRUCTIONAL_BUTTONS"); end();
    begin("SET_BACKGROUND_COLOUR"); num(0); num(0); num(0); num(80); end();
    Call(NAT::DRAW_SCALEFORM_MOVIE_FULLSCREEN,movie,255,255,255,0,0);
}

// Now-playing widget, drawn EVERY frame (menu open or closed). Look copied from the original:
// dark pill, top centre; 4 pink equaliser bars on the left, elapsed time on the right, thin pink progress line at the bottom.
void Menu::DrawWidget(){
    if(!music.showWidget || (!music.Playing() && !music.Paused())) return;
    const float cx=0.5f, cy=0.040f, w=0.16f, h=0.050f;               // 16:9 screen, ratio ~5.7:1 like the screenshot
    const float left=cx-w/2, right=cx+w/2, bottom=cy+h/2;

    // pill body = 20 horizontal strips whose ends follow a circle (no sqrt needed: precomputed insets)
    static const float inset[20]={0.6878f,0.4732f,0.3386f,0.2401f,0.1648f,0.1070f,0.0633f,0.0318f,0.0113f,0.0013f,0.0013f,0.0113f,0.0318f,0.0633f,0.1070f,0.1648f,0.2401f,0.3386f,0.4732f,0.6878f};
    const float rx=(h/2)*0.5625f, sh=h/20.0f;                         // horizontal corner radius corrected for 16:9
    for(int i=0;i<20;i++){ float sw=w-2.0f*rx*inset[i]; float sy=cy-h/2+sh*(i+0.5f); Rect(cx,sy,sw,sh,12,9,20,232); }

    // equaliser bars (animate while playing, flat while paused)
    const bool playing = music.Playing() && !music.Paused();
    const float pos0 = music.Position();
    static const float spd[4]={2.3f,3.1f,1.7f,2.7f}, ph[4]={0.0f,0.35f,0.7f,0.15f};
    const float baseY = cy+0.009f;
    for(int i=0;i<4;i++){
        float u=pos0*spd[i]+ph[i]; u-=(float)(int)u; float tri=(u<0.5f)?u*2.0f:2.0f-2.0f*u;
        float bh = playing ? (0.006f+0.016f*tri) : 0.006f;
        Rect(left+0.0150f+i*0.0052f, baseY-bh/2.0f, 0.0030f, bh, 255,70,170,255);
    }

    // progress line (bottom) : faint track + pink fill
    float dur=music.Duration(), pos=pos0; if(pos>dur) pos=dur; float frac=dur>0?pos/dur:0.0f;
    const float x0=left+0.014f, x1=right-0.014f, len=x1-x0, py=bottom-0.0085f;
    Rect(x0+len/2.0f,py,len,0.0030f,255,255,255,45);
    if(frac>0.0f) Rect(x0+len*frac/2.0f,py,len*frac,0.0030f,255,70,170,255);

    // elapsed time, right side
    char t[16]; int ps=(int)pos; snprintf(t,sizeof t,"%d:%02d",ps/60,ps%60);
    float tw=0.0058f*(float)strlen(t);
    Text(t,right-0.014f-tw,cy-0.0135f,0.30f,255,255,255,235,0,1);
}
void Menu::Tick(){
    static bool firstTick=true; if(firstTick){ firstTick=false; ps4::Log("first Tick: the per-frame call is working"); }
    if(pendingRebuild){ if(pendingRebuild==1){ scripts.Scan(); BuildScripts(); } else if(pendingRebuild==2){ addons.Scan(); BuildAddons(); } else { music.Scan(); BuildMusic(); }
        pendingRebuild=0; sel=0; scroll=0; }
    static int stage=0;
    if(stage==0) ps4::Log("tick stage 1: before Input() (first game native call is inside)");
    Input();
    if(stage==0) ps4::Log("tick stage 2: Input() returned (pad natives OK)");
    if(open) Draw(); DrawWidget(); scripts.Tick();
    if(stage==0) ps4::Log("tick stage 3: before message box (DRAW_RECT / text natives)");
    if(msgFrames>0){ msgFrames--;                                   // message box, bottom centre, visible with the menu open or closed
        Rect(0.5f,0.915f,0.34f,0.045f,6,4,20,200);
        Text(msg,0.5f,0.915f-0.014f,0.36f,255,255,255,255,0,0); }
    if(stage==0){ ps4::Log("tick stage 4: first full tick finished without crash"); stage=1; }
}
