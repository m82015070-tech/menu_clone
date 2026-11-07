#include "music.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "ps4_compat.h"
// dr_mp3 asserts would need __assert_fail from a C library we do not link: turn them off.
#define DRMP3_ASSERT(expression) ((void)0)
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

static const char* kDir="/data/16menu/sound/mp3";
static MusicPlayer* g_self=nullptr; static void* g_th=nullptr;
static void* ThreadEntry(void*){ if(g_self) g_self->ThreadMain(); return nullptr; }

void MusicPlayer::Scan(){
    tracks.clear(); std::vector<std::string> names; if(!ps4::ListDir(kDir,names)) return;
    for(auto& n:names){ if(!ps4::EndsWithI(n,".mp3")) continue; Track t; t.title=n.substr(0,n.size()-4); t.path=std::string(kDir)+"/"+n; tracks.push_back(t); }
    std::sort(tracks.begin(),tracks.end(),[](const Track&a,const Track&b){return a.title<b.title;});
}
void MusicPlayer::Init(){ g_self=this; Scan(); quit=false; ps4::ThreadCreate(&g_th,nullptr,ThreadEntry,nullptr,"menu_music"); }
void MusicPlayer::Shutdown(){ quit=true; ps4::ThreadJoin(g_th,nullptr); }
void MusicPlayer::Play(int i){ if(tracks.empty()) return; req=(i%(int)tracks.size()+(int)tracks.size())%(int)tracks.size(); }
void MusicPlayer::TogglePause(){ if(!playing){ Play(cur>=0?cur:0); } else paused=!paused; }
void MusicPlayer::Next(){ if(tracks.empty()) return; Play(shuffle? rand()%(int)tracks.size() : cur+1); }
void MusicPlayer::Prev(){ if(tracks.empty()) return; Play(cur-1); }
void MusicPlayer::Stop(){ req=-1; }
float MusicPlayer::Position() const { return (float)(framesOut-startFrames)/48000.0f; }

// Output: 48 kHz stereo S16, 256-frame blocks (the only format the PS4 audio port accepts). BGM port so we don't fight the game's MAIN port.
void MusicPlayer::ThreadMain(){
    ps4::AudioInit();
    int h=ps4::AudioOpen(0xFF /*SYSTEM user*/, 1 /*PORT_TYPE_BGM*/, 0, 256, 48000, 1 /*S16_STEREO*/);
    if(h<0) return;
    static short out[256*2]; static short in[4096*2];
    drmp3 mp3; bool open=false; double srcPos=0; unsigned srcRate=44100; unsigned ch=2; unsigned have=0, base=0; // 'in' holds [base, base+have) frames
    while(!quit){
        if(req!=-2){ int r=req; req=-2;
            if(open){ drmp3_uninit(&mp3); open=false; }
            playing=false; paused=false;
            if(r>=0 && r<(int)tracks.size() && drmp3_init_file(&mp3,tracks[r].path.c_str(),nullptr)){
                open=true; cur=r; srcRate=mp3.sampleRate; ch=mp3.channels; srcPos=0; have=0; base=0;
                duration=(float)drmp3_get_pcm_frame_count(&mp3)/(float)srcRate; startFrames=framesOut; playing=true; }
        }
        if(!open||!playing||paused){ memset(out,0,sizeof out); ps4::AudioOutput(h,out); if(paused) startFrames++; continue; }
        // resample linear to 48k
        const double step=(double)srcRate/48000.0; bool eof=false;
        for(int i=0;i<256;i++){
            unsigned idx=(unsigned)srcPos; double fr=srcPos-idx;
            while(idx+1>=base+have){ // need more source frames
                unsigned keep=0; if(have>0){ in[0]=in[(have-1)*2]; in[1]=in[(have-1)*2+1]; keep=1; base=base+have-1; have=1; }
                static short tmp[2048*2];
                unsigned got=(unsigned)drmp3_read_pcm_frames_s16(&mp3,2048,tmp);   // interleaved, mp3.channels per frame
                if(got==0){ eof=true; break; }
                for(unsigned k=0;k<got;k++){ short l=tmp[k*ch], r=(ch>=2)?tmp[k*ch+1]:l; in[(keep+k)*2]=l; in[(keep+k)*2+1]=r; }   // mono -> stereo
                have+=got; }
            if(eof) break;
            unsigned a=idx-base; short l0=in[a*2],r0=in[a*2+1],l1=in[(a+1)*2],r1=in[(a+1)*2+1];
            float v=volume/100.0f; out[i*2]=(short)((l0+(l1-l0)*fr)*v); out[i*2+1]=(short)((r0+(r1-r0)*fr)*v); srcPos+=step; framesOut++;
        }
        ps4::AudioOutput(h,out);
        if(eof){ drmp3_uninit(&mp3); open=false; playing=false;
            if(repeatAll||cur+1<(int)tracks.size()){ int n=shuffle?rand()%(int)tracks.size():cur+1; req=n%(int)tracks.size(); } }
    }
    if(open) drmp3_uninit(&mp3);
    ps4::AudioClose(h);
}
