// Separate translation unit on purpose: plugin_common.h declares NotifyStatic without extern "C",
// so including it from C++ would give the wrong (mangled) name. Here we declare it ourselves with C linkage.
#include "notify.h"
extern "C" void NotifyStatic(const char* IconUri, const char* text);
void PopupNotify(const char* text){ NotifyStatic("cxml://psnotification/tex_icon_system", text); }
