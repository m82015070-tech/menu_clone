#pragma once
#include <string>
#include <vector>
// Add-ons = (1) .rpf files dropped in /data/16menu/addons/ that REPLACE the game file with the same name
//           (same mechanism the original uses for scaleform_platform_ps4.rpf),
//           (2) DLC packs listed in /data/orbx/gta/dlclist.xml (read-only list, mounting is done by your loader, like the original).
struct Addon { std::string name, path; bool on=false; };
class AddonManager {
public:
    std::vector<Addon> rpfs; std::vector<std::string> dlcPacks;
    void Scan(); void SaveState();
    // Returns replacement path if `gamePath` ends with an enabled addon's file name, else gamePath.
    const char* Redirect(const char* gamePath);
};
