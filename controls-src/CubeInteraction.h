// Gate only information cubes, after native proximity detection and before
// pickup effects. Coins, tokens and all other pickup categories stay automatic.
typedef int (__cdecl *PickupCategoryFn)(int);
static PickupCategoryFn pickupCategory=(PickupCategoryFn)0x44e520;
static bool CubeInteractionAllowed(int category,bool focus,DWORD now){
 if(category!=4)return true;
 cubeHintTime=now;
 if(!focus||!interactPending||now-interactTime>250||
    (mem<unsigned short>(0x52b816)&1)||mem<short>(0x52b818))return false;
 interactPending=false;cubeHintTime=0;return true;
}
static int __cdecl CubeContactAllowed(const unsigned char* pickup){
 unsigned id=pickup[12];
 return CubeInteractionAllowed(id>=0x30?pickupCategory(id):16,Focused(),GetTickCount());
}
static DWORD cubeContactResume=0x4a1222,cubeContactSkip=0x4a1694;
__declspec(naked) static void CubeContactHook(){__asm{
 pushfd
 pushad
 push esi
 call CubeContactAllowed
 add esp,4
 test eax,eax
 jz blocked
 popad
 popfd
 shl edi,5
 mov [esp+38h],edi
 jmp dword ptr [cubeContactResume]
blocked:
 popad
 popfd
 jmp dword ptr [cubeContactSkip]
}}
static bool InstallCubeInteraction(){
 static const unsigned char expected[]={0xc1,0xe7,0x05,0x89,0x7c,0x24,0x38};
 return Patch(0x4a121b,expected,sizeof(expected),CubeContactHook);
}
