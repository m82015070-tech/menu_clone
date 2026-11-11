#pragma once
// Self-contained declarations of the PS4 system calls we use. They are bound to the REAL symbol names with asm labels,
// so we do not depend on header type names (ScePthread vs OrbisPthread ...) of any SDK version.
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace ps4 {
    int  ThreadCreate(void** thread, const void* attr, void* (*entry)(void*), void* arg, const char* name) __asm__("scePthreadCreate");
    int  ThreadJoin(void* thread, void** ret)                                                             __asm__("scePthreadJoin");
    int  Usleep(unsigned usec)                                                                           __asm__("sceKernelUsleep");
    int  LoadStartModule(const char* path, size_t argc, const void* argv, unsigned flags, const void* opt, int* res) __asm__("sceKernelLoadStartModule");
    int  StopUnloadModule(int handle, size_t argc, const void* argv, unsigned flags, const void* opt, int* res)     __asm__("sceKernelStopUnloadModule");
    int  Dlsym(int handle, const char* name, void** addr)                                                __asm__("sceKernelDlsym");
    // Audio is resolved when the music thread starts (NOT at plugin load): the plugin must not depend on libSceAudioOut at load time.
    struct AudioApi { int(*init)(void)=nullptr; int(*open)(int,int,int,unsigned,unsigned,unsigned)=nullptr; int(*output)(int,const void*)=nullptr; int(*close)(int)=nullptr; };
    unsigned long long TimeUs(void) __asm__("sceKernelGetProcessTime");   // microseconds since process start
    int  KMmap(void* addr, size_t len, int prot, int flags, int fd, long long off, void** res)           __asm__("sceKernelMmap");
    int  KMunmap(void* addr, size_t len)                                                                 __asm__("sceKernelMunmap");
    int  AttrInit(void** attr)                                                                           __asm__("scePthreadAttrInit");
    int  AttrSetStack(void* attr, size_t size)                                                           __asm__("scePthreadAttrSetstacksize");
    int  KOpen(const char* path, int flags, int mode)                                                    __asm__("sceKernelOpen");
    int  KClose(int fd)                                                                                  __asm__("sceKernelClose");
    int  KGetdents(int fd, char* buf, int nbytes)                                                        __asm__("sceKernelGetdents");

    // Append one line to /data/16menu/menu_clone.log (proof that the code ran, even if nothing is drawn)
    inline void Log(const char* line){ FILE* f=fopen("/data/16menu/menu_clone.log","a"); if(f){ fputs(line,f); fputs("\n",f); fclose(f);} }

    inline bool FillAudio(int h, AudioApi& a){
        void* f=nullptr;
        if(Dlsym(h,"sceAudioOutInit",&f)!=0||!f) return false;   a.init  =(int(*)(void))f; f=nullptr;
        if(Dlsym(h,"sceAudioOutOpen",&f)!=0||!f) return false;   a.open  =(int(*)(int,int,int,unsigned,unsigned,unsigned))f; f=nullptr;
        if(Dlsym(h,"sceAudioOutOutput",&f)!=0||!f) return false; a.output=(int(*)(int,const void*))f; f=nullptr;
        if(Dlsym(h,"sceAudioOutClose",&f)!=0||!f) return false;  a.close =(int(*)(int))f;
        return true;
    }
    // 1) load the library (works if the game has not loaded it), 2) otherwise look through every already loaded module.
    inline bool LoadAudio(AudioApi& a){
        static const char* paths[]={"/system/common/lib/libSceAudioOut.sprx","libSceAudioOut.sprx"};
        for(const char* p:paths){ int res=0; int h=LoadStartModule(p,0,nullptr,0,nullptr,&res); if(h>=0 && FillAudio(h,a)){ Log("audio: found via LoadStartModule"); return true; } }
        for(int h=0;h<1024;h++){ if(FillAudio(h,a)){ char b[64]; snprintf(b,sizeof b,"audio: found in already loaded module handle %d",h); Log(b); return true; } }
        Log("audio: sceAudioOut* not found anywhere"); return false;
    }

    // Executable memory for compiled scripts (read+write+exec, anonymous, private).
    inline void* ExecAlloc(size_t n){ void* r=nullptr; size_t len=(n+0x3FFF)&~(size_t)0x3FFF; if(KMmap(nullptr,len,7,0x1002,-1,0,&r)!=0) return nullptr; return r; }
    inline void  ExecFree(void* p,size_t n){ if(p) KMunmap(p,(n+0x3FFF)&~(size_t)0x3FFF); }
    // Run fn(arg) on a NEW thread with a big stack and wait for it (the compiler is recursive and needs far more stack
    // than a game script thread has). Falls back to running inline if the thread cannot be created.
    inline void RunBigStack(void* (*fn)(void*), void* arg, size_t stack){
        void* attr=nullptr; bool haveAttr = (AttrInit(&attr)==0) && attr!=nullptr;
        if(haveAttr) AttrSetStack(attr,stack);
        void* th=nullptr;
        int rc = ThreadCreate(&th, haveAttr ? (const void*)&attr : nullptr, fn, arg, "tcc_compile");
        if(rc!=0 || !th){ fn(arg); return; }
        ThreadJoin(th,nullptr);
    }

    // case-insensitive compare (no libc dependency)
    inline int StrCaseCmp(const char* a, const char* b){
        for(;;a++,b++){ int x=(unsigned char)*a, y=(unsigned char)*b; if(x>='A'&&x<='Z') x+=32; if(y>='A'&&y<='Z') y+=32; if(x!=y) return x-y; if(!x) return 0; }
    }
    // true if s ends with suffix (case-insensitive)
    inline bool EndsWithI(const std::string& s, const char* suf){ size_t m=strlen(suf); return s.size()>=m && StrCaseCmp(s.c_str()+s.size()-m,suf)==0; }

    // FreeBSD (PS4) directory entry layout
    struct DirEnt { uint32_t fileno; uint16_t reclen; uint8_t type; uint8_t namlen; char name[256]; };
    // List file names in a directory using libkernel (opendir/readdir are not available in Sony libc).
    inline bool ListDir(const char* path, std::vector<std::string>& out){
        int fd=KOpen(path,0x20000/*O_DIRECTORY*/,0); if(fd<0) return false;
        static char buf[8192]; int n;
        while((n=KGetdents(fd,buf,sizeof buf))>0){
            int off=0; while(off<n){ DirEnt* d=(DirEnt*)(buf+off); if(d->reclen==0) break;
                if(d->fileno!=0 && strcmp(d->name,".")!=0 && strcmp(d->name,"..")!=0) out.push_back(d->name);
                off+=d->reclen; }
        }
        KClose(fd); return true;
    }
}
