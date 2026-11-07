// Optional bridge entry points. Resolve lazily after all ASI modules load.
static FARPROC CoopEntry(const char* name){
#ifdef CONTROLS_TEST
 (void)name;return NULL;
#else
 HMODULE bridge=GetModuleHandleA("CoopBridge.asi");return bridge?GetProcAddress(bridge,name):NULL;
#endif
}
static void CoopDeathEvent(uintptr_t actor,int mode,bool begin){
 typedef void (__cdecl *Fn)(uintptr_t,int,int);
 Fn fn=(Fn)CoopEntry("CoopDeathEvent");if(fn)fn(actor,mode,begin?1:0);
}
static void CoopShotEvent(uintptr_t beam){
 typedef void (__cdecl *Fn)(uintptr_t);
 Fn fn=(Fn)CoopEntry("CoopShotEvent");if(fn)fn(beam);
}
static void CoopNameplates(){
 typedef void (__cdecl *Fn)();Fn fn=(Fn)CoopEntry("CoopNameplates");if(fn)fn();
}
static bool CoopStartRequested(){
 typedef int (__cdecl *Fn)();Fn fn=(Fn)CoopEntry("CoopStartRequested");return fn&&fn()!=0;
}
