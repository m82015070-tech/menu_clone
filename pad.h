#pragma once
// Adaptive button reader.
// We have THREE pad natives (PAD_A/B/C) taking (0, control) but we are not sure which one means "held" and which one means
// "just pressed" / "just released". This tracker learns it: a native that stays true for >= 70 ms is a HELD (level) native.
//   * once a level native is known, only level natives are used (clean press edges, no duplicate release events)
//   * until then every native counts (OR) with a 180 ms debounce, and L1 stays "held" for 400 ms after a tap
// Pure logic, no game calls: it is unit tested on a PC.
#include <cstdio>

enum { PAD_N = 3, CTRL_N = 7 };
// index of each control inside the arrays (ids = 201,202,187,188,189,190,205)
enum { C_ACCEPT = 0, C_BACK, C_DOWN, C_UP, C_LEFT, C_RIGHT, C_LB };

struct PadTracker {
    bool down[CTRL_N] = {};            // held state (best knowledge)
    bool edge[CTRL_N] = {};            // true for ONE update when the button goes down
    bool lb = false;                   // L1 considered held
    bool anyLevel = false;
    void (*log)(const char*) = nullptr;

    void Update(unsigned long long now, const bool raw[PAD_N][CTRL_N], const int* ids) {
        for (int n = 0; n < PAD_N; n++)
            for (int c = 0; c < CTRL_N; c++) {
                if (raw[n][c]) {
                    if (!prevRaw[n][c]) since[n][c] = now;
                    else if (!level[n] && now - since[n][c] >= 70000ull) {
                        level[n] = true; anyLevel = true;
                        if (log) { char b[96]; snprintf(b, sizeof b, "pad: native %c behaves like HELD (level)", 'A' + n); log(b); }
                    }
                } else since[n][c] = 0;
                prevRaw[n][c] = raw[n][c];
            }
        for (int c = 0; c < CTRL_N; c++) {
            // log the first changes so we can see what each native does with each button
            unsigned char bits = (raw[0][c] ? 1 : 0) | (raw[1][c] ? 2 : 0) | (raw[2][c] ? 4 : 0);
            if (bits != prevBits[c]) {
                if (log && logged < 150) { char b[96]; snprintf(b, sizeof b, "pad: ctrl %d  A=%d B=%d C=%d", ids[c], bits & 1, (bits >> 1) & 1, (bits >> 2) & 1); log(b); logged++; }
                prevBits[c] = bits;
            }
            bool d = false;
            if (anyLevel) { for (int n = 0; n < PAD_N; n++) if (level[n] && raw[n][c]) d = true; }
            else          { d = raw[0][c] || raw[1][c] || raw[2][c]; }
            down[c] = d;
            bool e = d && !prevDown[c];
            if (e && !anyLevel) { if (lastEdge[c] != 0 && now - lastEdge[c] < 180000ull) e = false; else lastEdge[c] = now; }
            edge[c] = e;
            prevDown[c] = d;
        }
        if (down[C_LB]) lbT = now;
        lb = anyLevel ? down[C_LB] : (lbT != 0 && now - lbT < 400000ull);
    }

private:
    bool prevRaw[PAD_N][CTRL_N] = {};
    unsigned long long since[PAD_N][CTRL_N] = {};
    bool level[PAD_N] = {};
    bool prevDown[CTRL_N] = {};
    unsigned long long lastEdge[CTRL_N] = {}, lbT = 0;
    unsigned char prevBits[CTRL_N] = {};
    int logged = 0;
};
