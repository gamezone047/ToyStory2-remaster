// Keep native pause navigation, pulsing font and the No/Yes confirmation.
// Only confirmation of the old Exit Level/Exit Game entry closes the window.
#ifdef CONTROLS_TEST
static int quitRequests;
#endif
static bool __cdecl ConfirmPauseQuit(){
 if(!mem<short>(0x52b818)||mem<short>(0x52adb0)!=3||mem<short>(0x52b7e4)!=1)return false;
#ifdef CONTROLS_TEST
 ++quitRequests;
#else
 // Native WM_DESTROY posts WM_QUIT (40936c); the normal message loop and
 // cleanup handle shutdown. Do not terminate the process or fake level completion.
 if(!PostMessage(GameWindow(),WM_CLOSE,0,0))return false;
#endif
 return true;
}
static DWORD pauseNoResume=0x49f70e,pauseYesResume=0x49f90a,pauseQuitResume=0x49fa2c;
__declspec(naked) static void PauseQuitHook(){__asm{
 cmp dx,bx
 jne noQuit
 pushfd
 pushad
 call ConfirmPauseQuit
 test al,al
 jz nativeExit
 popad
 popfd
 jmp dword ptr [pauseQuitResume]
 nativeExit:
 popad
 popfd
 jmp dword ptr [pauseYesResume]
 noQuit:
 jmp dword ptr [pauseNoResume]
}}
static bool InstallPauseQuit(){
 const unsigned char expected[]={0x66,0x3b,0xd3,0x0f,0x85,0x04,0xfe,0xff,0xff};
 return Patch(0x49f901,expected,sizeof(expected),PauseQuitHook);
}
