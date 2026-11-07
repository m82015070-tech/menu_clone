#pragma once
// ============================================================================
// Native invoker - reverse-engineered from the working menu (menu.sprx).
//
// How the original calls a GTA native (VA 0x1710 reset / 0x16d0 invoke / 0x1680 copy-back):
//   * One global context  ctx { u64* ret; u32 argCount; u64* args; u32 dataCount; ... }
//     ret and args both point to the same 8-byte-slot buffer.
//   * reset  : argCount = 0; dataCount = 0; ret = args = buffer
//   * push   : args[argCount++] = value   (floats stored as 32-bit, ints 32-bit, pointers 64-bit)
//   * invoke : handler = EBOOT_BASE(0x400000) + offset ; handler(&ctx)
//   * result : ((u32*)ctx.ret)[0]  (vector results need the copy-back table, not used by the menu UI)
// The offset depends on the game version (the original keeps it in a global: 0x9c=1.56, 0x9d=1.57,
// chosen from /data/16menu/config/force_version.txt "1.56"/"1.57").
// ============================================================================
#include <cstdint>
#include <cstring>

enum class GameVer : uint32_t { V156 = 0x9c, V157 = 0x9d };

struct NativeOff { uint32_t v156; uint32_t v157; };

// Offsets taken verbatim from the call sites in menu.sprx. Names are inferred from argument shapes
// and neighbouring strings - the OFFSETS are exact, a name may be slightly off (see comments).
namespace NAT {
    // --- textures / sprites / rects
    constexpr NativeOff DRAW_RECT                        {0x9c9180,0x9ccac0}; // x,y,w,h,r,g,b,a
    constexpr NativeOff REQUEST_STREAMED_TEXTURE_DICT    {0x9c9030,0x9cc970}; // dict,0
    constexpr NativeOff HAS_STREAMED_TEXTURE_DICT_LOADED {0x9c90c0,0x9cca00}; // dict
    constexpr NativeOff DRAW_SPRITE                      {0x9c9390,0x9cccd0}; // dict,name,x,y,w,h,heading,r,g,b,a
    // --- text
    constexpr NativeOff SET_TEXT_FONT                    {0x9dde00,0x9e19f0}; // font
    constexpr NativeOff SET_TEXT_SCALE                   {0x9ddce0,0x9e18d0}; // sx,sy
    constexpr NativeOff SET_TEXT_COLOUR                  {0x9ddd00,0x9e18f0}; // r,g,b,a
    constexpr NativeOff SET_TEXT_WRAP                    {0x9dddb0,0x9e19a0}; // 0.0, 1.0
    constexpr NativeOff SET_TEXT_JUSTIFICATION           {0x9ddd40,0x9e1930}; // int  (alignment)
    constexpr NativeOff SET_TEXT_RIGHT_JUSTIFY           {0x9ddd60,0x9e1950}; // bool (called with 0)
    constexpr NativeOff SET_TEXT_DROPSHADOW              {0x9dde30,0x9e1a20}; // 0,0,0,0,0
    constexpr NativeOff BEGIN_TEXT_COMMAND_DISPLAY_TEXT  {0x9dcaf0,0x9e0690}; // "STRING"
    constexpr NativeOff ADD_TEXT_COMPONENT_SUBSTRING     {0x9dcd50,0x9e08f0}; // const char*
    constexpr NativeOff END_TEXT_COMMAND_DISPLAY_TEXT    {0x9dcb20,0x9e06c0}; // x,y
    // --- scaleform (footer "instructional_buttons")
    constexpr NativeOff REQUEST_SCALEFORM_MOVIE          {0x9cd270,0x9d0cc0}; // "instructional_buttons"
    constexpr NativeOff DRAW_SCALEFORM_MOVIE_FULLSCREEN  {0x9cd7a0,0x9d11f0}; // movie,255,255,255,0,0
    constexpr NativeOff BEGIN_SCALEFORM_MOVIE_METHOD     {0x9cdc80,0x9d16d0}; // movie,"METHOD"
    constexpr NativeOff END_SCALEFORM_MOVIE_METHOD       {0x9cde20,0x9d1870};
    constexpr NativeOff SCALEFORM_PARAM_INT              {0x9cdf40,0x9d1990}; // int
    constexpr NativeOff BEGIN_TEXT_COMMAND_SCALEFORM_STR {0x9cdf90,0x9d19e0}; // "STRING"
    constexpr NativeOff END_TEXT_COMMAND_SCALEFORM_STR   {0x9cdfa0,0x9d19f0};
    // --- pad (same call shape as the menu's open/close logic: (0, control))
    constexpr NativeOff PAD_A                            {0xa66340,0xa6a9c0}; // (0,ctrl) probably IS_DISABLED_CONTROL_JUST_PRESSED
    constexpr NativeOff PAD_B                            {0xa66200,0xa6a880}; // (0,ctrl) probably IS_DISABLED_CONTROL_PRESSED
    constexpr NativeOff PAD_C                            {0xa663e0,0xa6aa60}; // (0,ctrl) probably IS_DISABLED_CONTROL_JUST_RELEASED
}

struct NativeCtx {              // matches the original's global at VA 0x123d1a0
    uint64_t* ret;              // +0x00
    uint32_t  argCount;         // +0x08
    uint32_t  _pad0;
    uint64_t* args;             // +0x10
    uint32_t  dataCount;        // +0x18
    uint32_t  _pad1;
    uint64_t  copyBack[32];     // +0x20 (unused for UI natives)
};

void     NativesInit();                         // reads force_version.txt, sets version
void     SetGameVersion(GameVer v);
uint64_t NativeInvoke(NativeOff off, const uint64_t* a, int n);

// ---- argument packing: floats = 32-bit pattern, ints = 32-bit, pointers = 64-bit
inline uint64_t A(float f)        { uint32_t u; memcpy(&u,&f,4); return u; }
inline uint64_t A(int i)          { return (uint32_t)i; }
inline uint64_t A(unsigned i)     { return i; }
inline uint64_t A(bool b)         { return b ? 1u : 0u; }
inline uint64_t A(const char* s)  { return (uint64_t)s; }
template<typename... T> inline uint64_t Call(NativeOff off, T... t){ uint64_t a[sizeof...(T)+1]={A(t)...}; return NativeInvoke(off,a,(int)sizeof...(T)); }
