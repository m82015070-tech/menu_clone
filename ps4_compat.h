#pragma once
// Self-contained declarations of the PS4 system calls we use. They are bound to the REAL symbol names with asm labels,
// so we do not depend on header type names (ScePthread vs OrbisPthread ...) of any SDK version.
#include <cstddef>
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
}
