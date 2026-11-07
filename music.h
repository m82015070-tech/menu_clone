#pragma once
#include <string>
#include <vector>
// MP3 player. Files: /data/16menu/sound/mp3/*.mp3 . Runs on its own thread, so it keeps playing with the menu closed.
// Needs dr_mp3.h (public domain, https://github.com/mackron/dr_libs) placed in source/ .
class MusicPlayer {
public:
    void Init(); void Shutdown(); void Scan();
    void Play(int idx); void TogglePause(); void Next(); void Prev(); void Stop();
    void SetVolume(int pct)          { volume = pct<0?0:(pct>100?100:pct); }
    int  Volume() const              { return volume; }
    bool Playing() const             { return playing; }
    bool Paused()  const             { return paused; }
    int  Current() const             { return cur; }
    float Position() const;          // seconds
    float Duration() const           { return duration; }
    std::string Title() const        { return (cur>=0&&cur<(int)tracks.size())?tracks[cur].title:std::string(); }
    std::vector<std::string> Titles() const { std::vector<std::string> t; for(auto& x:tracks) t.push_back(x.title); return t; }
    bool shuffle=false, repeatAll=true, showWidget=true;
    void ThreadMain();               // internal
private:
    struct Track{ std::string title, path; };
    std::vector<Track> tracks;
    volatile int cur=-1, req=-2, volume=70; volatile bool playing=false, paused=false, quit=false;
    volatile unsigned long long framesOut=0; float duration=0; unsigned long long startFrames=0;
};
