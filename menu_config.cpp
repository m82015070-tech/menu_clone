#include "menu_config.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
// Keep every other line of the original file untouched when saving.
bool LoadConfig(MenuConfig& c,const char* path){
    FILE* f=fopen(path,"r"); if(!f) return false; char line[160];
    while(fgets(line,sizeof line,f)){
        char* eq=strchr(line,'='); if(!eq) continue; *eq=0; const char* k=line; const char* v=eq+1;
        if      (!strcmp(k,"k289da8ac4e")) c.theme=atoi(v);
        else if (!strcmp(k,"k1f26d48fd6")) c.useCustomTex=atoi(v);
        else if (!strcmp(k,"k176e0b10c8")) c.iconW=(float)atof(v);
        else if (!strcmp(k,"k2188554c7f")) c.iconH=(float)atof(v);
        else if (!strcmp(k,"k8f2266446f")) c.textYOffset=(float)atof(v);
        else if (!strcmp(k,"k21520243e1")) c.titleGap=(float)atof(v);
        else if (!strcmp(k,"k70ac87f32d")) c.textScale=(float)atof(v);
        else if (!strcmp(k,"k82e7e3b5f7")) c.bannerHeight=(float)atof(v);
        else if (!strcmp(k,"kce865a7aea")) c.extraA=atoi(v);
        else if (!strcmp(k,"k5171a83a6a")) c.extraB=atoi(v);
    }
    fclose(f); return true;
}
bool SaveConfig(const MenuConfig& c,const char* path){
    std::vector<std::string> lines; if(FILE* f=fopen(path,"r")){ char b[160]; while(fgets(b,sizeof b,f)) lines.push_back(b); fclose(f);}    
    auto set=[&](const char* k,const char* val){ std::string pre=std::string(k)+"="; for(auto& l:lines) if(l.rfind(pre,0)==0){ l=pre+val+"\n"; return;} lines.push_back(pre+val+"\n"); };
    char t[32];
    snprintf(t,32,"%d",c.theme); set("k289da8ac4e",t); snprintf(t,32,"%d",c.useCustomTex); set("k1f26d48fd6",t);
    snprintf(t,32,"%f",c.iconW); set("k176e0b10c8",t); snprintf(t,32,"%f",c.iconH); set("k2188554c7f",t);
    snprintf(t,32,"%f",c.textYOffset); set("k8f2266446f",t); snprintf(t,32,"%f",c.titleGap); set("k21520243e1",t);
    snprintf(t,32,"%f",c.textScale); set("k70ac87f32d",t); snprintf(t,32,"%f",c.bannerHeight); set("k82e7e3b5f7",t);
    FILE* f=fopen(path,"w"); if(!f) return false; for(auto& l:lines) fputs(l.c_str(),f); fclose(f); return true;
}
