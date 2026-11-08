#include "natives.h"
#include "ps4_compat.h"
#include <cstdio>
static GameVer g_ver = GameVer::V157;
static NativeCtx g_ctx;
static uint64_t  g_buf[64];
static const uintptr_t EBOOT_BASE = 0x400000;   // original: rax += 0x400000

void SetGameVersion(GameVer v){ g_ver=v; }
void NativesInit(){
    // Original: /data/16menu/config/force_version.txt containing "1.56" or "1.57"
    FILE* f=fopen("/data/16menu/config/force_version.txt","r");
    if(f){ char b[16]={0}; fgets(b,sizeof b,f); fclose(f);
        if(!strncmp(b,"1.56",4)) g_ver=GameVer::V156; else if(!strncmp(b,"1.57",4)) g_ver=GameVer::V157; }
    ps4::Log(g_ver==GameVer::V156 ? "natives: using game version 1.56 offsets" : "natives: using game version 1.57 offsets");
    FILE* chk=fopen("/data/16menu/config/force_version.txt","r");
    if(chk){ fclose(chk); ps4::Log("natives: force_version.txt FOUND"); } else ps4::Log("natives: force_version.txt NOT found (defaulting to 1.57!)");
}
uint64_t NativeInvoke(NativeOff off,const uint64_t* a,int n){
    memset(g_buf,0,sizeof g_buf);
    g_ctx.ret=g_buf; g_ctx.args=g_buf; g_ctx.argCount=0; g_ctx.dataCount=0;
    for(int i=0;i<n;i++) g_ctx.args[g_ctx.argCount++]=a[i];
    uint32_t o = (g_ver==GameVer::V157)? off.v157 : off.v156;
    auto handler=(void(*)(NativeCtx*))(EBOOT_BASE+o);
    handler(&g_ctx);
    return g_buf[0];
}

uint64_t NativeAddr(NativeOff off){ return (uint64_t)EBOOT_BASE + ((g_ver==GameVer::V157)? off.v157 : off.v156); }
bool NativeByName(const char* name, NativeOff& out){
    struct E{ const char* n; NativeOff o; };
    static const E t[]={
        {"DRAW_RECT",NAT::DRAW_RECT},{"DRAW_SPRITE",NAT::DRAW_SPRITE},
        {"REQUEST_TXD",NAT::REQUEST_STREAMED_TEXTURE_DICT},{"HAS_TXD",NAT::HAS_STREAMED_TEXTURE_DICT_LOADED},
        {"SET_TEXT_FONT",NAT::SET_TEXT_FONT},{"SET_TEXT_SCALE",NAT::SET_TEXT_SCALE},{"SET_TEXT_COLOUR",NAT::SET_TEXT_COLOUR},
        {"BEGIN_TEXT",NAT::BEGIN_TEXT_COMMAND_DISPLAY_TEXT},{"END_TEXT",NAT::END_TEXT_COMMAND_DISPLAY_TEXT},
        {"PAD_A",NAT::PAD_A},{"PAD_B",NAT::PAD_B},{"PAD_C",NAT::PAD_C}};
    for(const E& e:t) if(!strcmp(e.n,name)){ out=e.o; return true; }
    return false;
}
