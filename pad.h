#pragma once
// Button reader. The roles of the three pad natives were CONFIRMED from a real log:
//   PAD_A = "just pressed"  (true for 1 frame when the button goes down)
//   PAD_B = "held"          (true the whole time the button is down)
//   PAD_C = "just released" (true for 1 frame when the button comes up)  <- ignored
// down[] uses B (and A as a backup if a frame was missed), edge[] = the moment the button goes down.
// Pure logic, no game calls: it is unit tested on a PC.
#include <cstdio>

enum { PAD_N = 3, CTRL_N = 7 };
// index of each control inside the arrays (ids = 201,202,187,188,189,190,205)
enum { C_ACCEPT = 0, C_BACK, C_DOWN, C_UP, C_LEFT, C_RIGHT, C_LB };

struct PadTracker {
    bool down[CTRL_N] = {};            // held
    bool edge[CTRL_N] = {};            // true for ONE update when the button goes down
    void (*log)(const char*) = nullptr;

    // raw[0] = PAD_A, raw[1] = PAD_B, raw[2] = PAD_C (not used)
    void Update(const bool raw[PAD_N][CTRL_N], const int* ids) {
        for (int c = 0; c < CTRL_N; c++) {
            unsigned char bits = (raw[0][c] ? 1 : 0) | (raw[1][c] ? 2 : 0) | (raw[2][c] ? 4 : 0);
            if (bits != prevBits[c]) {
                if (log && logged < 40) { char b[96]; snprintf(b, sizeof b, "pad: ctrl %d  A=%d B=%d C=%d", ids[c], bits & 1, (bits >> 1) & 1, (bits >> 2) & 1); log(b); logged++; }
                prevBits[c] = bits;
            }
            bool d = raw[1][c] || raw[0][c];
            edge[c] = d && !down[c];
            down[c] = d;
        }
    }
private:
    unsigned char prevBits[CTRL_N] = {};
    int logged = 0;
};

// Open/close gesture: L1 and Circle pressed within 700 ms of each other, in ANY order (together or one after the other).
struct OpenGesture {
    unsigned long long tLb = 0, tBack = 0;
    bool Update(unsigned long long now, bool edgeLb, bool edgeBack) {
        if (edgeLb) tLb = now;
        if (edgeBack) tBack = now;
        if ((edgeLb || edgeBack) && tLb && tBack && now - tLb < 700000ull && now - tBack < 700000ull) { tLb = tBack = 0; return true; }
        return false;
    }
};
