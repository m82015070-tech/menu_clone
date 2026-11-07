#include "addons.h"
#include <dirent.h>
#include <strings.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
static const char* kDir="/data/16menu/addons";
static const char* kState="/data/16menu/config/addons.ini";
static const char* kDlc="/data/orbx/gta/dlclist.xml";
static char g_tmp[512];
static bool IsOn(const std::string& n){
    FILE* f=fopen(kState,"r"); if(!f) return false; char l[256]; bool on=false;
    while(fgets(l,sizeof l,f)){ char* e=strchr(l,'='); if(!e) continue; *e=0; if(n==l){ on=atoi(e+1)!=0; break; } }
    fclose(f); return on;
}
void AddonManager::Scan(){
    rpfs.clear(); dlcPacks.clear();
    if(DIR* d=opendir(kDir)){
        while(dirent* e=readdir(d)){ std::string n=e->d_name; if(n.size()<5||strcasecmp(n.c_str()+n.size()-4,".rpf")) continue;
            Addon a; a.name=n; a.path=std::string(kDir)+"/"+n; a.on=IsOn(n); rpfs.push_back(a); }
        closedir(d);
        std::sort(rpfs.begin(),rpfs.end(),[](const Addon&a,const Addon&b){return a.name<b.name;});
    }
    // dlclist.xml: collect <Item>dlcpacks:/NAME/</Item>
    if(FILE* f=fopen(kDlc,"r")){ char l[512];
        while(fgets(l,sizeof l,f)){ char* p=strstr(l,"dlcpacks:/"); if(!p) continue; p+=10; char* q=strchr(p,'/'); if(q){ *q=0; dlcPacks.push_back(p);} }
        fclose(f); }
}
void AddonManager::SaveState(){ FILE* f=fopen(kState,"w"); if(!f) return; for(auto& a:rpfs) fprintf(f,"%s=%d\n",a.name.c_str(),a.on?1:0); fclose(f); }
const char* AddonManager::Redirect(const char* gp){
    if(!gp) return gp; size_t n=strlen(gp);
    for(auto& a:rpfs){ if(!a.on) continue; size_t m=a.name.size();
        if(n>=m && strcasecmp(gp+n-m,a.name.c_str())==0 && (n==m||gp[n-m-1]=='/')){ snprintf(g_tmp,sizeof g_tmp,"%s",a.path.c_str()); return g_tmp; } }
    return gp;
}
