// Portable, versioned single-player checkpoints. Only mapped scalar fields are
// persisted; native animation/resource pointers are reconstructed by level load.
#pragma pack(push,1)
struct SavedActor {int position[3];short angles[3],model,health;};
struct SavedGame {
 unsigned magic,format,bytes,checksum,layout;int level,index,completed;
 unsigned char campaign[0x188],pickups[512][16];unsigned pickupCount;
 int position[3];short angles[3],health,lives,coins;int yaw,pitch,tokenBaseline;
 unsigned char quest[0x28],collectibles[0x1a];SavedActor actors[64];
};
#pragma pack(pop)
typedef int (__cdecl *SaveLoadLevelFn)(int);
typedef void (__cdecl *SaveCameraFn)(void*,void*);
typedef void (__cdecl *SaveCampaignFn)(void*);
static SaveLoadLevelFn saveNativeLoad=(SaveLoadLevelFn)0x414720;
static SaveCameraFn saveResetCamera=(SaveCameraFn)0x403450;
static SaveCampaignFn saveCaptureCampaign=(SaveCampaignFn)0x4a2c80;
static SavedGame savedGame,queuedSave;
static bool saveAvailable,savePending,restorePending,saveReady;
static bool autosaveMainMenu;
static bool coopAutoStart,coopLevelStart,coopLoadConfirm;
static unsigned saveLayout,savePickupCount;
static int saveLevel;
static DWORD lastSaveCapture;
static CRITICAL_SECTION saveLock;
static HANDLE saveEvent,saveFlushed;
static char savePath[MAX_PATH],saveTempPath[MAX_PATH];
static unsigned SaveHash(const void* data,size_t size){
 const unsigned char* p=(const unsigned char*)data;unsigned h=2166136261u;
 for(size_t i=0;i<size;i++)h=(h^p[i])*16777619u;return h;
}
static unsigned SaveChecksum(SavedGame value){value.checksum=0;return SaveHash(&value,sizeof(value));}
static bool ValidSave(const SavedGame& s){
 if(s.magic!=0x32595354||s.format!=1||s.bytes!=sizeof(s)||s.checksum!=SaveChecksum(s)||
    (s.completed!=0&&s.completed!=1)||s.level<1||s.level>15||s.index<0||s.index>=15||s.pickupCount>512||s.health<0||s.health>100||
    s.coins<0||s.coins>999||s.lives<0||s.lives>999||s.yaw<0||s.yaw>=4096||s.pitch< -256||s.pitch>512)return false;
 for(int i=0;i<3;i++)if(s.position[i]<-0x10000000||s.position[i]>0x10000000)return false;
 return true;
}
static bool SaveReadable(uintptr_t address,size_t bytes){
 MEMORY_BASIC_INFORMATION info;
 return address&&VirtualQuery((void*)address,&info,sizeof(info))&&info.State==MEM_COMMIT&&
  !(info.Protect&(PAGE_NOACCESS|PAGE_GUARD))&&bytes<=(uintptr_t)info.BaseAddress+info.RegionSize-address;
}
static uintptr_t SavePickupTable(){
 uintptr_t p=mem<uintptr_t>(0x559d6c);
 if(!SaveReadable(p,4))return 0;
 unsigned count=*(unsigned short*)p;
 return count<=512&&SaveReadable(p,4+count*16)?p:0;
}
static DWORD WINAPI SaveWriter(void*){
 for(;;){
  WaitForSingleObject(saveEvent,INFINITE);
  SavedGame copy;
  EnterCriticalSection(&saveLock);copy=queuedSave;savePending=false;LeaveCriticalSection(&saveLock);
  HANDLE file=CreateFileA(saveTempPath,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
  bool ok=false;
  if(file!=INVALID_HANDLE_VALUE){DWORD count=0;ok=WriteFile(file,&copy,sizeof(copy),&count,NULL)&&count==sizeof(copy);if(ok)ok=FlushFileBuffers(file)!=0;CloseHandle(file);}
  if(ok)ok=MoveFileExA(saveTempPath,savePath,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
  if(!ok)Log("Autosave write failed; previous checkpoint retained.");
  EnterCriticalSection(&saveLock);if(!savePending)SetEvent(saveFlushed);LeaveCriticalSection(&saveLock);
 }
}
static bool SinglePlayerSave();
static void InitAutosave(){
 if(saveReady||!SinglePlayerSave())return;
 char directory[MAX_PATH];GetModuleFileNameA(NULL,directory,MAX_PATH);
 char* slash=strrchr(directory,'\\');if(!slash)return;slash[1]=0;
 if(strlen(directory)+20>=MAX_PATH)return;
 sprintf(savePath,"%sautosave.bin",directory);sprintf(saveTempPath,"%sautosave.tmp",directory);
 FILE* file=fopen(savePath,"rb");
 if(file){SavedGame candidate;bool ok=fread(&candidate,1,sizeof(candidate),file)==sizeof(candidate)&&fgetc(file)==EOF;fclose(file);
  if(ok&&ValidSave(candidate)){savedGame=candidate;saveAvailable=true;}}
 InitializeCriticalSection(&saveLock);
 saveEvent=CreateEvent(NULL,FALSE,FALSE,NULL);saveFlushed=CreateEvent(NULL,TRUE,TRUE,NULL);
 if(!saveEvent||!saveFlushed)return;
 HANDLE thread=CreateThread(NULL,0,SaveWriter,NULL,0,NULL);if(!thread)return;
 CloseHandle(thread);saveReady=true;
}
static void QueueAutosave(const SavedGame& value){
 EnterCriticalSection(&saveLock);queuedSave=value;savePending=true;ResetEvent(saveFlushed);SetEvent(saveEvent);LeaveCriticalSection(&saveLock);
}
static void FlushAutosave(){
 if(saveReady&&saveAvailable){QueueAutosave(savedGame);WaitForSingleObject(saveFlushed,2000);}
}
static bool SinglePlayerSave(){
 char value[8];return !GetEnvironmentVariableA("TOY2_COOP_BRIDGE",value,sizeof(value));
}
static void BeginContinue(){
 memcpy(&mem<unsigned char>(0x52ef90),savedGame.campaign,sizeof(savedGame.campaign));
 ((void(__cdecl*)(void*))0x4a2cc0)(&mem<unsigned char>(0x52ef90));
 mem<short>(0x52ad8a)=(short)savedGame.index;
 mem<int>(0x830d58)=1;restorePending=!savedGame.completed;
}
static const char* AutosaveMenuLabel(const char* label){
 if(!autosaveMainMenu)return label;
 if((uintptr_t)label==0x4f5ae8||(uintptr_t)label==0x4f5afc)return saveAvailable?"continue game":"new game";
 if((uintptr_t)label==0x4f67f0)return "new game";
 return label;
}
static int __cdecl AutosaveMainMenu(){
 InitAutosave();autosaveMainMenu=true;
 coopAutoStart=CoopStartRequested();
 int action=((int(__cdecl*)())0x437c40)();autosaveMainMenu=false;
 if(coopAutoStart){action=4;coopAutoStart=false;coopLevelStart=true;}
 if(action==2&&saveAvailable&&SinglePlayerSave())BeginContinue();
 else if(action==4){
  restorePending=false;mem<int>(0x830d58)=0;
  ((void(__cdecl*)(void*))0x4a2c20)(&mem<unsigned char>(0x52ef90));
  ((void(__cdecl*)(void*))0x4a2cc0)(&mem<unsigned char>(0x52ef90));action=2;
 }
 return action;
}
static void __cdecl CoopMenuFrame(){
 ((void(__cdecl*)())0x4a1be0)();
 // Run native renderer/menu initialization before requesting New Game.
 if(coopAutoStart){mem<int>(0x53e4a8)=2;mem<int>(0x53ca60)=0;}
}
static int __cdecl AutosaveStartup(){
 InitAutosave();
 // Let native menu setup initialize its fade, scene and renderer.
 // Resume only when Continue is selected, so startup remains visible.
 mem<int>(0x830c88)=-1;
 return ((int(__cdecl*)())0x437fb0)();
}
static int __cdecl AutosaveLevelSelect(){
 if(consoleLevelRequest){int wanted=consoleLevelRequest;consoleLevelRequest=0;for(int i=0;i<15;i++)if(mem<int>(0x50268c+i*4)==wanted){mem<short>(0x52ad8a)=(short)i;return 0;}}

 if(coopLevelStart){coopLevelStart=false;mem<short>(0x52ad8a)=0;return 0;}
 if(restorePending){mem<short>(0x52ad8a)=(short)savedGame.index;return 0;}
 return ((int(__cdecl*)())0x453cf0)();
}
static int __cdecl AutosaveLoadLevel(int level){
 saveLevel=0;
 coopLoadConfirm=!SinglePlayerSave();
 int result=saveNativeLoad(level);
 coopLoadConfirm=false;
 uintptr_t table=SavePickupTable();
 if(result||level<1||level>15||!table)return result;
 saveLevel=level;savePickupCount=*(unsigned short*)table;
 // Y may already carry the collected sentinel for campaign tokens. Hash
 // stable X/Z coordinates and item type, not mutable pickup state.
 saveLayout=2166136261u;
 for(unsigned i=0;i<savePickupCount;i++){
  unsigned char* p=(unsigned char*)(table+4+i*16);
  saveLayout=(saveLayout^SaveHash(p,4))*16777619u;
  saveLayout=(saveLayout^SaveHash(p+8,5))*16777619u;
 }
 if(restorePending){
  restorePending=false;
  if(savedGame.level!=level||savedGame.layout!=saveLayout||savedGame.pickupCount!=savePickupCount){
   Log("Autosave level layout differs; exact restore refused.");return result;
  }
  memcpy((void*)(table+4),savedGame.pickups,savePickupCount*16);
  memcpy(&mem<int>(0x52f300),savedGame.position,12);
  memcpy(&mem<short>(0x52f30c),savedGame.angles,6);
  mem<short>(0x52f348)=savedGame.angles[1];
  mem<short>(0x52f396)=savedGame.health;mem<short>(0x52f39a)=savedGame.lives;mem<short>(0x52f39e)=savedGame.coins;
  memcpy(&mem<unsigned char>(0x52ad58),savedGame.quest,sizeof(savedGame.quest));
  memcpy(&mem<unsigned char>(0x52c820),savedGame.collectibles,sizeof(savedGame.collectibles));
  mem<int>(0x830ca8)=savedGame.tokenBaseline;
  for(int i=0;i<64;i++){
   uintptr_t a=0x52c840+i*0x9c;const SavedActor& s=savedGame.actors[i];
   if(s.model==0){mem<short>(a+0x14)=0;mem<short>(a+0x7e)=0;mem<short>(a+0x7a)=0;}
   else if(mem<short>(a+0x14)==s.model){memcpy(&mem<int>(a),s.position,12);memcpy(&mem<short>(a+0xc),s.angles,6);mem<short>(a+0x7e)=s.health;}
  }
  thirdYaw=savedGame.yaw;thirdPitch=savedGame.pitch;pitchInitialized=true;lastThird=GetTickCount();
  // Rebuild the native camera around the restored player, not the level spawn.
  saveResetCamera(&mem<unsigned char>(0x52f3a0),&mem<unsigned char>(0x52f300));
  Log("Autosave restored: position, inventory, pickup table and actor positions/health.");
 }
 return result;
}
static void CaptureAutosave(){
 if(!saveReady||!SinglePlayerSave()||restorePending||saveLevel!=mem<int>(0x88278c)||saveLevel<1||saveLevel>15||
    mem<short>(0x52f396)<0||mem<short>(0x52b7dc)||mem<short>(0x52b818)||
    (mem<unsigned char>(0x52b816)&1)||(mem<unsigned char>(0x52f340)&4)||mem<int>(0x52ada0)>=0)return;
 uintptr_t table=SavePickupTable();if(!table||*(unsigned short*)table!=savePickupCount)return;
 SavedGame value={};value.magic=0x32595354;value.format=1;value.bytes=sizeof(value);value.layout=saveLayout;
 value.level=saveLevel;value.index=mem<short>(0x52ad8a);value.pickupCount=savePickupCount;
 saveCaptureCampaign(&mem<unsigned char>(0x52ef90));
 memcpy(value.campaign,&mem<unsigned char>(0x52ef90),sizeof(value.campaign));
 memcpy(value.pickups,(void*)(table+4),savePickupCount*16);
 memcpy(value.position,&mem<int>(0x52f300),12);memcpy(value.angles,&mem<short>(0x52f30c),6);
 value.health=mem<short>(0x52f396);value.lives=mem<short>(0x52f39a);value.coins=mem<short>(0x52f39e);
 value.yaw=thirdYaw&4095;value.pitch=Clamp(thirdPitch,-256,512);value.tokenBaseline=mem<int>(0x830ca8);
 memcpy(value.quest,&mem<unsigned char>(0x52ad58),sizeof(value.quest));
 memcpy(value.collectibles,&mem<unsigned char>(0x52c820),sizeof(value.collectibles));
 for(int i=0;i<64;i++){
  uintptr_t a=0x52c840+i*0x9c;SavedActor& s=value.actors[i];
  memcpy(s.position,&mem<int>(a),12);memcpy(s.angles,&mem<short>(a+0xc),6);
  s.model=mem<short>(a+0x14);s.health=mem<short>(a+0x7e);
 }
 value.checksum=SaveChecksum(value);
 if(!ValidSave(value))return;
 savedGame=value;saveAvailable=true;
}
static bool CompleteAutosave(bool bossCompleted=false){
 int level=mem<int>(0x88278c);
 if(!saveReady||!SinglePlayerSave()||level<1||level>15||
    !(LevelComplete(level)||(bossCompleted&&level%3==0)))return false;
 // Level resources may already have been unloaded for the summary screen.
 // Keep the last snapshot, but never dereference its old pickup/actor pointers.
 if(!saveAvailable||savedGame.level!=level){
  savedGame={};savedGame.magic=0x32595354;savedGame.format=1;savedGame.bytes=sizeof(savedGame);
  savedGame.level=level;
 }
 saveCaptureCampaign(&mem<unsigned char>(0x52ef90));
 memcpy(savedGame.campaign,&mem<unsigned char>(0x52ef90),sizeof(savedGame.campaign));
 savedGame.index=mem<short>(0x52ad8a);savedGame.completed=1;
 savedGame.checksum=SaveChecksum(savedGame);saveAvailable=true;
 return true;
}
static void __cdecl AutosaveInsteadOfSlots(){
 if(CompleteAutosave())FlushAutosave();
}
static void __cdecl AutosaveBossInsteadOfSlots(){
 if(CompleteAutosave(true))FlushAutosave();
}
static void __cdecl AutosaveFrame(){
 ((void(__cdecl*)())0x49dfe0)();
 CaptureAutosave(); // in-memory checkpoint every frame; disk work is throttled
 if(saveReady&&saveAvailable&&GetTickCount()-lastSaveCapture>=1000){QueueAutosave(savedGame);lastSaveCapture=GetTickCount();}
}
static bool InstallAutosave(){
 return CallHook(0x4381c5,0x437c40,AutosaveMainMenu)&&CallHook(0x4382cc,0x437fb0,AutosaveStartup)&&
  CallHook(0x437ccd,0x4a1be0,CoopMenuFrame)&&
  CallHook(0x49dee9,0x453cf0,AutosaveLevelSelect)&&CallHook(0x49dbc6,0x414720,AutosaveLoadLevel)&&
  CallHook(0x49dc74,0x49dfe0,AutosaveFrame)&&
  CallHook(0x49de14,0x454020,AutosaveInsteadOfSlots)&&CallHook(0x49de71,0x454020,AutosaveBossInsteadOfSlots);
}
