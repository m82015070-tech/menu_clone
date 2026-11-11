#pragma once
// The API header that every C script sees (it is placed in front of the script source before TinyCC compiles it).
// Scripts must NOT #include system headers: there are none on the PS4. Everything a script needs is declared here.
static const char kScriptApiPrelude[] = R"SCRIPTAPI(
typedef unsigned char u8;  typedef unsigned short u16;  typedef unsigned int u32;  typedef unsigned long long u64;
typedef signed char s8;    typedef short s16;           typedef int s32;           typedef long long s64;
#define NULL ((void*)0)
#define TRUE 1
#define FALSE 0
/* pad buttons for pad_down() / pad_pressed() */
enum { BTN_ACCEPT = 0, BTN_BACK = 1, BTN_DOWN = 2, BTN_UP = 3, BTN_LEFT = 4, BTN_RIGHT = 5, BTN_L1 = 6 };

/* ---- functions provided by the menu ---- */
void host_log(const char* msg);                    /* append a line to /data/16menu/menu_clone.log */
void host_notify(const char* msg);                 /* PS4 popup notification */
void draw_rect(float cx, float cy, float w, float h, int r, int g, int b, int a);   /* screen coords 0..1, centre based */
void draw_text(const char* s, float x, float y, float scale, int r, int g, int b, int a);
int  pad_down(int btn);                            /* 1 while the button is held */
int  pad_pressed(int btn);                         /* 1 only on the frame the button goes down */
u64  time_ms(void);
void menu_toggle(const char* label, int* var);     /* call from script_init(): adds an ON/OFF row to this script's page */
void menu_action(const char* label, void (*cb)(void)); /* call from script_init(): adds a row that runs cb */
u64  native_call(u32 off156, u32 off157, const u64* args, int nargs);   /* raw game native (offset for 1.56 and for 1.57) */
u64  arg_f(float f);                               /* pack a float argument for native_call */
u64  arg_i(int i);
u64  arg_p(const void* p);

/* ---- tiny libc subset ---- */
void* memcpy(void* d, const void* s, u64 n);
void* memset(void* d, int c, u64 n);
u64   strlen(const char* s);
int   strcmp(const char* a, const char* b);
int   snprintf(char* buf, u64 n, const char* fmt, ...);

/* A script may define any of:  void script_init(void);  void script_tick(void);  void script_stop(void);  */
)SCRIPTAPI";
