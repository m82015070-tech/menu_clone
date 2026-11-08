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
