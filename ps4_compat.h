#pragma once
// Self-contained declarations of the PS4 system calls we use. They are bound to the REAL symbol names with asm labels,
// so we do not depend on header type names (ScePthread vs OrbisPthread ...) of any SDK version.
#include <cstddef>
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
    int  AudioInit(void)                                                                                 __asm__("sceAudioOutInit");
    int  AudioOpen(int user, int type, int index, unsigned len, unsigned freq, unsigned param)           __asm__("sceAudioOutOpen");
    int  AudioOutput(int handle, const void* buf)                                                        __asm__("sceAudioOutOutput");
    int  AudioClose(int handle)                                                                          __asm__("sceAudioOutClose");
    int  KOpen(const char* path, int flags, int mode)                                                    __asm__("sceKernelOpen");
    int  KClose(int fd)                                                                                  __asm__("sceKernelClose");
    int  KGetdents(int fd, char* buf, int nbytes)                                                        __asm__("sceKernelGetdents");

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
