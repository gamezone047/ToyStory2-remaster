// Gate positional actor effects at the sound layer, not at visibility or AI.
// Local Buzz audio is centered, independent of the freely orbiting camera.
// Enemy room gating and already non-positional dialogue remain unchanged.
static bool IsLocalBuzzSound(const int* position){return (uintptr_t)position==0x52f300;}
static bool IsActorSoundPosition(uintptr_t p){
 return p>=0x52c840&&p<0x52ef40&&(p-0x52c840)%0x9c==0;
}
static bool SameAudioRoom(int playerRoom,int actorRoom){return playerRoom>=0&&actorRoom>=0&&playerRoom==actorRoom;}
typedef int (__cdecl *RoomFn)(const int*);
static RoomFn soundRoom=(RoomFn)0x4885c0;
static bool ActorSoundAudible(const int* position){
 if(!IsActorSoundPosition((uintptr_t)position))return true;
 // Use the player's position, never the freely orbiting camera's room.
 return SameAudioRoom(soundRoom(&mem<int>(0x52f300)),soundRoom(position));
}
typedef int (__cdecl *SpatialOneFn)(int,int,int,const int*);
typedef int (__cdecl *SpatialLoopFn)(int,int,int,int,const int*,int);
static SpatialOneFn nativeSpatialOne;
typedef int (__cdecl *CenteredOneFn)(int,int,int,int);
static CenteredOneFn nativeCenteredOne=(CenteredOneFn)0x4a37e0;
static SpatialLoopFn nativeSpatialLoop,nativeSpatialEffect;
// The tagged-voice path used GetCurrentPosition()==0 as "not playing".
// A live looping buffer can wrap to zero. Query its actual status instead.
static HRESULT __stdcall TaggedVoiceStatus(void* buffer,DWORD* playing,DWORD* unused){
 typedef HRESULT (__stdcall *StatusFn)(void*,DWORD*);
 DWORD status=0;
 HRESULT result=((StatusFn)(*(uintptr_t**)buffer)[9])(buffer,&status);
 *playing=SUCCEEDED(result)?(status&1):1;
 *unused=0;return result;
}
static DWORD taggedStatusResume=0x47e187;
// Allocation has a separate cursor query. A just-started voice can still have
// cursor zero while DirectSound queues playback. It must not be reused by the
// next robot in the same frame, or all robots repeatedly restart one buffer.
static HRESULT __stdcall AllocationVoiceCursor(void* buffer,DWORD* cursor,DWORD* writeCursor){
 typedef HRESULT (__stdcall *CursorFn)(void*,DWORD*,DWORD*);
 typedef HRESULT (__stdcall *StatusFn)(void*,DWORD*);
 uintptr_t* table=*(uintptr_t**)buffer;
 HRESULT result=((CursorFn)table[4])(buffer,cursor,writeCursor);
 if(FAILED(result))return result;
 DWORD status=0;HRESULT state=((StatusFn)table[9])(buffer,&status);
 if(FAILED(state)){if(!*cursor)*cursor=1;return result;}
 if(!(status&1))*cursor=0;
 else if(!*cursor)*cursor=1;
 return result;
}
static DWORD allocationStatusResume=0x47e24e;
__declspec(naked) static void AllocationVoiceCursorHook(){__asm{
 call AllocationVoiceCursor
 test eax,eax
 jmp dword ptr [allocationStatusResume]
}}
__declspec(naked) static void TaggedVoiceStatusHook(){__asm{
 // Replace the native indirect call without adding another return address
 // between its three stdcall arguments and the callee.
 call TaggedVoiceStatus
 test eax,eax
 jmp dword ptr [taggedStatusResume]
}}
static int __cdecl RoomOneShot(int sample,int pitch,int volume,const int* position){
 if(IsLocalBuzzSound(position)){
  int gain=Clamp(volume,0,128);
  nativeCenteredOne(sample,pitch,gain,gain);
  // The spatial wrapper returns average gain, not the channel handle returned
  // by 4a37e0. Preserve that contract for the sound event sequencer.
  return gain;
 }
 if(!ActorSoundAudible(position))return 0;
 return nativeSpatialOne(sample,pitch,volume,position);
}
static int __cdecl RoomLoop(int tag,int sample,int pitch,int volume,const int* position,int flag){
 if(IsLocalBuzzSound(position))return nativeSpatialLoop(tag,sample,pitch,volume,NULL,flag);
 // Keep updating the existing tagged voice at zero volume after leaving a
 // room. Skipping the update would leave a looping sound playing audibly.
 if(!ActorSoundAudible(position))return nativeSpatialLoop(tag,sample,pitch,0,NULL,flag);
 return nativeSpatialLoop(tag,sample,pitch,volume,position,flag);
}
static int __cdecl RoomEffect(int tag,int sample,int pitch,int volume,const int* position,int flag){
 if(!ActorSoundAudible(position))return nativeSpatialEffect(tag,sample,pitch,0,NULL,flag);
 return nativeSpatialEffect(tag,sample,pitch,volume,position,flag);
}
static bool InstallRoomAudio(){
 const unsigned char status[]={0xff,0x51,0x10,0x85,0xc0};
 const unsigned char allocation[]={0xff,0x52,0x10,0x85,0xc0};
 if(memcmp((void*)0x47e182,status,sizeof(status))||memcmp((void*)0x47e249,allocation,sizeof(allocation)))return false;
 if(!Patch(0x47e182,status,sizeof(status),TaggedVoiceStatusHook)||
    !Patch(0x47e249,allocation,sizeof(allocation),AllocationVoiceCursorHook))return false;
 const unsigned char one[]={0x8b,0x44,0x24,0x10,0x8b,0x15,0x18,0x53,0x55,0};
 const unsigned char loop[]={0x8b,0x44,0x24,0x14,0x53};
 const unsigned char effect[]={0x8b,0x44,0x24,0x18,0x8b,0x4c,0x24,0x10};
 if(memcmp((void*)0x4a3810,one,sizeof(one))||memcmp((void*)0x4a3c80,loop,sizeof(loop))||memcmp((void*)0x4a3b90,effect,sizeof(effect)))return false;
 nativeSpatialOne=(SpatialOneFn)MenuTrampoline(0x4a3810,one,sizeof(one),RoomOneShot);
 nativeSpatialLoop=(SpatialLoopFn)MenuTrampoline(0x4a3c80,loop,sizeof(loop),RoomLoop);
 nativeSpatialEffect=(SpatialLoopFn)MenuTrampoline(0x4a3b90,effect,sizeof(effect),RoomEffect);
 return nativeSpatialOne&&nativeSpatialLoop&&nativeSpatialEffect;
}
