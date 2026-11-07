#pragma once
// Reads the REAL settings.ini of the original menu (hashed keys), so your existing file works as-is.
// Order written by the original: 2 ints, 6 floats, 2 ints (then many other script settings we ignore).
// Meaning of the six floats is inferred from your file's values + the 6 sliders in "Reglages":
//   k176e0b10c8=0.022 iconW | k2188554c7f=0.030 iconH | k8f2266446f=-0.0125 textYOffset
//   k21520243e1=0.014 titleGap | k70ac87f32d=0.32 textScale | k82e7e3b5f7=0.11 bannerHeight
// If something looks swapped on screen, just swap the assignments in config.cpp (one line each).
struct MenuConfig {
    int   theme=3, useCustomTex=0, extraA=3, extraB=1;
    float iconW=0.022f, iconH=0.030f, textYOffset=-0.0125f, titleGap=0.014f, textScale=0.32f, bannerHeight=0.11f;
    // fixed look, from the draw code (VA 0xf5e0-0x10b00): panel colour (6,4,20,105), row text white, arrows 0.02x0.03
    float x=0.145f, width=0.225f, rowHeight=0.034f; int maxVisible=10;
};
bool LoadConfig(MenuConfig& c, const char* path="/data/16menu/config/settings.ini");
bool SaveConfig(const MenuConfig& c, const char* path="/data/16menu/config/settings.ini");
