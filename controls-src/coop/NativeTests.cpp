#define COOP_TEST
#include "CoopBridge.cpp"
#include <assert.h>
#include <stddef.h>
static REGSAM requestedAccess;
static int meshMoves,collisionMoves,lastMeshX,lastCollisionX;
static void __cdecl MockMesh(int,int x,int,int){++meshMoves;lastMeshX=x;}
static void __cdecl MockCollision(int,int x,int,int){++collisionMoves;lastCollisionX=x;}
static bool newPushContact;
static void __cdecl MockPush(void*){M<int>(0x53c680)+=32;if(newPushContact)M<int>(0x53c648)=1;}
static int remoteDeaths,questRemovals;
static void __cdecl MockQuestRemoval(uintptr_t actor,int mode){assert(mode==2);bool present=false;for(unsigned i=0;i<64&&M<uintptr_t>(0x52f1d0+i*4);i++)if(M<uintptr_t>(0x52f1d0+i*4)==actor)present=true;assert(present);++questRemovals;M<short>(actor+0x14)=0;}
static void __cdecl MockActors(){}
static void __cdecl MockLevelTick(int){}
static void __cdecl MockRemoteDeath(uintptr_t actor,int mode){assert(mode==1);++remoteDeaths;M<short>(actor+0x7e)=999;M<short>(actor+0x12)=2;}
static LSTATUS WINAPI MockRegistryOpen(HKEY,LPCSTR,DWORD,REGSAM access,PHKEY result){requestedAccess=access;if(result)*result=(HKEY)123;return ERROR_SUCCESS;}

static int plankCalls,plankMeshCalls;
static int backyardBalanceCalls,backyardBalanceMesh;
static void __cdecl MockBackyardBalanceMesh(int a,int b){assert(a==b&&a>=0&&a<2);backyardBalanceMesh=a;}
static int __cdecl MockBackyardBalance(int a,int b,int velocity,int mode,int minimum,int maximum,int divisor){assert(a==b&&mode==0&&minimum==-448&&maximum==(a?224:448)&&divisor==8);++backyardBalanceCalls;M<short>(0x728720+a*0x34+0x10)=(short)(200+a);M<short>(0x728720+a*0x34+0x24)=12;return velocity+8;}
// Model the real split update: 49ec00 queues delta at +24, while 488bd0
// reads the CURRENT angle. A later collision pass commits the queued rotation.
static int queuedBalanceCalls,queuedMeshAngle;
static void __cdecl MockQueuedBalanceMesh(int collider,int mesh){assert(collider==mesh&&collider>=0&&collider<2);queuedMeshAngle=M<short>(0x728720+collider*0x34+0x10)>>2;}
static int __cdecl MockQueuedBalance(int collider,int mesh,int velocity,int,int,int,int){
 ++queuedBalanceCalls;uintptr_t c=0x728720+collider*0x34;M<short>(c+0x24)=12;M<unsigned short>(c+0x32)|=8;
 MockQueuedBalanceMesh(collider,mesh);return velocity+8;
}
static void CommitQueuedBalance(int collider){uintptr_t c=0x728720+collider*0x34;M<short>(c+0x10)=(short)(M<short>(c+0x10)+M<short>(c+0x24));M<short>(c+0x24)=0;}
static void __cdecl MockPlatformMesh(int a,int b){assert(a==15&&b==15);++plankMeshCalls;}
static int __cdecl MockBalance(int,int,int velocity,int,int,int,int){++plankCalls;M<short>(Plank+0x10)=100;M<short>(Plank+0x24)=12;return velocity+8;}
static void __cdecl MockGarageMesh(int,int,int,int){}
static int musicCalls;
static int __cdecl MockMusic(int track,int){++musicCalls;M<int>(0x5282cc)=track;return 1;}
static int thoughtTarget[3];
static void __cdecl MockEnemyThink(uintptr_t){memcpy(thoughtTarget,enemyTargetPosition,sizeof(thoughtTarget));}
static void DiagnosticTestRaise(){__try{RaiseException(EXCEPTION_INT_DIVIDE_BY_ZERO,0,0,NULL);}__except(EXCEPTION_EXECUTE_HANDLER){}}
static int tokenReveals;
static void __cdecl MockReveal(int index,int quiet){assert(quiet==1);++tokenReveals;M<int>(0x830cd0+index*16)=1;}
static int rayBlocked;
static int __cdecl MockNameRay(int* origin,int* delta,int mask,int mode,int radius){assert(mask==0x8000&&mode==1&&radius==256);assert(origin[0]==10&&delta[0]==90);return rayBlocked;}
static int rcImpulses,rcContacts;
static void __cdecl MockRcImpulse(uintptr_t actor,int angle,int damage){assert(actor==RcActor&&angle>=0&&angle<4096&&damage==0);++rcImpulses;M<int>(actor+0x44)=77;}
static void __cdecl MockRcContact(){++rcContacts;bool found=false;for(unsigned i=0;i<64&&M<uintptr_t>(0x52f1d0+i*4);i++)if(M<uintptr_t>(0x52f1d0+i*4)==RcActor)found=true;
 if(found){assert(M<unsigned short>(RcActor+0x40)&0x80);SharedContactImpulse(RcActor,1024,0);}}
static int backyardMeshCalls,backyardEnabled,backyardDisabled,duckDeltaY;
static int treeRotations[2][3];
static void __cdecl MockTreeRotate(int id,int x,int y,int z){assert(id==30||id==31);treeRotations[id-30][0]=x;treeRotations[id-30][1]=y;treeRotations[id-30][2]=z;}
static void __cdecl MockBackyardTransform(int,int,int,int){++backyardMeshCalls;}
static void __cdecl MockBackyardEnable(int id){backyardEnabled=id;}
static void __cdecl MockBackyardDisable(int id){backyardDisabled=id;}
static void __cdecl MockBackyardPosition(int id,int* p){assert(id==7);p[0]=100;p[1]=200;p[2]=300;}
static void __cdecl MockBackyardDelta(int id,int,int y,int){assert(id==7);duckDeltaY=y;}
int main(int argc,char** argv){
 {CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=2;pickupCount=0;localOwner=0;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=2;localFrame.layout=remoteFrame.layout=123;
 backyardScale=backyardRotate=MockBackyardTransform;backyardEnable=MockBackyardEnable;backyardDisable=MockBackyardDisable;
 backyardGetPosition=MockBackyardPosition;backyardMoveCollider=MockBackyardDelta;moveObjectMesh=MockBackyardTransform;moveObjectCollision=MockBackyardTransform;
 M<short>(0x52c854)=12;M<short>(0x52c8be)=101;M<int>(0x52c840)=12345;M<unsigned short>(0x52c880)=0x184;SyncMower();CoopWorldActor mower=localFrame.mower;assert(mower.revision&&mower.pose.x==12345);
 localOwner=1;remoteFrame.mower=mower;M<int>(0x52c840)=0;SyncMower();assert(M<int>(0x52c840)==12345&&M<short>(0x52c8be)==101);
 localOwner=0;ResetEnemySync();remoteFrame.pose.x=12345;remoteFrame.pose.health=14;M<short>(0x52c882)=1400;ActivatePeerEnemies();assert(enemyForced[0]);RestoreEnemyRanges();
 CoopDuck duck={};duck.revision=1;duck.owner=1;duck.value[0]=100;duck.value[1]=4400;duck.value[2]=300;duck.value[3]=4096;duck.value[6]=300;duck.value[7]=500;
 M<short>(0x728720+7*0x34+0x30)=1;WriteDuck(duck);assert(M<int>(0x52f64c)==4400&&duckDeltaY==104&&backyardMeshCalls==3);
 localFrame.duck=duck;localFrame.worldContext=0x10000;remoteFrame.worldContext=0x10200;assert(DuckOwner()==1);
 localFrame.worldContext|=0x200;assert(DuckOwner()==0);localFrame.worldContext=remoteFrame.worldContext=0x10000;assert(DuckOwner()==1);
 backyardRotate=MockTreeRotate;remoteFrame.tree={2,400,24};M<int>(0x52f5ec)=0;SyncTree();assert(M<int>(0x52f5ec)==400&&backyardDisabled==11&&backyardEnabled==12);
 for(int i=0;i<2;i++)assert(treeRotations[i][0]==0&&treeRotations[i][1]==0&&treeRotations[i][2]==3695);
 remoteFrame.tree.angle=50;M<int>(0x52f5ec)=100;SyncTree();assert(M<int>(0x52f5ec)==400);
 M<int>(0x52f5ec)=581;SyncTree();assert(localFrame.tree.angle==581&&CoopValidFrame(localFrame));
 for(int i=0;i<2;i++)assert(treeRotations[i][0]==0&&treeRotations[i][1]==0&&treeRotations[i][2]==3514);
 // A joining observer receiving the completed state must apply both final meshes.
 localOwner=1;localFrame.tree={};remoteFrame.tree={4,581,24};M<int>(0x52f5ec)=0;SyncTree();
 assert(M<int>(0x52f5ec)==581&&backyardDisabled==11&&backyardEnabled==12);
 for(int i=0;i<2;i++)assert(treeRotations[i][0]==0&&treeRotations[i][1]==0&&treeRotations[i][2]==3514);
 localOwner=0;
 remoteFrame.travel={3,1,2};travelConsumed=0;assert(CoopConsoleLevel(0)==2&&CoopConsoleLevel(0)==0);ResetSession();assert(localFrame.travel.value==2&&CoopConsoleLevel(0)==0);assert(CoopConsoleLevel(15)==15&&localFrame.travel.revision==4);
 shared=NULL;helperTick=0;cachedLevel=0;localFrame={};remoteFrame={};ResetEnemySync();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));travelConsumed=0;travelOwnerConsumed=2;
 puts("PASS: mower special-actor state/peer activation, duck mesh/collider rider delta/ownership, irreversible tree fall and deduplicated cross-level console travel.");}

 { CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=1;pickupCount=0;localOwner=1;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=1;localFrame.layout=remoteFrame.layout=123;
 ResetGarage();nativeRcImpulse=MockRcImpulse;nativeActorContact=MockRcContact;
 M<short>(RcActor+20)=8;remoteFrame.garage.revision=1;remoteFrame.garage.owner=0;remoteFrame.garage.health=1;remoteFrame.garage.flags=0x80;
 M<uintptr_t>(0x52f1d0)=0;SharedActorContact();assert(rcContacts==1&&rcImpulses==1&&localFrame.rcPushSequence==1&&M<uintptr_t>(0x52f1d0)==0);
 for(unsigned i=0;i<64;i++)M<uintptr_t>(0x52f1d0+i*4)=0x1000+i*4;M<uintptr_t>(0x52f2d0)=0;
 SharedActorContact();assert(rcContacts==3&&rcImpulses==2);for(unsigned i=0;i<64;i++)assert(M<uintptr_t>(0x52f1d0+i*4)==0x1000+i*4);
 localOwner=0;remoteFrame.rcPushSequence=7;remoteFrame.rcPushAngle=1024;ApplyRcPush();ApplyRcPush();assert(rcImpulses==3&&rcPushApplied==7);
 remoteFrame.rcPushSequence=8;remoteFrame.pose.x=1000000;ApplyRcPush();assert(rcImpulses==3);
 localFrame.garage.revision=200;localFrame.garage.phase=0;remoteFrame.garage.revision=2;remoteFrame.garage.owner=1;remoteFrame.garage.phase=1;SyncRc();assert(localFrame.garage.revision==200&&localFrame.garage.phase==0);
 remoteFrame.garage.revision=201;SyncRc();assert(localFrame.garage.owner==1&&localFrame.garage.phase==1);
 localFrame.garage.revision=203;localFrame.garage.phase=0;remoteFrame.garage.revision=202;remoteFrame.garage.phase=2;SyncRc();assert(localFrame.garage.phase==0);
 shared=NULL;helperTick=0;cachedLevel=0;localFrame={};remoteFrame={};ResetGarage();memset(testData,0,sizeof(testData));
 puts("PASS: RC contact bit replication, empty/full local collision lists, guest push replay exactly once, distance rejection and race-over-idle arbitration."); }
 { CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=2;pickupCount=0;localOwner=0;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=2;localFrame.layout=remoteFrame.layout=123;
 unsigned char path[4+6*12]={};M<unsigned short>((uintptr_t)path)=6;M<uintptr_t>(0x559c78)=(uintptr_t)path;
 M<short>(BurrowActor+20)=13;M<short>(BurrowActor+0x7e)=102;M<unsigned short>(BurrowActor+0x40)=0x20;
 M<int>(BurrowActor)=777;M<int>(BurrowActor+4)=0x3800;M<int>(BurrowActor+0x44)=55;
 M<unsigned>(0x52f5dc)=2;M<unsigned>(0x52f5e8)=1;M<unsigned>(0x52f61c)=60;SyncBurrow();CoopBurrow host=localFrame.burrow;
 assert(host.revision&&host.hole==2&&host.pose.x==777&&host.flags==0x20&&CoopValidFrame(localFrame));
 localOwner=1;localFrame.burrow={};remoteFrame.burrow=host;M<int>(BurrowActor)=99999;M<unsigned>(0x52f5dc)=5;SyncBurrow();
 assert(M<int>(BurrowActor)==777&&M<unsigned>(0x52f5dc)==2&&M<unsigned>(0x52f61c)==60&&M<int>(BurrowActor+0x44)==55);
 // A guest's native hard landing closes a hole; it survives reconciliation.
 M<int>((uintptr_t)path+8+12)=(-2147483647-1);SyncBurrow();assert(localFrame.burrow.closed==2&&M<unsigned>(0x52f6d0)==1&&M<unsigned>(0x52f61c)==0);
 localOwner=0;remoteFrame.burrow.closed=0x3f;SyncBurrow();assert(M<unsigned>(0x52f6d0)==6&&(M<unsigned short>(BurrowActor+0x40)&0xa4)==0x84);
 remoteFrame.burrow={};localFrame.burrow={};memset(path,0,sizeof(path));M<unsigned short>((uintptr_t)path)=6;M<unsigned short>(BurrowActor+0x40)=0x20;M<int>(BurrowActor+0x60)=0;
 remoteFrame.pose.x=777;remoteFrame.pose.y=0x3800;remoteFrame.pose.health=14;BeginBurrow();assert(M<int>(BurrowActor+0x60)==0x5000&&M<unsigned>(0x52f61c)==100&&M<unsigned>(0x52f5e8)==2);
 ResetEnemySync();ActivatePeerEnemies();assert(enemyForced[4]);RestoreEnemyRanges();
 localOwner=1;remoteFrame.burrow=host;CoopSetBit(remoteFrame.actorCollections,4);M<int>(BurrowActor)=1234;SyncBurrow();assert(M<int>(BurrowActor)==1234);
 shared=NULL;helperTick=0;cachedLevel=0;localFrame={};remoteFrame={};ResetEnemySync();memset(testData,0,sizeof(testData));
 puts("PASS: backyard pop-up army hole/timing/transform state, guest closure union, six-hole completion, distant peer trigger and collected-actor suppression."); }

 { remoteActorDeath=MockQuestRemoval;uintptr_t actor=0x52ca14;
 M<uintptr_t>(0x52f1d0)=0;SafeActorDeath(actor,2);assert(M<uintptr_t>(0x52f1d0)==0);
 for(unsigned i=0;i<64;i++)M<uintptr_t>(0x52f1d0+i*4)=0x1000+i*4;M<uintptr_t>(0x52f2d0)=0;
 SafeActorDeath(actor,2);for(unsigned i=0;i<64;i++)assert(M<uintptr_t>(0x52f1d0+i*4)==0x1000+i*4);assert(M<uintptr_t>(0x52f2d0)==0);
 questRemovals=0;memset(testData,0,sizeof(testData));puts("PASS: off-screen remote removal with empty/full native candidate lists, preserved list and no unbounded search."); }

 { strcpy(diagnosticCrash,"controls-src/coop/diagnostic-test.log");diagnosticExceptions=0;
 PVOID handler=AddVectoredExceptionHandler(1,DiagnosticVectored);assert(handler);DiagnosticTestRaise();RemoveVectoredExceptionHandler(handler);
 FILE* report=fopen(diagnosticCrash,"rb");assert(report);char text[4096]={};fread(text,1,sizeof(text)-1,report);fclose(report);
 assert(strstr(text,"exception=c0000094")||strstr(text,"exception=C0000094"));assert(strstr(text,"build=2026.11.7.5")&&strstr(text,"phase=startup"));DeleteFileA(diagnosticCrash);diagnosticCrash[0]=0;diagnosticExceptions=0;
 puts("PASS: handled deliberate exception records version, phase and exception registers without suppressing native exception handling."); }
 { CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=1;pickupCount=0;localOwner=0;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=1;localFrame.layout=remoteFrame.layout=123;
 ResetEnemySync();unsigned slot=5;uintptr_t actor=0x52c840+slot*0x9c;M<short>(actor+20)=3;M<short>(actor+0x7e)=2;M<short>(actor+0x42)=1000;M<unsigned short>(actor+0x40)=0x2000;
 M<int>(actor)=1000000;M<int>(0x52f300)=0;M<short>(0x52f396)=14;remoteFrame.pose.x=1000010;remoteFrame.pose.health=14;
 ActivatePeerEnemies();M<uintptr_t>(0x52f1d0)=0;RepairEnemyCandidates();assert(M<uintptr_t>(0x52f1d0)==actor&&M<uintptr_t>(0x52f1d4)==0);assert(M<short>(actor+0x42)==1000&&(M<unsigned short>(actor+0x40)&0x2001)==1);
 RepairEnemyCandidates();assert(M<uintptr_t>(0x52f1d4)==0);RestoreEnemyRanges();
 nativeEnemyThink=MockEnemyThink;SharedEnemyThink(actor);assert(thoughtTarget[0]==1000010&&M<int>(0x52f300)==0);
 remoteFrame.pose.health=0;SharedEnemyThink(actor);assert(thoughtTarget[0]==0);
 assert(ChooseEnemyTarget(1,100,110,true,true)==1&&ChooseEnemyTarget(1,10,100,true,true)==0);
 shared=NULL;helperTick=0;cachedLevel=0;localFrame={};remoteFrame={};ResetEnemySync();memset(testData,0,sizeof(testData));
 puts("PASS: distant peer candidate insertion, bounded/duplicate-free list, unchanged native range, independent nearest-player targeting and host position preserved."); }

 { // Native model-local limits must reject mismatched animation indices.
 unsigned char scene[0x27c]={},asset[0x30]={},clip[16]={};uintptr_t models[4]={},clips[2]={};
 testScene=(uintptr_t)scene;M<uintptr_t>(testScene+0x274)=(uintptr_t)models;M<unsigned>(testScene+0x278)=4;
 models[3]=(uintptr_t)asset;M<uintptr_t>((uintptr_t)asset+0x28)=(uintptr_t)clips;M<int>((uintptr_t)asset+0x2c)=2;
 clips[0]=clips[1]=(uintptr_t)clip;M<unsigned short>((uintptr_t)clip+4)=12;
 assert(NativeEnemyFrame(3,0,99<<16)==11<<16);assert(NativeEnemyFrame(3,2,0)==-1);assert(NativeEnemyFrame(4,0,0)==-1);
 CoopShared page={};shared=&page;helperTick=GetTickCount();localOwner=1;cachedLevel=2;pickupCount=0;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=2;localFrame.layout=remoteFrame.layout=99;
 ResetEnemySync();uintptr_t actor=0x52c840;M<short>(actor+20)=3;M<short>(actor+0x7e)=2;M<short>(actor+18)=1;M<short>(actor+22)=-1;M<int>(actor+24)=123;
 remoteFrame.enemies[0].revision=1;remoteFrame.enemies[0].model=3;remoteFrame.enemyAnimations[0]={0,-1,9<<16,0};
 BeginEnemyPresentation();assert(M<short>(actor+18)==0&&M<int>(actor+24)==9<<16);EndEnemyPresentation();assert(M<short>(actor+18)==1&&M<int>(actor+24)==123);
 localOwner=0;M<short>(actor+0x42)=1400;M<int>(actor)=600*256;remoteFrame.pose.x=0x1000000;
 ActivatePeerEnemies();assert(enemyForced[0]);RestoreEnemyRanges();assert(M<short>(actor+0x42)==1400);
 M<int>(actor)=0x1000000;remoteFrame.pose.x=-0x1000000;ActivatePeerEnemies();assert(enemyForced[0]);RestoreEnemyRanges();
 enemyAwakeUntil[0]=GetTickCount()-1;ActivatePeerEnemies();assert(!enemyForced[0]);
 shared=NULL;helperTick=0;testScene=0;cachedLevel=0;localFrame={};remoteFrame={};ResetEnemySync();memset(testData,0,sizeof(testData));
 puts("PASS: model-local animation limits, render-only animation override/restoration, host-visible and in-flight activation."); }
 { ResetRcPresentation();CoopPose p={};p.health=1;p.x=0;p.yaw=4090;
 assert(SmoothRcPoseAt(p,1,1000).x==0);p.x=400;p.yaw=6;SmoothRcPoseAt(p,2,1040);p.x=800;SmoothRcPoseAt(p,3,1080);
 assert(SmoothRcPoseAt(p,3,1160).x==600);p.x=999;assert(SmoothRcPoseAt(p,2,1160).x==600);
 assert(SmoothRcPoseAt(p,2,1500).x==800);p.x=0x80000;assert(SmoothRcPoseAt(p,4,1600).x==0x80000);ResetRcPresentation();
 nameRay=MockNameRay;int camera[3]={10,20,30};p.x=100;rayBlocked=0;assert(NameplateVisible(camera,p));rayBlocked=1;assert(!NameplateVisible(camera,p));
 puts("PASS: RC delayed interpolation, stale/stall/teleport handling; nameplate wall-ray visibility."); }
 { CoopShared page={};shared=&page;helperTick=GetTickCount();revealToken=MockReveal;
 unsigned char table[4+5*16]={};pickupTable=(uintptr_t)table;pickupCount=5;
 for(unsigned i=0;i<5;i++)M<uintptr_t>(0x830cd4+i*16)=pickupTable+8+i*16;
 for(unsigned level=1;level<=15;level++){
 cachedLevel=level;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=level;localFrame.layout=remoteFrame.layout=123;remoteFrame.pickupCount=5;
 for(unsigned i=0;i<5;i++)M<int>(0x830cd0+i*16)=0;
 remoteFrame.spawnedTokens=2;tokenReveals=0;SyncTokenSpawns();assert(tokenReveals==(level%3?1:0));SyncTokenSpawns();assert(tokenReveals==(level%3?1:0));
 M<int>(0x830ce0)=0;remoteFrame.tokens=2;SyncTokenSpawns();assert(tokenReveals==(level%3?1:0));
 M<unsigned char>(0x52f0d7+level)=0;remoteFrame.tokens=0x80;SyncTokens();assert(localFrame.tokens==0x80);
 }
 localFrame.spawnedTokens=32;assert(!CoopValidFrame(localFrame));
 shared=NULL;helperTick=0;cachedLevel=0;pickupTable=0;pickupCount=0;localFrame={};remoteFrame={};memset(testData,0,sizeof(testData));memset(testSession,0,sizeof(testSession));
 puts("PASS: token reveal across every ordinary campaign level, duplicate/collected reveal suppression and boss completion bits."); }

 {CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=2;pickupCount=0;localOwner=1;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=2;localFrame.layout=remoteFrame.layout=123;
 ResetGarage();M<short>(RcActor+0x14)=8;M<short>(RcActor+0x7e)=202;M<short>(RcActor+0x16)=-1;
 M<unsigned short>(RcActor+0x40)=0x200;M<unsigned>(0x52f2f8)=0;
 BeginGarage();assert(localFrame.garage.revision&&localFrame.garage.owner==1);
 M<unsigned>(0x52f2f8)=2;M<unsigned>(0x52f668)=64;M<unsigned>(0x52f620)=2;M<unsigned>(0x52ad64)=1;M<unsigned>(0x52f6b0)=7;M<unsigned>(0x52f5e0)=1;M<int>(RcActor)=777;
 EndGarage();CoopGarage race=localFrame.garage;assert(race.node==64&&race.laps==2&&race.region==7&&CoopValidFrame(localFrame));
 localOwner=0;remoteFrame.garage=race;localFrame.garage={};M<unsigned>(0x52f528)=33;M<unsigned>(0x52f4f8)=22;M<unsigned>(0x52f584)=11;M<unsigned>(0x52f580)=9;
 BeginGarage();assert(raceSpectator&&M<unsigned>(0x52f2f8)==0);M<int>(RcActor)=0;M<unsigned>(0x52f668)=0;EndGarage();
 assert(M<int>(RcActor)==777&&M<unsigned>(0x52f668)==64&&M<unsigned>(0x52f620)==2&&M<unsigned>(0x52f6b0)==7);
 assert(M<unsigned>(0x52f528)==33&&M<unsigned>(0x52f4f8)==22&&M<unsigned>(0x52f584)==11&&M<unsigned>(0x52f580)==9);
 assert(!OrdinaryEnemy(29));localFrame.garage.node=65;assert(!CoopValidFrame(localFrame));localFrame.garage.node=64;localFrame.level=1;assert(!CoopValidFrame(localFrame));
 shared=NULL;helperTick=0;cachedLevel=0;localFrame={};remoteFrame={};ResetGarage();memset(testData,0,sizeof(testData));
 puts("PASS: backyard RC guest-start ownership, spectator race suppression, correct level-specific counters, node 64 and House field isolation.");}
 {CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=2;pickupCount=0;localOwner=1;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=2;localFrame.layout=remoteFrame.layout=123;nativeBalance=MockBackyardBalance;platformMesh=MockBackyardBalanceMesh;
 M<int>(0x54dea0)=0;M<short>(0x52f396)=14;M<short>(0x52f38e)=1;M<int>(0x559e80)=-1;
 for(int i=0;i<2;i++){
 uintptr_t c=0x728720+i*0x34;M<short>(c+0x30)=1;M<unsigned short>(c+0x32)=2;
 int v=BalancePlatform(i,i,0,0,-448,i?224:448,8);assert(v==8&&localFrame.backyardPlatforms[i].owner==1);
 CoopPlatform state=localFrame.backyardPlatforms[i];localOwner=0;remoteFrame.worldContext=0x10000|(0x400u<<i);remoteFrame.backyardPlatforms[i]=state;localFrame.backyardPlatforms[i]={};M<unsigned short>(c+0x32)=0;M<short>(c+0x10)=0;
 int before=backyardBalanceCalls;v=BalancePlatform(i,i,999,0,-448,i?224:448,8);assert(v==8&&backyardBalanceCalls==before&&M<short>(c+0x10)==200+i&&M<short>(c+0x24)==12&&backyardBalanceMesh==i);
 M<unsigned short>(c+0x32)=2;BalancePlatform(i,i,v,0,-448,i?224:448,8);assert(localFrame.backyardPlatforms[i].owner==0&&backyardBalanceCalls==before+1);
 localOwner=1;remoteFrame.worldContext=0;M<unsigned short>(c+0x32)=0;
 }
 shared=NULL;helperTick=0;cachedLevel=0;localFrame={};remoteFrame={};nativeBalance=MockBalance;platformMesh=MockPlatformMesh;memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: both backyard balance planks, guest weight authority, observer mesh/collider/momentum and contact handoff.");}
 {CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=2;pickupCount=0;
 nativeBalance=MockQueuedBalance;platformMesh=MockQueuedBalanceMesh;
 for(int i=0;i<2;i++){
 localOwner=1;localFrame={};remoteFrame={};ResetPlatform();localFrame.level=remoteFrame.level=2;localFrame.layout=remoteFrame.layout=123;
 M<int>(0x54dea0)=0;M<short>(0x52f396)=14;M<short>(0x52f38e)=1;M<int>(0x559e80)=-1;
 uintptr_t c=0x728720+i*0x34;M<short>(c+0x30)=1;M<unsigned short>(c+0x32)=2;M<short>(c+0x10)=M<short>(c+0x24)=0;
 BalancePlatform(i,i,0,0,-448,i?224:448,8);assert(queuedMeshAngle==0);CommitQueuedBalance(i);assert(M<short>(c+0x10)==12);RefreshBackyardPlatformMeshes();assert(queuedMeshAngle==(M<short>(c+0x10)>>2)&&queuedMeshAngle==3);
 BalancePlatform(i,i,8,0,-448,i?224:448,8);assert(queuedMeshAngle==3);CoopPlatform sample=localFrame.backyardPlatforms[i];assert(sample.angle==12&&sample.delta==12);
 CommitQueuedBalance(i);assert(M<short>(c+0x10)==24);RefreshBackyardPlatformMeshes();assert(queuedMeshAngle==(M<short>(c+0x10)>>2)&&queuedMeshAngle==6);
 // An observer renders the same snapshot then commits its pending rotation.
 localOwner=0;ResetPlatform();localFrame.backyardPlatforms[i]={};remoteFrame.backyardPlatforms[i]=sample;remoteFrame.worldContext=0x10000|(0x400u<<i);M<unsigned short>(c+0x32)=0;M<short>(c+0x10)=M<short>(c+0x24)=0;
 int calls=queuedBalanceCalls;BalancePlatform(i,i,0,0,-448,i?224:448,8);assert(queuedBalanceCalls==calls&&queuedMeshAngle==3);CommitQueuedBalance(i);assert(M<short>(c+0x10)==24);RefreshBackyardPlatformMeshes();assert(queuedMeshAngle==(M<short>(c+0x10)>>2)&&queuedMeshAngle==6);
 // A held packet must neither rewind the committed angle nor replay its delta.
 BalancePlatform(i,i,0,0,-448,i?224:448,8);assert(queuedBalanceCalls==calls&&M<short>(c+0x10)==24&&M<short>(c+0x24)==0&&queuedMeshAngle==6);CommitQueuedBalance(i);assert(M<short>(c+0x10)==24);RefreshBackyardPlatformMeshes();assert(queuedMeshAngle==(M<short>(c+0x10)>>2)&&queuedMeshAngle==6);
 // A genuinely newer network sample moves the observer again.
 remoteFrame.backyardPlatforms[i]=sample;++remoteFrame.backyardPlatforms[i].revision;remoteFrame.backyardPlatforms[i].angle=36;
 BalancePlatform(i,i,0,0,-448,i?224:448,8);assert(queuedBalanceCalls==calls&&M<short>(c+0x10)==36&&M<short>(c+0x24)==12&&queuedMeshAngle==9);
 CommitQueuedBalance(i);RefreshBackyardPlatformMeshes();assert(M<short>(c+0x10)==48&&queuedMeshAngle==(M<short>(c+0x10)>>2)&&queuedMeshAngle==12);
 // Re-entering a level clears packet identity; the same initial revision is valid again.
 ResetPlatform();localFrame.backyardPlatforms[i]={};remoteFrame.backyardPlatforms[i]=sample;M<short>(c+0x10)=M<short>(c+0x24)=0;
 BalancePlatform(i,i,0,0,-448,i?224:448,8);assert(queuedBalanceCalls==calls&&M<short>(c+0x10)==12&&M<short>(c+0x24)==12&&queuedMeshAngle==3);
 CommitQueuedBalance(i);RefreshBackyardPlatformMeshes();assert(M<short>(c+0x10)==24&&queuedMeshAngle==6);
 M<unsigned short>(c+0x32)=0;
 }
 shared=NULL;helperTick=0;cachedLevel=0;localFrame={};remoteFrame={};nativeBalance=MockBalance;platformMesh=MockPlatformMesh;memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: native queued platform rotation commits across ticks; owner mesh progresses; observer follows and stale packets neither rewind nor replay motion.");}
 garageMesh=MockGarageMesh;playMusic=MockMusic;
 {CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=1;pickupCount=0;localOwner=0;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=1;localFrame.layout=remoteFrame.layout=123;
 ResetSession();M<short>(0x52b818)=0;M<int>(0x5282cc)=4;SessionPoll();
 remoteFrame.pause={1,1,1};SessionPoll();assert(M<short>(0x52b818)==1&&pauseApplied);
 M<short>(0x52b818)=0;SessionPoll();assert(localFrame.pause.revision==2&&localFrame.pause.value==0);
 remoteFrame.music={9,1,7};SessionPoll();assert(M<int>(0x5282cc)==7&&musicCalls==1);SessionPoll();assert(musicCalls==1);
 M<short>(0x52f394)=90;SessionPoll();assert(localFrame.damageFlash==90);
 remoteFrame.pause={10,1,1};SessionPoll();shared=NULL;SessionPoll();assert(M<short>(0x52b818)==0); // disconnect cannot trap a remote pause
 shared=&page;helperTick=GetTickCount();ResetSession();ResetGarage();
 M<short>(RcActor+0x14)=8;M<short>(RcActor+0x7e)=1;M<short>(RcActor+0x12)=0;M<short>(RcActor+0x16)=-1;M<unsigned short>(RcActor+0x40)=0x200;
 BeginGarage();assert(localFrame.garage.owner==0&&localFrame.garage.revision);M<unsigned>(0x52f2f8)=2;M<unsigned>(0x52f528)=3;M<int>(RcActor)=100;EndGarage();
 CoopGarage race=localFrame.garage;localOwner=1;remoteFrame.garage=race;localFrame.garage={};BeginGarage();assert(raceSpectator&&M<unsigned>(0x52f2f8)==0);M<int>(RcActor)=900;EndGarage();assert(M<int>(RcActor)==100&&M<unsigned>(0x52f2f8)==2&&M<unsigned>(0x52f528)==3);
 M<int>(0x53c5e4)=48;M<int>(0x52f504)=0;BeginShovel();assert(localFrame.shovel.owner==1);M<int>(0x52f504)=34;M<int>(0x52f5bc)=1234;EndShovel();CoopShovel shovel=localFrame.shovel;
 localOwner=0;remoteFrame.shovel=shovel;localFrame.shovel={};shovelContact=false;M<int>(0x53c5e4)=0;BeginShovel();M<int>(0x52f5bc)=999;EndShovel();assert(M<int>(0x52f5bc)==1234&&M<int>(0x52f504)==34);
 // Idle host snapshots must not outrun a joining player's contact claim.
 localFrame.shovel={1,0,0,0};remoteFrame.shovel={};M<int>(0x52f504)=0;M<int>(0x52f5bc)=0;
 EndShovel();assert(localFrame.shovel.revision==1);
 remoteFrame.shovel={2,1,2,0};BeginShovel();assert(localFrame.shovel.owner==1);
 CoopGarage preserved=race;M<short>(RcActor+0x14)=0;ReadRc(preserved);assert(!memcmp(&preserved,&race,sizeof(race)));M<short>(RcActor+0x14)=8;
 // Loss/reset beats delayed racing snapshots; music returns once and cannot be reasserted by the observer.
 localOwner=0;localFrame.garage=race;localFrame.garage.owner=0;remoteFrame.garage={};rcPreviousPhase=2;rcBackgroundTrack=4;M<unsigned>(0x52f2f8)=3;M<int>(0x5282cc)=7;
 EndGarage();assert(rcEndMusicPending);remoteFrame.music={50,1,7};int beforeMusic=musicCalls;SessionPoll();assert(M<int>(0x5282cc)==4&&localFrame.music.revision>50&&musicCalls==beforeMusic+1);SessionPoll();assert(musicCalls==beforeMusic+1);
 localOwner=1;localFrame.garage.owner=0;remoteFrame.music=localFrame.music;lastTrack=4;M<int>(0x5282cc)=7;musicRetryAt=0;SessionPoll();assert(M<int>(0x5282cc)==4&&localFrame.music.owner==0);
 assert(HealthFraction(14)==1&&HealthFraction(7)==.5f&&HealthFraction(-1)==0);
 localFrame.garage=race;localFrame.shovel=shovel;assert(CoopValidFrame(localFrame));localFrame.damageFlash=181;assert(!CoopValidFrame(localFrame));
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetGarage();ResetSession();memset(testData,0,sizeof(testData));
 puts("PASS: pause/resume/disconnect, music deduplication, native damage timer, RC observer state, shovel authority and health scale.");}


 {ResetBossPresentation();CoopPose p={};p.health=20;p.animation=0;
 assert(SmoothBossPoseAt(p,1,1000).x==0);p.x=400;SmoothBossPoseAt(p,2,1040);p.x=800;SmoothBossPoseAt(p,3,1080);
 assert(SmoothBossPoseAt(p,3,1160).x==600);p.x=999;assert(SmoothBossPoseAt(p,2,1160).x==600);
 assert(SmoothBossPoseAt(p,3,1400).x==800);p.x=0x80000;assert(SmoothBossPoseAt(p,4,1410).x==0x80000);ResetBossPresentation();
 puts("PASS: observer boss snapshot interpolation, stale rejection, stall hold and teleport reset.");}
 {CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=1;pickupCount=0;localOwner=1;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=1;localFrame.layout=remoteFrame.layout=123;
 nativeBalance=MockBalance;platformMesh=MockPlatformMesh;ResetPlatform();M<int>(0x54dea0)=5;M<short>(0x52f396)=14;remoteFrame.worldContext=0x10001;M<short>(Plank+0x30)=1;M<short>(Plank+0x32)=2;M<short>(0x52f38e)=1;
 assert(BalancePlatform(15,15,0,1,-448,448,8)==8&&localFrame.platform.owner==1&&localFrame.platform.angle==100&&plankCalls==1);
 remoteFrame.worldContext=0x10005;remoteFrame.platform=localFrame.platform;remoteFrame.platform.revision+=2;remoteFrame.platform.owner=0;remoteFrame.platform.angle=120;remoteFrame.platform.delta=-4;
 M<short>(Plank+0x32)=0;assert(BalancePlatform(15,15,0,1,-448,448,8)==8&&plankCalls==1&&M<short>(Plank+0x10)==120&&M<short>(Plank+0x24)==-4&&plankMeshCalls>0);
 // Guest-alone attic entry must acquire control BEFORE touching the plank.
 remoteFrame.worldContext=0x10001;M<short>(Plank+0x32)=0;BeginPlatformTick();int before=plankCalls;
 assert(localFrame.platform.owner==1);BalancePlatform(15,15,0,1,-448,448,8);EndPlatformTick();assert(plankCalls==before+1);
 // The host in another room must import the guest's moving collider and mesh.
 CoopPlatform guest=localFrame.platform;localOwner=0;M<int>(0x54dea0)=1;remoteFrame.worldContext=0x10005;remoteFrame.platform=guest;
 localFrame.platform={};M<short>(Plank+0x10)=0;before=plankCalls;BeginPlatformTick();EndPlatformTick();assert(plankCalls==before&&M<short>(Plank+0x10)==guest.angle);
 // With both outside the attic, keep unloaded native settling alive once/tick.
 remoteFrame.worldContext=0x10002;M<int>(0x559e80)=15;M<short>(Plank+0x32)=2;before=plankCalls;BeginPlatformTick();EndPlatformTick();
 assert(plankCalls==before+1&&M<int>(0x559e80)==15&&(M<short>(Plank+0x32)&3)==2);
 EndPlatformTick();assert(plankCalls==before+1);
 assert(PlatformOwner(0x10005,0x10105)==1&&PlatformOwner(0x10105,0x10105)==0&&PlatformOwner(0x10001,0x10005)==1);
 CoopFrame invalid={};invalid.platform=remoteFrame.platform;assert(CoopValidFrame(invalid));invalid.platform.angle=2000;assert(!CoopValidFrame(invalid));
 localOwner=0;shared=NULL;helperTick=0;cachedLevel=0;localFrame={};remoteFrame={};ResetPlatform();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: plank contact authority, observer rotation/collision/momentum, no spectator simulation and packet bounds.");}

 // Enemy snapshots are host-owned, bounded and independent of native pointers.
 {CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=1;pickupCount=4;
 localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=1;localFrame.layout=remoteFrame.layout=123;remoteFrame.pickupCount=4;
 // Rendering interpolation must not move the authoritative collision state.
 localFrame.boss={};localFrame.boss.revision=1;localFrame.boss.owner=1;localFrame.boss.pose.health=20;localFrame.boss.pose.x=100;localFrame.boss.pose.secondary=-1;localOwner=0;
 M<short>(TinRobot+0x14)=5;M<int>(TinRobot)=900;M<short>(TinRobot+0x7e)=19;
 BeginBossPresentation();assert(M<int>(TinRobot)==100&&M<short>(TinRobot+0x14)==5&&M<short>(TinRobot+0x7e)==19);EndBossPresentation();assert(M<int>(TinRobot)==900);
 ResetBossPresentation();localFrame.boss={};M<short>(TinRobot+0x14)=0;
 ResetEnemySync();localOwner=0;uintptr_t enemy=0x52c840+5*0x9c;
 M<short>(enemy+0x14)=3;M<short>(enemy+0x7e)=4;M<int>(enemy)=10000;M<int>(enemy+4)=20000;M<int>(enemy+8)=30000;
 SyncEnemies();assert(localFrame.enemies[5].revision&&localFrame.enemies[5].position[0]==10000&&localFrame.enemies[5].health==4);
 CoopEnemy authoritative=localFrame.enemies[5];remoteFrame.enemies[5]=authoritative;remoteFrame.enemies[5].damage=1;
 SyncEnemies();assert(M<short>(enemy+0x7e)==3);SyncEnemies();assert(M<short>(enemy+0x7e)==3); // duplicate request does not hurt twice
 remoteFrame.enemies[5].model=4;remoteFrame.enemies[5].damage=2;SyncEnemies();assert(M<short>(enemy+0x7e)==3);
 ResetEnemySync();localOwner=1;remoteFrame.enemies[5]=authoritative;M<int>(enemy)=90000;M<short>(enemy+0x7e)=4;
 SyncEnemies();assert(M<int>(enemy)==10000&&M<short>(enemy+0x7e)==4);
 M<short>(enemy+0x7e)=3;SyncEnemies();assert(localFrame.enemies[5].damage==1&&M<short>(enemy+0x7e)==4);
 SyncEnemies();assert(localFrame.enemies[5].damage==1); // restoring authoritative HP is not a second hit
 remoteFrame.enemies[5].revision++;remoteFrame.enemies[5].position[0]=11000;ApplyEnemy(5,1000);ApplyEnemy(5,1033);assert(M<int>(enemy)==11000);
 remoteFrame.enemies[5].revision--;remoteFrame.enemies[5].position[0]=99999;ApplyEnemy(5,1066);assert(M<int>(enemy)==11000);
 CoopSetBit(remoteFrame.deadActors,5);remoteFrame.enemies[5].revision+=2;ApplyEnemy(5,1100);assert(M<int>(enemy)==11000); // never resurrect a retired enemy
 uintptr_t target=0x52c840;M<short>(target+0x14)=2;M<short>(target+0x7e)=80;assert(!OrdinaryEnemy(0));
 localOwner=0;memset(remoteFrame.deadActors,0,sizeof(remoteFrame.deadActors));remoteFrame.pose.x=M<int>(enemy);remoteFrame.pose.y=M<int>(enemy+4);remoteFrame.pose.z=M<int>(enemy+8);
 M<short>(enemy+0x42)=123;M<unsigned short>(enemy+0x40)=0;ActivatePeerEnemies();RepairEnemyCandidates();assert(M<short>(enemy+0x42)==123&&(M<unsigned short>(enemy+0x40)&1));RestoreEnemyRanges();assert(M<short>(enemy+0x42)==123&&!(M<unsigned short>(enemy+0x40)&1));

 // Missing native membership must not hide a corrected, nearby live enemy.
 uintptr_t visible[65]={0x52f300,0};M<short>(enemy+0x42)=1000;
 M<int>(0x52adc0)=M<int>(enemy);M<int>(0x52adc4)=M<int>(enemy+4);M<int>(0x52adc8)=M<int>(enemy+8);
 assert(IncludeVisibleEnemies(visible,1)==2&&visible[0]==enemy&&visible[1]==0x52f300&&visible[2]==0);
 assert(IncludeVisibleEnemies(visible,2)==2); // no duplicates
 visible[0]=0x52f300;visible[1]=0;M<int>(0x52adc0)+=10000000;
 assert(IncludeVisibleEnemies(visible,1)==1); // local camera, not peer visibility
 M<int>(0x52adc0)=M<int>(enemy);CoopSetBit(remoteFrame.deadActors,5);
 assert(IncludeVisibleEnemies(visible,1)==1); // never resurrect
 memset(remoteFrame.deadActors,0,sizeof(remoteFrame.deadActors));
 for(unsigned j=0;j<64;j++)visible[j]=1;visible[64]=0;
 assert(IncludeVisibleEnemies(visible,64)==64&&visible[64]==0);
 visible[0]=enemy;visible[1]=0x52f300;visible[2]=0;
 M<unsigned short>(enemy+0x40)=0x10c;BeginEnemyRender(visible);
 assert(M<unsigned short>(enemy+0x40)==0x10d);EndEnemyRender();assert(M<unsigned short>(enemy+0x40)==0x10c);
 M<unsigned short>(enemy+0x40)=0x10d;BeginEnemyRender(visible);EndEnemyRender();assert(M<unsigned short>(enemy+0x40)==0x10d);
 M<unsigned short>(enemy+0x40)=0x10c;CoopSetBit(remoteFrame.deadActors,5);BeginEnemyRender(visible);assert(M<unsigned short>(enemy+0x40)==0x10c);EndEnemyRender();
 memset(remoteFrame.deadActors,0,sizeof(remoteFrame.deadActors));
 puts("PASS: native draw-enable bit forced only during live enemy rendering; original flags/dead enemies preserved.");
 puts("PASS: corrected enemy visibility, local draw radius, no duplicates/dead actors, renderer capacity.");
 CoopFrame bad={};bad.enemies[5]=authoritative;assert(CoopValidFrame(bad));bad.enemies[5].position[0]=INT32_MAX;assert(!CoopValidFrame(bad));bad.enemies[5]=authoritative;bad.enemies[5].health=80;assert(!CoopValidFrame(bad));
 memset(testData,0,sizeof(testData));ResetEnemySync();shared=NULL;helperTick=0;cachedLevel=0;pickupCount=0;localOwner=0;localFrame={};remoteFrame={};
 puts("PASS: host enemy transforms, interpolation/stale rejection, guest damage deduplication, model matching, no resurrection, peer activation and range restoration.");}
 // Irregular arrivals are rendered on a delayed timeline, not eased anew.
 CoopPose sample={};poseReady=false;
 assert(SmoothPoseAt(sample,1,1000).x==0);
 sample.x=400;SmoothPoseAt(sample,2,1040);
 sample.x=700;SmoothPoseAt(sample,3,1070);
 sample.x=1300;assert(SmoothPoseAt(sample,4,1130).x==300);
 assert(SmoothPoseAt(sample,4,1150).x==500);
 assert(SmoothPoseAt(sample,4,1180).x==800);
 assert(SmoothPoseAt(sample,4,1400).x==1300); // stall holds position
 sample.x=999;assert(SmoothPoseAt(sample,3,1410).x==1300); // stale sequence
 sample.x=0x80000;assert(SmoothPoseAt(sample,5,1420).x==0x80000); // teleport
 poseReady=false;sample.x=0;SmoothPoseAt(sample,0xfffffffe,0xfffffff0);
 sample.x=320;SmoothPoseAt(sample,1,16);
 assert(SmoothPoseAt(sample,1,100).x==160); // clock and sequence wrap
 poseReady=false;
 float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 float projection[16];memcpy(projection,identity,sizeof(identity));
 float head[3]={0,.25f,.5f},screenX,screenY;
 assert(ProjectNameplate(identity,projection,head,screenX,screenY)&&screenX==160&&screenY==90);
 assert(ProjectNameplate(identity,projection,head,screenX,screenY,512,256)&&screenX==256&&screenY==96);
 identity[12]=.5f;assert(ProjectNameplate(identity,projection,head,screenX,screenY)&&screenX==240);
 head[2]=-1;assert(!ProjectNameplate(identity,projection,head,screenX,screenY));

 if(argc==3&&!strcmp(argv[1],"--ipc")){
  HANDLE m=OpenFileMappingA(FILE_MAP_ALL_ACCESS,FALSE,argv[2]);assert(m);
  char lockName[160];sprintf(lockName,"%s_lock",argv[2]);
  HANDLE lock=OpenMutexA(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,lockName);assert(lock);
  CoopShared* page=(CoopShared*)MapViewOfFile(m,FILE_MAP_ALL_ACCESS,0,0,sizeof(CoopShared));assert(page);
  assert(WaitForSingleObject(lock,2000)==WAIT_OBJECT_0);
  assert(page->magic==CoopMagic&&page->version==CoopVersion&&page->remote.sequence==42);
  page->local=page->remote;page->local.sequence=1234;page->local.pose.x=67890;page->gameTick=GetTickCount();
  ReleaseMutex(lock);UnmapViewOfFile(page);CloseHandle(lock);CloseHandle(m);
  puts("PASS: native process read/write of managed shared-memory bridge.");return 0;
 }
 assert(offsetof(CoopFrame,pose)==16&&offsetof(CoopFrame,coins)==48);
 assert(offsetof(CoopShared,remote)==16+sizeof(CoopFrame));
 CoopFrame frame={};assert(CoopValidFrame(frame));
 frame.pose.animation=35;assert(!CoopValidFrame(frame));frame.pose.animation=0;
 frame.pickupCount=513;assert(!CoopValidFrame(frame));frame.pickupCount=512;
 frame.pose.x=INT32_MAX;assert(!CoopValidFrame(frame));frame.pose.x=0;
 frame.reserved=1;assert(!CoopValidFrame(frame));frame.reserved=0;
 frame.shots[0].sequence=1;frame.shots[0].life=32;assert(CoopValidFrame(frame));
 frame.shots[0].start[0]=INT32_MAX;assert(!CoopValidFrame(frame));frame.shots[0].start[0]=0;
 frame.shots[0].life=65;assert(!CoopValidFrame(frame));frame.shots[0].life=32;
 frame.objects[0].revision=1;frame.objects[0].owner=2;assert(!CoopValidFrame(frame));frame.objects[0].owner=1;
 assert(CoopValidFrame(frame));frame.objects[0].node=-1;assert(!CoopValidFrame(frame));frame.objects[0].node=0;
 CoopObject hostObject={},clientObject={};hostObject.revision=clientObject.revision=7;clientObject.owner=1;
 assert(CoopObjectNewer(hostObject,clientObject)&&!CoopObjectNewer(clientObject,hostObject));
 clientObject.revision=8;assert(CoopObjectNewer(clientObject,hostObject));
 hostObject=clientObject;assert(!CoopObjectNewer(clientObject,hostObject));
 // Exercise actual dropped-coin bridge code against isolated mapped data,
 // with no game executable or native engine calls.
 memset(testData,0,sizeof(testData));
 CoopShared testShared={};shared=&testShared;helperTick=GetTickCount();cachedLevel=localFrame.level=remoteFrame.level=1;
 localFrame.layout=remoteFrame.layout=123;pickupCount=localFrame.pickupCount=remoteFrame.pickupCount=0;
 for(unsigned n=0;n<64;n++)dropSource[n]=-1;
 dropSource[0]=3;M<short>(0x52f39e)=10;
 assert(CollectDrop(0x529e58)==1&&CoopBit(localFrame.drops,3)); // native awards once
 ++M<short>(0x52f39e);assert(CollectDrop(0x529e58)==0&&M<short>(0x52f39e)==11);
 CoopSetBit(remoteFrame.drops,3);ApplyDrops();assert(M<short>(0x52f39e)==11);
 CoopSetBit(remoteFrame.drops,4);ApplyDrops();assert(M<short>(0x52f39e)==12);
 ApplyDrops();assert(M<short>(0x52f39e)==12);
 dropSource[1]=5;CoopSetBit(remoteFrame.drops,5);
 assert(CollectDrop(0x529e94)==0&&M<short>(0x52f39e)==13); // packet arrives on contact frame
 ApplyDrops();assert(M<short>(0x52f39e)==13);
 M<short>(0x52f39e)=99;CoopSetBit(remoteFrame.drops,6);ApplyDrops();assert(M<short>(0x52f39e)==99);
 M<unsigned>(0x88278c)=1;pickupTable=0;M<uintptr_t>(0x559d6c)=0;
 unsigned short pathData[20]={10};M<uintptr_t>(0x559c70)=(uintptr_t)pathData;
 M<int>(0x53c65c)=1;M<short>(0x53c6a6)=1; // mesh resource ID
 moveObjectMesh=MockMesh;moveObjectCollision=MockCollision;nativePushables=MockPush;
 localOwner=1;lastPushed=-1;
 remoteFrame.objects[0]={};remoteFrame.objects[0].revision=5;remoteFrame.objects[0].position[0]=320;
 PushableObjects(NULL);
 assert(M<int>(0x53c680)==320&&lastMeshX==10&&lastCollisionX==320&&meshMoves&&collisionMoves);
 newPushContact=true;M<int>(0x53c648)=0;PushableObjects(NULL);newPushContact=false;
 assert(localFrame.objects[0].owner==1&&localFrame.objects[0].revision>5&&M<int>(0x53c680)==352);
 M<int>(0x53c648)=0;PushableObjects(NULL);assert(M<int>(0x53c680)==384); // last pusher owns fall
 nativeLevelTick=MockLevelTick;
 M<unsigned short>(0x52ad88)=0xf0;SharedLevelTick(1);assert(M<int>(0x53c680)==384);
 SharedLevelTick(1);assert(M<int>(0x53c680)==416&&M<unsigned short>(0x52ad88)==0xf0);
 // Aiming/no walking update advances a fall once; a normal walking frame never advances twice.
 CoopObject bad=remoteFrame.objects[0];bad.path=1;assert(!ObjectMatches(bad,0x53c680));
 bad.path=0;bad.node=10;assert(!ObjectMatches(bad,0x53c680));
 bad.node=9;bad.minimum=9;assert(ObjectMatches(bad,0x53c680)); // final fall node is valid
 remoteActorDeath=MockQuestRemoval;
 uintptr_t sheep=0x52c840+5*0x9c;M<short>(sheep+0x14)=7;M<short>(sheep+0x7e)=102;
 M<int>(0x52b7d8)=0;CoopSetBit(remoteFrame.actorCollections,5);
 M<uintptr_t>(0x52f1d0)=0x52c840;M<uintptr_t>(0x52f1d4)=0; // sheep absent from native active list
 ApplyQuestActors();assert(M<int>(0x52b7d8)==1&&questRemovals==1&&M<short>(sheep+0x14)==0);
 assert(M<uintptr_t>(0x52f1d0)==0x52c840&&M<uintptr_t>(0x52f1d4)==0);
 ApplyQuestActors();assert(M<int>(0x52b7d8)==1&&questRemovals==1);
 ++M<int>(0x52b7d8);CollectQuestActor(sheep,2);assert(M<int>(0x52b7d8)==1); // simultaneous native award
 sheep+=0x9c;M<short>(sheep+0x14)=7;M<short>(sheep+0x7e)=102;
 ++M<int>(0x52b7d8);CollectQuestActor(sheep,2);CoopSetBit(remoteFrame.actorCollections,6);
 ApplyQuestActors();assert(M<int>(0x52b7d8)==2&&CoopBit(localFrame.actorCollections,6));
 nativeActors=MockActors;remoteActorDeath=MockRemoteDeath;
 M<short>(0x52c854)=3;M<short>(0x52c8be)=10;CoopSetBit(remoteFrame.deadActors,0);
 Actors();assert(remoteDeaths==1&&M<short>(0x52c854)==3&&M<short>(0x52c8be)==999);
 Actors();assert(remoteDeaths==1); // Duplicate packets cannot replay a death/drop.
 // Native pickup replay uses stable slots, survives duplicate packets, and
 // never remotely activates an information cube or unknown script category.
 unsigned char slots[4+16*4]={};pickupTable=(uintptr_t)slots;
 pickupCount=localFrame.pickupCount=remoteFrame.pickupCount=4;
 for(int i=0;i<4;i++){pickupY[i]=123;M<int>(pickupTable+8+i*16)=INT32_MIN;}
 pickupCategoryCache[0]=0;pickupCategoryCache[1]=1;pickupCategoryCache[2]=4;pickupCategoryCache[3]=16;
 remoteFrame.collections[0]=1;remoteFrame.collections[1]=2;remoteFrame.collections[2]=1;
 assert(RemotePickup(pickupTable+4)==1&&M<int>(pickupTable+8)==123);
 RecordPickup(pickupTable+4);assert(localFrame.collections[0]==1&&!RemotePickup(pickupTable+4));
 assert(RemotePickup(pickupTable+20));RecordPickup(pickupTable+20);
 assert(localFrame.collections[1]==2&&!RemotePickup(pickupTable+20));
 assert(!RemotePickup(pickupTable+36));RecordPickup(pickupTable+36);assert(!localFrame.collections[2]);
 RecordPickup(pickupTable+52);remoteFrame.collections[3]=1;
 assert(!RemotePickup(pickupTable+52)); // simultaneous contact receives one award
 remoteFrame.collections[0]=2;assert(RemotePickup(pickupTable+4));RecordPickup(pickupTable+4);
 assert(localFrame.collections[0]==2); // same power-up after native respawn
 assert(!RemotePickup(pickupTable+5)&&!RemotePickup(pickupTable+68));
 M<unsigned char>(0x52f0d8)=1;remoteFrame.tokens=2;SyncTokens();
 assert(M<unsigned char>(0x52f0d8)==3&&localFrame.tokens==3);SyncTokens();assert(localFrame.tokens==3);
 puts("PASS: all collectible categories use deduplicated native replay; respawns, quest pickups, token union and excluded cubes.");
 pickupCategoryCache[0]=-1;remoteFrame.collections[0]=3;
 assert(RemotePickup(pickupTable+4));RecordPickup(pickupTable+4);assert(localFrame.collections[0]==3);
 assert(!RemotePickup(pickupTable+4)); // Mr Potato Head part category (-1), not an unknown script
 unsigned char targetDescriptor[2][32]={{},{}};
 for(unsigned i=0;i<2;i++){
  uintptr_t actor=0x52c840+i*2*0x9c;
  M<uintptr_t>(actor+0x98)=(uintptr_t)targetDescriptor[i];targetDescriptor[i][0x1a]=4;
  M<short>(actor+0x14)=1;M<short>(actor+0x7e)=80;
 }
 remoteFrame.scriptFlags=1;SyncCribTargets();
 assert(M<short>(0x52c8be)==79&&M<short>(0x52c9f6)==80&&localFrame.scriptFlags==1);
 M<short>(0x52c9f6)=79;SyncCribTargets();assert(localFrame.scriptFlags==3);
 targetDescriptor[0][0x1a]=0;M<short>(0x52c8be)=80;SyncCribTargets();assert(M<short>(0x52c8be)==80);
 puts("PASS: ear category replay, crib damage latches and nameplate world/view projection.");
 unsigned char commands[4096]={},bossDescriptor[32]={};testBossCommands=(uintptr_t)commands;
 M<short>(TinRobot+0x14)=5;M<short>(TinRobot+0x7e)=20;
 M<uintptr_t>(TinRobot+0x98)=(uintptr_t)bossDescriptor;M<uintptr_t>(TinRobot+0x80)=testBossCommands+20;
 CoopBoss fighting={};fighting.revision=10;fighting.owner=1;fighting.phase=2;fighting.pose.health=20;
 fighting.pose.secondary=-1;fighting.commandOffset=20;fighting.previousHealth=20;fighting.collisionFlags=7;
 remoteFrame.boss=fighting;localFrame.boss={};localOwner=0;bossLastHealth=-1;
 BeginBossTick();assert(localFrame.boss.owner==1&&M<unsigned>(0x52f578)==2&&bossDescriptor[0x1a]==7);
 M<short>(TinRobot+0x7e)=18;BeginBossTick();EndBossTick();
 assert(localFrame.boss.damage[0]==2&&M<short>(TinRobot+0x7e)==20); // guest damage request, authoritative pose
 CoopBoss guest=localFrame.boss;localOwner=1;localFrame.boss=fighting;remoteFrame.boss=guest;
 bossLastHealth=20;memset(bossDamage,0,sizeof(bossDamage));memset(bossDamageApplied,0,sizeof(bossDamageApplied));
 BeginBossTick();EndBossTick();assert(M<short>(TinRobot+0x7e)==18&&localFrame.boss.pose.health==18);
 BeginBossTick();EndBossTick();assert(M<short>(TinRobot+0x7e)==18); // deduplicated request
 remoteFrame.boss=fighting;remoteFrame.boss.owner=0;remoteFrame.boss.revision=1;
 BeginBossTick();assert(localFrame.boss.owner==0); // deterministic simultaneous-start winner
 frame.boss=fighting;assert(CoopValidFrame(frame));frame.boss.commandOffset=INT32_MAX;assert(!CoopValidFrame(frame));
 frame.boss=fighting;frame.boss.pose.animation=8;assert(!CoopValidFrame(frame));frame.boss={};
 puts("PASS: Tin Robot activation/pose import, local damage routing, duplicate damage rejection and authority conflict.");
 shared=NULL;helperTick=0;cachedLevel=0;localFrame={};remoteFrame={};
 puts("PASS: pushable native state, visible mesh and collider agree; spectator fall rejected; push ownership/fall continuation and local path bounds.");
 puts("PASS: remote death invokes native animation once instead of immediately deleting the model.");
 puts("PASS: shot bounds, object ownership conflict/stale revisions, real drop merge/contact deduplication and native coin cap.");
 for(unsigned n=0;n<512;n++){CoopSetBit(frame.coins,n);assert(CoopBit(frame.coins,n));}
 for(unsigned n=0;n<16;n++)assert(frame.coins[n]==0xffffffffu);
 uintptr_t native[65]={},out[66];
 for(unsigned n=0;n<63;n++)native[n]=100+n;
 native[63]=1;native[64]=0;out[65]=0xdeadbeef;
 assert(CoopBuildRenderList(out,native,1,2)==64);
 assert(out[62]==2&&out[63]==1&&out[64]==0&&out[65]==0xdeadbeef);
 native[0]=1;native[1]=0;
 assert(CoopBuildRenderList(out,native,1,2)==2&&out[0]==2&&out[1]==1&&out[2]==0);
 assert(CoopBuildRenderList(out,native,1,0)==1&&out[0]==1&&out[1]==0);
 for(short n=0;n<35;n++){
  assert(SafeAnimationFrame(n,-1)==0);
  assert(SafeAnimationFrame(n,INT32_MAX)==(int)((animationFrames[n]-1)<<16));
 }
 assert(SafeAnimationFrame(-1,100)==0&&SafeAnimationFrame(35,100)==0);
 assert(CoopLerpAngle(4090,6,5,10)==0&&CoopLerpAngle(6,4090,5,10)==0);
 assert(CoopLerp(-0x10000000,0x10000000,5,10)==0&&CoopLerp(10,20,20,10)==20);
 strcpy(assetRoot,"C:\\CoopTest\\");
 nativeOpenKey=MockRegistryOpen;
 HKEY software=NULL;assert(OpenGameKey(HKEY_LOCAL_MACHINE,"Software",0,KEY_ALL_ACCESS,&software)==0&&requestedAccess==KEY_READ);
 HKEY key=NULL;assert(OpenGameKey(HKEY_LOCAL_MACHINE,"TravellersTalesToyStory2",0,KEY_READ,&key)==0&&key==privateGameKey);
 DWORD length=0,type=0;assert(QueryGameKey(key,"path",NULL,&type,NULL,&length)==0&&type==REG_SZ&&length==18);
 char path[128];DWORD small=2;assert(QueryGameKey(key,"path",NULL,&type,(BYTE*)path,&small)==ERROR_MORE_DATA&&small==length);
 length=sizeof(path);assert(QueryGameKey(key,"path",NULL,&type,(BYTE*)path,&length)==0&&!strcmp(path,"C:\\CoopTest\\data\\"));
 length=sizeof(path);assert(QueryGameKey(key,"cdpath",NULL,&type,(BYTE*)path,&length)==0&&!strcmp(path,"C:\\CoopTest\\DiscData\\"));
 assert(QueryGameKey(key,"unknown",NULL,&type,(BYTE*)path,&length)==ERROR_FILE_NOT_FOUND);
 assert(CloseGameKey(key)==0);
 // A golden ABI frame is consumed by the managed tests to catch packing drift.
 frame.enemies[63].revision=7;frame.enemies[63].model=3;frame.enemies[63].position[0]=4321;frame.enemies[63].health=4;
 frame.boss=fighting;frame.actorCollections[1]=0x80000000;frame.scriptFlags=3;frame.collections[511]=123;frame.sequence=42;frame.level=1;frame.layout=0x12345678;frame.pickupCount=512;
 frame.pose.x=12345;frame.pose.y=-9876;frame.pose.yaw=1024;frame.pose.animation=6;
 frame.pose.secondary=-1;frame.pose.frame=7<<16;frame.pose.health=14;
 frame.drops[0]=0x18;frame.shots[0].sequence=11;frame.shots[0].start[0]=123;frame.shots[0].end[0]=456;
 frame.collections[511]=123;frame.objects[0].revision=12;frame.objects[0].owner=1;frame.objects[0].position[0]=789;
 frame.platform={7,1,48,-256,8};
 frame.garage.revision=3;frame.garage.owner=1;frame.garage.health=202;frame.garage.phase=2;frame.garage.node=12;frame.shovel={4,1,34,1234};frame.pause={5,0,1};frame.music={6,1,7};frame.damageFlash=90;
 frame.worldContext=0x10105;frame.spawnedTokens=5;frame.enemyAnimations[63]={3,-1,0x12345,0};
 frame.rcPushSequence=9;frame.rcPushAngle=1024;frame.burrow.revision=1;frame.burrow.hole=2;frame.burrow.previous=1;frame.burrow.timer=60;frame.burrow.closed=3;
 frame.backyardPlatforms[0]={1,1,8,200,12};frame.backyardPlatforms[1]={1,0,-8,224,-12};
 frame.mower.revision=1;frame.mower.pose.x=7654;frame.duck.revision=1;frame.duck.owner=1;frame.duck.value[3]=4096;frame.tree={1,581,20};frame.travel={2,1,15};
 FILE* golden=fopen("controls-src/coop/native-frame.bin","wb");assert(golden);
 assert(fwrite(&frame,1,sizeof(frame),golden)==sizeof(frame));fclose(golden);
 puts("PASS: native ABI, bounded render list/local-last, frame rejection, coin bits, animation limits, portable asset path responses. No game launched.");
 return 0;
}


