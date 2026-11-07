#pragma once
// ===== API for external scripts (.prx/.sprx in /data/16menu/scripts/) =====
// A script is a PRX that exports (extern "C"):
//     void script_init(const HostApi* host);   // optional, called once after load
//     void script_tick();                      // optional, called every frame while the script is ON
//     void script_stop();                      // optional, called on unload
#include <cstdint>
struct HostApi {
    uint32_t version;                                                   // = 1
    uint64_t (*native)(uint32_t off156, uint32_t off157, const uint64_t* args, int n); // raw native call
    void (*rect)(float x,float y,float w,float h,int r,int g,int b,int a);
    void (*text)(const char* s,float x,float y,float scale,int r,int g,int b,int a);
    void (*notify)(const char* msg);                                    // shows text in the menu's message line
};
