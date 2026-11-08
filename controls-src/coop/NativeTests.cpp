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
static int tokenReveals,tokenHides;
static void __cdecl MockReveal(int index,int quiet){assert(quiet==1);++tokenReveals;M<int>(0x830cd0+index*16)=1;}
static void __cdecl MockHide(int index){++tokenHides;M<int>(0x830cd0+index*16)=0;}
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
static int mechScales[128][3],mechRotations[128][3],mechPositions[128][3],mechToggles[32],mechCopies,mechSettles,mechToggleCalls;
static void __cdecl MockMechScale(int id,int x,int y,int z){assert(id>=0&&id<128);mechScales[id][0]=x;mechScales[id][1]=y;mechScales[id][2]=z;}
static void __cdecl MockMechRotate(int id,int x,int y,int z){assert(id>=0&&id<128);mechRotations[id][0]=x;mechRotations[id][1]=y;mechRotations[id][2]=z;}
static void __cdecl MockMechPosition(int id,int x,int y,int z){assert(id>=0&&id<128);mechPositions[id][0]=x;mechPositions[id][1]=y;mechPositions[id][2]=z;}
static void __cdecl MockMechEnable(int id){assert(id>=0&&id<32);mechToggles[id]=1;++mechToggleCalls;}
static void __cdecl MockMechDisable(int id){assert(id>=0&&id<32);mechToggles[id]=-1;++mechToggleCalls;}
static void __cdecl MockMechCopy(int a,int b){assert(a==0x16&&b==0x15);++mechCopies;}
static void __cdecl MockMechSettle(){++mechSettles;}
static void SetRemoteMechanism(unsigned i,uint32_t revision,unsigned owner,int a,int b=0,int c=0){CoopMechanism& m=remoteFrame.mechanisms[i];m.revision=revision;m.owner=owner;m.value[0]=a;m.value[1]=b;m.value[2]=c;m.value[3]=0;}
static void TestMechanisms(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;
 mechScale=MockMechScale;mechRotate=MockMechRotate;mechPosition=MockMechPosition;mechEnable=MockMechEnable;mechDisable=MockMechDisable;mechCopy=MockMechCopy;mechSettle=MockMechSettle;
 // ---- House: room-one trigger is one-way progress, replayed once.
 cachedLevel=1;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=1;localFrame.layout=remoteFrame.layout=123;ResetMechanisms();
 localOwner=1;M<int>(0x54dea0)=4;M<int>(0x52f5a0)=0;SetRemoteMechanism(1,1,0,0x200);
 SyncMechanisms();assert(M<int>(0x52f5a0)==0x200&&mechToggles[8]==1&&mechScales[0x15][0]==0x200&&mechCopies==1);
 SyncMechanisms();assert(mechCopies==1&&localFrame.mechanisms[1].value[0]==0x200);
 M<int>(0x52f5a0)=0x400;SyncMechanisms();assert(M<int>(0x52f5a0)==0x400&&mechCopies==1); // never rewinds
 assert(CoopValidFrame(localFrame));
 // ---- House: garage vents follow the room-2 owner, tolerate lag, correct real divergence.
 remoteFrame.worldContext=0x10002;M<int>(0x54dea0)=2;M<int>(0x52f588)=0x100;M<int>(0x52f568)=0x150;
 SetRemoteMechanism(0,5,0,0x1500,0x100);mechToggleCalls=0;
 SyncMechanisms();assert(M<int>(0x52f588)==0x1500&&M<int>(0x52f568)==0x150&&mechToggles[12]==1&&mechToggles[13]==-1);
 M<int>(0x52f588)=0x100;SyncMechanisms();assert(M<int>(0x52f588)==0x100); // duplicate packet ignored
 SetRemoteMechanism(0,6,0,0x180,0x100);SyncMechanisms();assert(M<int>(0x52f588)==0x100); // lag only
 SetRemoteMechanism(0,7,0,0x1f00,0x100);M<int>(0x52f588)=0x300;SyncMechanisms();assert(M<int>(0x52f588)==0x300); // wraps: distance 0x400
 localOwner=0;SetRemoteMechanism(0,8,1,0x1000,0x100);SyncMechanisms();assert(M<int>(0x52f588)==0x300); // host in room 2 owns
 // ---- Backyard: twin buttons, earliest press wins; gate stage replays side effects once.
 cachedLevel=2;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=2;localFrame.layout=remoteFrame.layout=123;ResetMechanisms();
 memset(mechScales,0xff,sizeof(mechScales));memset(mechToggles,0,sizeof(mechToggles));
 localOwner=0;M<int>(0x54dea0)=1;M<int>(0x52f6c8)=0x1000;M<int>(0x52f6cc)=0x800;M<int>(0x52f610)=0;M<int>(0x52f6a8)=0;M<int>(0x52f5f0)=0x2ee;M<int>(0x52f5e4)=0;
 SetRemoteMechanism(0,1,1,0x900,0x1000,0);SyncMechanisms();
 assert(M<int>(0x52f6c8)==0x900&&M<int>(0x52f6cc)==0x800&&mechScales[0x20][0]==0x1000&&mechScales[0x22][0]==0&&mechScales[2][1]==0x900&&mechScales[0x21][0]==-1);
 SetRemoteMechanism(0,2,1,0x700,0x1000,1);SyncMechanisms();
 assert(M<int>(0x52f610)==1&&mechToggles[5]==1&&mechToggles[6]==-1&&mechSettles==1&&M<int>(0x52f6c8)==0x700);
 SetRemoteMechanism(0,3,1,0,0,2);SyncMechanisms();
 assert(M<int>(0x52f610)==2&&mechSettles==1&&M<int>(0x52f614)==0x80&&mechRotations[8][2]==0x80&&mechScales[4][0]==0&&mechScales[0x15][1]==0x1000);
 SetRemoteMechanism(0,4,1,0,0,0);SyncMechanisms();assert(M<int>(0x52f610)==2); // no rewind
 // ---- Backyard: toppling board.
 SetRemoteMechanism(1,1,1,0,0x2a0);SyncMechanisms();assert(M<int>(0x52f5f0)==0x2a0&&M<int>(0x52f5e4)==0&&mechToggles[2]==0); // push progress
 M<int>(0x53c648)=1;SetRemoteMechanism(1,2,1,0x40,0x200);SyncMechanisms();
 assert(M<int>(0x52f5f0)==0x200&&M<int>(0x52f5e4)==0x40&&mechToggles[2]==-1&&mechToggles[3]==-1&&mechToggles[4]==1&&M<int>(0x53c648)==0&&mechRotations[5][2]==0x200&&mechRotations[0xb][2]==0x200);
 M<int>(0x52f5f0)=0x150;SyncMechanisms();assert(M<int>(0x52f5f0)==0x150); // stale packet, native keeps falling
 SetRemoteMechanism(1,3,1,0x30,0x100);SyncMechanisms();assert(M<int>(0x52f5f0)==0x150); // host owns simultaneous fall
 mechToggleCalls=0;SetRemoteMechanism(1,4,1,INT32_MIN,0);SyncMechanisms();
 assert(M<int>(0x52f5e4)==INT32_MIN&&M<int>(0x52f5f0)==0&&mechToggleCalls==0&&localFrame.mechanisms[1].value[0]==INT32_MIN);
 assert(CoopValidFrame(localFrame));
 // ---- Backyard: rising platform keeps collision entry, rider delta and visuals together.
 uintptr_t table=0x728720+0x400;M<uintptr_t>(0x559d64)=table;M<int>(table+8)=-0x30000;M<int>(table+0x10)=0;M<int>(table+0x14)=7;
 M<int>(0x52f684)=-0x30000;M<int>(0x52f680)=0x1000;M<int>(0x52f688)=0x2000;
 assert(RiseScale(-0x2500)==0x1000&&RiseScale(-0x5826c)==0&&RiseScale(-0x30000)>0&&RiseScale(-0x30000)<0x1000);
 SetRemoteMechanism(2,1,1,0x100,-0x20000);SyncMechanisms();
 assert(M<int>(0x52f6a8)==0x100&&M<int>(0x52f684)==-0x20000&&M<int>(table+8)==-0x20000&&M<int>(table+0x10)==2&&M<int>(table+0x14)==7+0x10000);
 assert(mechPositions[0x19][0]==0x80&&mechPositions[0x19][1]==-0x1000&&mechPositions[0x19][2]==0x100&&mechScales[0x1b][1]==RiseScale(-0x20000));
 SetRemoteMechanism(2,2,1,-1,-0x2500);SyncMechanisms();assert(M<int>(0x52f6a8)==-1&&M<int>(table+0x10)==0&&mechScales[0x1b][1]==0x1000);
 SetRemoteMechanism(2,3,1,0x200,-0x10000);SyncMechanisms();assert(M<int>(0x52f6a8)==-1); // completed never restarts
 // ---- Backyard: challenge stage starts local actor 26 script once.
 testChallengeCommands=0x1000;M<short>(0x52d82c)=5;M<int>(0x52f664)=0;M<uintptr_t>(0x52d898)=0;
 SetRemoteMechanism(3,1,1,3);SyncMechanisms();
 assert(M<int>(0x52f664)==3&&M<uintptr_t>(0x52d898)==0x101c&&M<short>(0x52d890)==0&&M<int>(0x830d44)==0xb4);
 M<uintptr_t>(0x52d898)=0x2000;SetRemoteMechanism(3,2,1,0x10);SyncMechanisms();assert(M<int>(0x52f664)==0x10&&M<uintptr_t>(0x52d898)==0x2000);
 SetRemoteMechanism(3,3,1,1);SyncMechanisms();assert(M<int>(0x52f664)==0x10);
 assert(CoopValidFrame(localFrame));
 // ---- Level-specific validation.
 CoopFrame f={};f.level=2;f.mechanisms[1]={1,1,{INT32_MIN,0,0,0}};assert(CoopValidFrame(f));
 f.mechanisms[3]={1,0,{0xc9,0,0,0}};assert(!CoopValidFrame(f));f.mechanisms[3]={};
 f.mechanisms[0]={1,0,{0,0,3,0}};assert(!CoopValidFrame(f));f.mechanisms[0]={};
 f.mechanisms[1].owner=2;assert(!CoopValidFrame(f));f.mechanisms[1].owner=0;
 f.level=1;assert(!CoopValidFrame(f));f.mechanisms[1]={};f.mechanisms[2]={1,0,{}};assert(!CoopValidFrame(f));
 f.mechanisms[2]={};f.mechanisms[0]={1,0,{0x2000,0,0,0}};assert(!CoopValidFrame(f));
 f.level=3;f.mechanisms[0]={1,0,{}};assert(!CoopValidFrame(f));
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};
 memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));memset(testSession,0,sizeof(testSession));testChallengeCommands=0;
 puts("PASS: House vents/trigger and Backyard buttons, gate, toppling board, rising platform and challenge stage merge once, never rewind, and validate per level.");
}
static void SetBossActor(short model,short health,int x){uintptr_t a=0x52c840;M<short>(a+0x14)=model;M<short>(a+0x7e)=health;M<int>(a)=x;}
static CoopScriptActor HostBoss(uint32_t revision,int x,short health,uint32_t guestDamage=0){
 CoopScriptActor a={};a.revision=revision;a.slot=0;a.owner=0;a.live=1;a.pose.x=x;a.pose.health=health;a.pose.animation=-1;a.pose.secondary=-1;a.commandOffset=-1;a.timer=-1;a.damage[1]=guestDamage;a.collision=4;return a;
}
static void SetHostVars(uint32_t revision,int phase,int damage,int expected){
 CoopScriptVars& v=remoteFrame.script;v={};v.revision=revision;v.owner=0;v.value[0]=phase;v.value[1]=damage;v.value[2]=0x800;v.value[3]=0x600;v.value[4]=expected;v.value[5]=0;v.value[6]=0xb4;
}
static void TestScriptBoss3(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;
 mechScale=MockMechScale;memset(mechScales,0xff,sizeof(mechScales));
 cachedLevel=3;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=3;localFrame.layout=remoteFrame.layout=123;ResetScript();
 // Guest observer: host activated the fight; guest adopts owner, pose and phase.
 localOwner=1;SetBossActor(7,100,555);M<int>(0x52f754)=0;M<int>(0x52f6f0)=0x1000;M<int>(0x52f730)=100;
 uintptr_t table=0x728720+0x900;M<uintptr_t>(0x559d6c)=table;M<int>(table+8)=(int)0x80000000;M<int>(0x52f72c)=0x4444;
 uintptr_t descriptor=0x728720+0xa00;M<uintptr_t>(0x52c8d8)=descriptor;M<unsigned char>(descriptor+0x1a)=7;
 M<unsigned char>(0x529e7c+9)=0x3f;M<short>(0x529e7c)=0;
 remoteFrame.scriptActors[0]=HostBoss(5,9000,100);SetHostVars(3,0x3e7,0x1000,100);
 assert(!OrdinaryEnemy(0)&&ScriptActorSlot(0)&&!ScriptActorSlot(1));
 BeginScript();assert(scriptOwner==0&&M<int>(0x52c840)==9000&&M<int>(0x52f754)==0x3e7&&M<int>(0x52f75c)==0x800&&M<unsigned char>(descriptor+0x1a)==4);
 EndScript();assert(localFrame.scriptActors[0].revision==5&&localFrame.scriptActors[0].owner==0&&localFrame.script.owner==0&&CoopValidFrame(localFrame));
 // A local hit becomes one request; native never sees it (health restored).
 M<short>(0x52c8be)=88;BeginScript();assert(scriptRequests[0]==1&&M<short>(0x52c8be)==100);
 EndScript();assert(localFrame.scriptActors[0].damage[1]==1);
 BeginScript();EndScript();assert(scriptRequests[0]==1); // no double count
 // Owner damage advances: observer replays rearm per step and the 0x3000 reveal once.
 SetHostVars(4,0x3e7,0x3000,100);BeginScript();
 assert(M<int>(0x52f6f0)==0x3000&&mechScales[0x32][0]==0x1000&&M<int>(table+8)==0x4444&&M<int>(0x52f72c)==0&&M<short>(0x529e7c)==1);
 M<short>(0x529e7c)=0;mechScales[0x32][0]=-1;BeginScript();assert(mechScales[0x32][0]==-1&&M<short>(0x529e7c)==0); // duplicate packet
 SetHostVars(5,0x3e7,0x1800,100);BeginScript();assert(M<int>(0x52f6f0)==0x3000); // never rewinds
 // Defeat: fall start of mesh 0 replayed once; observer stops writing after native removal.
 SetHostVars(6,0x3e8,0x3800,100);M<int>(0x52c844)=0x10000;BeginScript();assert(M<int>(0x52f754)==0x3e8&&M<int>(0x52f6f4)==0x10000-0x2ee0);
 M<int>(0x52f754)=0x3ea;remoteFrame.scriptActors[0]=HostBoss(9,1234,100);M<int>(0x52c840)=777;BeginScript();assert(M<int>(0x52c840)==777&&M<int>(0x52f754)==0x3ea);
 // Host owner: guest requests are applied as single native hits, exactly once.
 ResetScript();localFrame={};localFrame.level=3;localFrame.layout=123;remoteFrame.script={};remoteFrame.scriptActors[0]={};
 localOwner=0;SetBossActor(7,100,555);M<int>(0x52f754)=1;M<int>(0x52f730)=100;
 BeginScript();assert(scriptOwner==0);EndScript();assert(localFrame.scriptActors[0].revision==1&&localFrame.scriptActors[0].owner==0&&localFrame.script.owner==0);
 remoteFrame.scriptActors[0]=localFrame.scriptActors[0];remoteFrame.scriptActors[0].damage[1]=2;
 BeginScript();assert(M<short>(0x52c8be)==100&&scriptIncorporated[0]==0); // intro: held
 M<int>(0x52f754)=0x3e7;BeginScript();assert(M<short>(0x52c8be)==99&&scriptIncorporated[0]==1);
 BeginScript();assert(M<short>(0x52c8be)==99&&scriptIncorporated[0]==1);  // native hit pending
 M<short>(0x52c8be)=100;BeginScript();assert(M<short>(0x52c8be)==99&&scriptIncorporated[0]==2);
 M<short>(0x52c8be)=100;BeginScript();assert(M<short>(0x52c8be)==100&&scriptIncorporated[0]==2); // duplicate packet
 EndScript();assert(localFrame.scriptActors[0].damage[1]==2&&CoopValidFrame(localFrame));
 // Observer values never flow back into the owner's phase.
 remoteFrame.script={};remoteFrame.script.revision=9;remoteFrame.script.owner=0;remoteFrame.script.value[0]=0x400;remoteFrame.script.value[1]=0x3800;
 BeginScript();assert(M<int>(0x52f754)==0x3e7);
 // Simultaneous claim: a guest owner yields to the host.
 ResetScript();localOwner=1;M<int>(0x52f754)=1;remoteFrame.scriptActors[0]={};remoteFrame.script={};
 BeginScript();assert(scriptOwner==1);EndScript();
 remoteFrame.scriptActors[0]=HostBoss(1,4321,100);BeginScript();assert(scriptOwner==0&&M<int>(0x52c840)==4321);
 // Per-level validation.
 CoopFrame f={};f.level=3;f.script.revision=1;f.script.owner=0;f.script.value[1]=0x1000;assert(CoopValidFrame(f));
 f.script.value[1]=0x3801;assert(!CoopValidFrame(f));f.script.value[1]=0x1000;
 f.script.value[20]=1;assert(!CoopValidFrame(f));f.script.value[20]=0;
 f.scriptActors[0]=HostBoss(1,0,100);assert(CoopValidFrame(f));
 f.scriptActors[0].slot=1;assert(!CoopValidFrame(f));f.scriptActors[0].slot=0;
 f.scriptActors[0].pose.health=1001;assert(!CoopValidFrame(f));f.scriptActors[0].pose.health=100;
 f.scriptActors[1]=HostBoss(1,0,100);assert(!CoopValidFrame(f));f.scriptActors[1]={};
 f.level=4;f.scriptActors[0]={};assert(!CoopValidFrame(f));
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();
 memset(testData,0,sizeof(testData));
 puts("PASS: level 3 boss: owner snapshot/phase follow, observer hit requests applied once as native hits, damage reveal/rearm replayed once, no rewind, retirement, host claim priority and per-level validation.");
}
static void SetHost6(uint32_t revision,int phase,int expected){CoopScriptVars& v=remoteFrame.script;v={};v.revision=revision;v.owner=0;v.value[0]=phase;v.value[1]=expected;v.value[3]=0x200;v.value[4]=0x40000;}
static void TestScriptBoss6(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;
 cachedLevel=6;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=6;localFrame.layout=remoteFrame.layout=123;ResetScript();
 localOwner=1;SetBossActor(9,60,100);M<int>(0x52f9a4)=0;M<int>(0x52f98c)=60;
 remoteFrame.scriptActors[0]=HostBoss(3,5000,60);SetHost6(2,2,60);
 BeginScript();assert(scriptOwner==0&&M<int>(0x52f9a4)==2&&M<int>(0x52c840)==5000&&M<int>(0x52f9b8)==0x40000&&M<int>(0x52f9a0)==0x200);
 EndScript();assert(CoopValidFrame(localFrame));
 M<short>(0x52c8be)=45;BeginScript();assert(scriptRequests[0]==15&&M<short>(0x52c8be)==60&&M<int>(0x52f98c)==60); // native sees no hit
 EndScript();assert(localFrame.scriptActors[0].damage[1]==15);
 // Defeat snapshot is written once (health <= 10), then native retirement runs locally.
 remoteFrame.scriptActors[0]=HostBoss(4,5100,8);SetHost6(3,3,8);BeginScript();assert(M<int>(0x52f9a4)==3&&M<short>(0x52c8be)==8&&M<int>(0x52c840)==5100);
 remoteFrame.scriptActors[0]=HostBoss(5,5200,8);M<int>(0x52c840)=1;BeginScript();assert(M<int>(0x52c840)==1);
 // Host applies HP requests once, never writes 0 and holds while a native hit is pending.
 ResetScript();localFrame={};localFrame.level=6;localFrame.layout=123;remoteFrame.script={};remoteFrame.scriptActors[0]={};
 localOwner=0;SetBossActor(9,60,100);M<int>(0x52f9a4)=2;M<int>(0x52f98c)=60;BeginScript();EndScript();
 remoteFrame.scriptActors[0]=localFrame.scriptActors[0];remoteFrame.scriptActors[0].damage[1]=15;
 BeginScript();assert(M<short>(0x52c8be)==45&&scriptIncorporated[0]==15);
 BeginScript();assert(M<short>(0x52c8be)==45&&scriptIncorporated[0]==15);
 M<int>(0x52f98c)=45;remoteFrame.scriptActors[0].damage[1]=200;BeginScript();assert(M<short>(0x52c8be)==1&&scriptIncorporated[0]==200);
 EndScript();assert(CoopValidFrame(localFrame));
 CoopFrame f={};f.level=6;f.script.revision=1;f.script.value[0]=5;assert(!CoopValidFrame(f));f.script.value[0]=4;assert(CoopValidFrame(f));
 f.script.value[4]=0x40001;assert(!CoopValidFrame(f));
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();memset(testData,0,sizeof(testData));
 puts("PASS: level 6 boss: owner pose/attack-cycle vars, observer HP requests deduplicated, defeat snapshot written once before local retirement, no zero-health write.");
}
static void TestScriptBoss12(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;
 cachedLevel=12;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=12;localFrame.layout=remoteFrame.layout=123;ResetScript();
 // Host owner holds guest requests until phase 2, then applies them once.
 localOwner=0;SetBossActor(4,50,100);M<int>(0x52fe24)=1;M<int>(0x52fe00)=50;BeginScript();EndScript();
 remoteFrame.scriptActors[0]=localFrame.scriptActors[0];remoteFrame.scriptActors[0].damage[1]=12;
 BeginScript();assert(M<short>(0x52c8be)==50&&scriptIncorporated[0]==0);
 M<int>(0x52fe24)=2;BeginScript();assert(M<short>(0x52c8be)==38&&scriptIncorporated[0]==12);
 BeginScript();assert(M<short>(0x52c8be)==38&&scriptIncorporated[0]==12);
 // Guest observer: attack cycle stays local, owner phase/expected health follow.
 ResetScript();localFrame={};localFrame.level=12;localFrame.layout=123;localOwner=1;
 SetBossActor(4,50,100);M<int>(0x52fe24)=0;M<int>(0x52fe30)=0x77;M<int>(0x52fe00)=50;
 remoteFrame.scriptActors[0]=HostBoss(7,3000,38);remoteFrame.script={};remoteFrame.script.revision=4;remoteFrame.script.owner=0;
 remoteFrame.script.value[0]=2;remoteFrame.script.value[1]=38;remoteFrame.script.value[2]=0x3c;
 BeginScript();assert(M<int>(0x52fe24)==2&&M<int>(0x52fe00)==38&&M<short>(0x52c8be)==38&&M<int>(0x52fe04)==0x3c&&M<int>(0x52fe30)==0x77&&M<int>(0x52c840)==3000);
 M<short>(0x52c8be)=30;BeginScript();assert(scriptRequests[0]==8&&M<short>(0x52c8be)==38);
 EndScript();assert(CoopValidFrame(localFrame));
 CoopFrame f={};f.level=12;f.script.revision=1;f.script.value[3]=0x100001;assert(!CoopValidFrame(f));f.script.value[3]=0;f.script.value[4]=1;assert(!CoopValidFrame(f));
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();memset(testData,0,sizeof(testData));
 puts("PASS: level 12 boss: owner phase/health follow, local attack cycle, guest HP requests held until the fight and applied once.");
}
static int colliderRotations[32][3],colliderRotateCalls;
static void __cdecl MockColliderRotate(int id,int x,int y,int z){assert(id>=0&&id<32);colliderRotations[id][0]=x;colliderRotations[id][1]=y;colliderRotations[id][2]=z;++colliderRotateCalls;}
static void TestScriptLevel4(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;
 mechRotate=MockMechRotate;scriptColliderRotate=MockColliderRotate;memset(mechRotations,0,sizeof(mechRotations));colliderRotateCalls=0;
 cachedLevel=4;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=4;localFrame.layout=remoteFrame.layout=123;ResetScript();
 localOwner=1;M<int>(0x52f784)=8|1;M<int>(0x52f770)=2;M<int>(0x52f804)=0;
 M<short>(0x52d6f4)=6;M<uintptr_t>(0x52d760)=0;testScriptCommands[4]=0x3000;
 // Local lift-motion bit 8 is never published; only target bits are.
 EndScript();assert(localFrame.script.value[0]==1&&localFrame.script.value[1]==2&&localFrame.script.owner==2&&CoopValidFrame(localFrame));
 remoteFrame.script={};remoteFrame.script.revision=1;remoteFrame.script.owner=2;remoteFrame.script.value[0]=4;remoteFrame.script.value[1]=5;remoteFrame.script.value[2]=3;
 BeginScript();
 assert(M<int>(0x52f784)==(8|1|4)&&M<int>(0x52f770)==7&&M<int>(0x52f804)==3&&colliderRotateCalls==1&&colliderRotations[0x18][0]==-0x180&&mechRotations[0x43][0]==-0x180);
 assert(M<uintptr_t>(0x52d760)==0x3018&&M<short>(0x52d758)==0&&M<int>(0x830d44)==0xb4&&scriptOwner==2);
 M<uintptr_t>(0x52d760)=0x5555;BeginScript();assert(colliderRotateCalls==1&&M<uintptr_t>(0x52d760)==0x5555); // replayed once
 remoteFrame.script.value[0]=0;remoteFrame.script.value[2]=1;remoteFrame.script.revision=2;BeginScript();assert(M<int>(0x52f784)==13&&M<int>(0x52f804)==3); // never rewinds
 EndScript();assert(localFrame.script.value[0]==5&&localFrame.script.value[2]==3);
 CoopFrame f={};f.level=4;f.script.revision=1;f.script.owner=2;f.script.value[0]=8;assert(!CoopValidFrame(f));f.script.value[0]=7;assert(CoopValidFrame(f));
 f.scriptActors[0].revision=1;assert(!CoopValidFrame(f));
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();memset(testData,0,sizeof(testData));testScriptCommands[4]=0;
 puts("PASS: level 4 target/sequence bits OR-merge with collider+mesh replay once, local lift bit kept private, actor 24 stage replays its command start once, never rewinds.");
}
static void TestRewardStages(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;localOwner=1;
 for(unsigned i=0;i<sizeof(rewardStages)/sizeof(rewardStages[0]);i++){
  const RewardStage& r=rewardStages[i];
  cachedLevel=r.level;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=r.level;localFrame.layout=remoteFrame.layout=123;ResetScript();
  const ScriptLevelDesc* d=ScriptLevel();assert(d);unsigned index=r.level==4?2:0;assert(d->vars[index].address==r.stage);
  uintptr_t actor=0x52c840+r.slot*0x9c,descriptor=0x728720+0xc00;
  M<short>(actor+0x14)=5;M<short>(actor+0x78)=77;M<uintptr_t>(actor+0x80)=0;M<uintptr_t>(actor+0x98)=descriptor;M<unsigned char>(descriptor+0xf)=9;
  M<int>(r.stage)=0;M<int>(0x830d44)=0;testScriptCommands[(r.commandBase-0x4e02fc)/4]=0x7000;
  remoteFrame.script={};remoteFrame.script.revision=1;remoteFrame.script.owner=2;remoteFrame.script.value[index]=5;
  BeginScript();
  assert(M<int>(r.stage)==5&&M<short>(actor+0x78)==0&&M<uintptr_t>(actor+0x80)==0x7000+r.offset&&M<int>(0x830d44)==0xb4);
  assert(M<unsigned char>(descriptor+0xf)==(r.descriptor?0:9));
  if(r.level==8)assert(M<short>(actor+0x8a)==0x190&&M<int>(0x52fb9c)==0x258);
  M<uintptr_t>(actor+0x80)=0x1234;remoteFrame.script.value[index]=0x40;remoteFrame.script.revision=2;BeginScript();
  assert(M<int>(r.stage)==0x40&&M<uintptr_t>(actor+0x80)==0x1234); // crossing replayed once
  remoteFrame.script.value[index]=1;remoteFrame.script.revision=3;BeginScript();assert(M<int>(r.stage)==0x40); // no rewind
  EndScript();assert(localFrame.script.value[index]==0x40&&CoopValidFrame(localFrame));
  // A friend already gone locally is not restarted.
  M<int>(r.stage)=0;M<short>(actor+0x14)=0;M<uintptr_t>(actor+0x80)=0x1234;remoteFrame.script.value[index]=3;remoteFrame.script.revision=4;BeginScript();
  assert(M<int>(r.stage)==3&&M<uintptr_t>(actor+0x80)==0x1234);
  CoopFrame f={};f.level=r.level;f.script.revision=1;f.script.owner=2;f.script.value[index]=0xc9;assert(!CoopValidFrame(f));
  testScriptCommands[(r.commandBase-0x4e02fc)/4]=0;
 }
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: rescued-friend reward stages (levels 4,5,7,8,10,11,13,14): max merge, native command start replayed once per crossing, absent friend untouched, no rewind, bounds.");
}
static int spawnCalls[64],animationCalls[64],deathCalls[64];
static void __cdecl MockScriptSpawn(uintptr_t actor,int mode){assert(mode==0);unsigned slot=(unsigned)((actor-0x52c840)/0x9c);++spawnCalls[slot];M<short>(actor+0x7e)=5;}
static void __cdecl MockScriptAnimation(uintptr_t actor,int animation,int script){assert(animation==1&&script==0x17);unsigned slot=(unsigned)((actor-0x52c840)/0x9c);++animationCalls[slot];M<short>(actor+0x12)=1;}
static void __cdecl MockScriptDeath(uintptr_t actor,int mode){assert(mode==1);unsigned slot=(unsigned)((actor-0x52c840)/0x9c);++deathCalls[slot];}
static CoopScriptActor HostActor(uint32_t revision,unsigned slot,int x,short health,uint32_t guestDamage=0){CoopScriptActor a=HostBoss(revision,x,health,guestDamage);a.slot=(uint8_t)slot;return a;}
static void SetHost9(uint32_t revision,int phase,int count,int pair,int a,int b,int expected,int timer){
 CoopScriptVars& v=remoteFrame.script;v={};v.revision=revision;v.owner=0;v.value[0]=phase;v.value[1]=count;v.value[2]=pair;v.value[3]=a;v.value[4]=b;v.value[5]=expected;v.value[6]=timer;v.value[8]=0x123;
}
static void TestScriptBoss9(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;
 scriptSpawnActor=MockScriptSpawn;scriptActorAnimation=MockScriptAnimation;remoteActorDeath=MockScriptDeath;
 memset(spawnCalls,0,sizeof(spawnCalls));memset(animationCalls,0,sizeof(animationCalls));memset(deathCalls,0,sizeof(deathCalls));
 cachedLevel=9;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=9;localFrame.layout=remoteFrame.layout=123;ResetScript();
 for(unsigned k=0;k<12;k++){uintptr_t a=0x52c840+k*0x9c;M<short>(a+0x14)=(short)(k==0?20:k<7?21:22);M<short>(a+0x7e)=(short)(k==0?25:k<7?1:0);}
 // Boss, satellites and every minion slot are excluded from generic enemy sync.
 for(unsigned k=0;k<12;k++)assert(ScriptActorSlot(k)&&!OrdinaryEnemy(k));
 assert(!ScriptActorSlot(12));
 // Guest observer: host owns the fight (phase 2), first pair (8,8) is out.
 localOwner=1;M<int>(0x52fc04)=0;M<int>(0x52fbd4)=1;M<int>(0x52fbf4)=0;M<int>(0x52fbc8)=7;M<int>(0x52fbc4)=7;M<int>(0x52fbb8)=25;M<int>(0x52fbc0)=0;
 remoteFrame.scriptActors[0]=HostActor(3,0,7000,25);SetHost9(2,2,1,2,8,8,25,0x258);
 BeginScript();
 assert(scriptOwner==0&&M<int>(0x52fc04)==2&&M<int>(0x52fbf4)==2&&M<int>(0x52fbc8)==8&&M<int>(0x52fbc4)==8&&M<int>(0x52c840)==7000);
 // The same pair slot is spawned once, with the native field writes.
 assert(spawnCalls[8]==1&&spawnCalls[7]==0&&M<short>(0x52c840+8*0x9c+0x42)==0xaf0&&M<short>(0x52c840+8*0x9c+0x32)==(short)-0x8000);
 assert(animationCalls[1]==1&&M<int>(0x52fc0c)==0x123);
 // Guard: the release timer is held at 0 during native passes, restored after.
 assert(M<int>(0x52fbc0)==0);
 EndScript();assert(M<int>(0x52fbc0)==0x258&&scriptSlot[1]==8&&scriptSlot[2]==-1&&CoopValidFrame(localFrame));
 // Minion snapshot follows the owner's pair slot; a shared slot is owned by one record only.
 remoteFrame.scriptActors[1]=HostActor(4,8,8800,5);remoteFrame.scriptActors[2]=HostActor(4,7,1,0);remoteFrame.scriptActors[2].live=0;
 BeginScript();assert(M<int>(0x52c840+8*0x9c)==8800&&M<short>(0x52c840+8*0x9c+0x7e)==5);
 // A local minion hit becomes a request (native keeps the owner's health).
 M<short>(0x52c840+8*0x9c+0x7e)=3;EndScript();assert(scriptRequests[1]==2&&localFrame.scriptActors[1].damage[1]==2&&M<short>(0x52c840+8*0x9c+0x7e)==5);
 // Local boss hit: request only, native expected health untouched.
 BeginScript();M<short>(0x52c8be)=22;EndScript();assert(scriptRequests[0]==3&&M<short>(0x52c8be)==25&&M<int>(0x52fbb8)==25);
 // Owner's count advance removes satellite #1 exactly once; next pair (7,7) spawns once.
 SetHost9(3,2,2,4,7,7,24,0x78);BeginScript();assert(M<int>(0x52fbd4)==2&&deathCalls[1]==1&&spawnCalls[7]==1&&animationCalls[2]==1);
 BeginScript();assert(deathCalls[1]==1&&spawnCalls[7]==1);  // duplicate packet
 SetHost9(4,2,1,2,7,7,24,0x78);BeginScript();assert(M<int>(0x52fbd4)==2&&M<int>(0x52fbf4)==4);  // never rewinds
 // Record 1 moved to slot 7: no damage is inferred across the slot change.
 EndScript();assert(scriptSlot[1]==7&&scriptRequests[1]==2);
 // Defeat snapshot written once, then local retirement.
 remoteFrame.scriptActors[0]=HostActor(9,0,7100,9);SetHost9(5,4,7,12,11,7,9,0);BeginScript();
 assert(M<int>(0x52fc04)==4&&M<short>(0x52c8be)==9&&M<int>(0x52c840)==7100&&deathCalls[6]==1&&deathCalls[2]==1);
 remoteFrame.scriptActors[0]=HostActor(10,0,1,9);M<int>(0x52c840)=5;BeginScript();assert(M<int>(0x52c840)==5);
 // Host owner: guest boss hit applied as one native hit while health == 52fbb8.
 ResetScript();localFrame={};localFrame.level=9;localFrame.layout=123;remoteFrame.script={};memset(remoteFrame.scriptActors,0,sizeof(remoteFrame.scriptActors));
 localOwner=0;M<int>(0x52fc04)=2;M<int>(0x52fbd4)=2;M<int>(0x52fbc8)=9;M<int>(0x52fbc4)=10;M<short>(0x52c8be)=24;M<int>(0x52fbb8)=24;
 M<short>(0x52c840+9*0x9c+0x7e)=4;M<short>(0x52c840+10*0x9c+0x7e)=4;
 BeginScript();EndScript();assert(localFrame.scriptActors[1].slot==9&&localFrame.scriptActors[2].slot==10&&CoopValidFrame(localFrame));
 remoteFrame.scriptActors[0]=localFrame.scriptActors[0];remoteFrame.scriptActors[0].damage[1]=3;
 remoteFrame.scriptActors[1]=localFrame.scriptActors[1];remoteFrame.scriptActors[1].damage[1]=6;
 BeginScript();assert(M<short>(0x52c8be)==21&&scriptIncorporated[0]==3&&M<short>(0x52c840+9*0x9c+0x7e)==0&&scriptIncorporated[1]==6);
 BeginScript();assert(M<short>(0x52c8be)==21&&scriptIncorporated[0]==3);   // once
 // Validation: minion slot range and bounds.
 CoopFrame f={};f.level=9;f.scriptActors[1]=HostActor(1,11,0,4);assert(CoopValidFrame(f));
 f.scriptActors[1].slot=12;assert(!CoopValidFrame(f));f.scriptActors[1].slot=6;assert(!CoopValidFrame(f));f.scriptActors[1]={};
 f.scriptActors[0]=HostActor(1,1,0,4);assert(!CoopValidFrame(f));f.scriptActors[0]={};
 f.script.revision=1;f.script.value[3]=12;assert(!CoopValidFrame(f));f.script.value[3]=7;f.script.value[1]=8;assert(!CoopValidFrame(f));
 scriptSpawnActor=(ScriptSpawnFn)0x406cd0;scriptActorAnimation=(ScriptAnimationFn)0x405cf0;remoteActorDeath=(ActorDeathFn)0x405d20;
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();memset(testData,0,sizeof(testData));
 puts("PASS: level 9 boss: owner snapshot, dynamic minion-pair slots, pair spawn/satellite opening and removal replayed once, observer release-timer guard, hit requests once, no rewind, bounds.");
}
static void SetHost15(uint32_t revision,int defeats,int f0,int f1,int f2,int e0,int e1,int e2){
 CoopScriptVars& v=remoteFrame.script;v={};v.revision=revision;v.owner=0;v.value[0]=defeats;v.value[1]=f0;v.value[2]=f1;v.value[3]=f2;v.value[4]=e0;v.value[5]=e1;v.value[6]=e2;
}
static void TestScriptBoss15(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;
 cachedLevel=15;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=15;localFrame.layout=remoteFrame.layout=123;ResetScript();
 for(unsigned k=0;k<3;k++){uintptr_t a=0x52c840+k*0x9c;M<short>(a+0x14)=(short)(30+k);M<short>(a+0x7e)=40;}
 testScriptCommands[(0x4e0348-0x4e02fc)/4]=0x728f00;
 for(unsigned k=0;k<3;k++)assert(ScriptActorSlot(k));
 assert(!ScriptActorSlot(3));
 // Guest observer: host started the fight; flags follow, commands and poses come from snapshots.
 localOwner=1;M<int>(0x530008)=0;M<int>(0x530004)=0;M<int>(0x53000c)=0;M<int>(0x530064)=0;
 for(unsigned k=0;k<3;k++)remoteFrame.scriptActors[k]=HostActor(2,k,1000*(k+1),40);
 remoteFrame.scriptActors[1].commandOffset=0x1c;SetHost15(2,0,2,2,2,40,40,40);
 BeginScript();
 assert(scriptOwner==0&&M<int>(0x52c840+0x9c)==2000&&M<uintptr_t>(0x52c840+0x9c+0x80)==0x728f1c&&M<int>(0x530028)==40);
 // Guard: flag 2 is held at 1 during native passes (callbacks cannot defeat locally).
 assert(M<int>(0x530008)==1&&M<int>(0x530004)==1&&M<int>(0x53000c)==1);
 M<short>(0x52c840+0x9c+0x7e)=5;   // local hit below 10 during the actor pass
 EndScript();assert(M<int>(0x530004)==2&&M<int>(0x530064)==0&&M<short>(0x52c840+0x9c+0x7e)==40&&scriptRequests[1]==35&&CoopValidFrame(localFrame));
 // Owner defeat of actor 1 arrives: flag 3 and defeat count follow; not re-guarded.
 remoteFrame.scriptActors[1]=HostActor(3,1,2100,8);remoteFrame.scriptActors[1].commandOffset=0x68;SetHost15(3,1,2,3,2,40,8,40);
 BeginScript();assert(M<int>(0x530004)==3&&M<int>(0x530064)==1&&M<uintptr_t>(0x52c840+0x9c+0x80)==0x728f68&&M<int>(0x530008)==1);
 EndScript();assert(M<int>(0x530008)==2);
 SetHost15(4,0,0,0,0,40,8,40);BeginScript();assert(M<int>(0x530064)==1&&M<int>(0x530004)==3);
 EndScript(); // never rewinds
 // All three defeated: retired, the last snapshot is still written once.
 remoteFrame.scriptActors[2]=HostActor(5,2,3300,9);SetHost15(5,3,3,3,3,9,8,9);BeginScript();
 assert(M<int>(0x530064)==3&&M<int>(0x52c840+2*0x9c)==3300&&M<int>(0x53000c)==3);
 EndScript();
 M<int>(0x52c840+2*0x9c)=7;remoteFrame.scriptActors[2]=HostActor(6,2,1,9);BeginScript();assert(M<int>(0x52c840+2*0x9c)==7);
 EndScript();
 // Host owner: requests held before the fight, applied once as native hits, moot after defeat.
 ResetScript();localFrame={};localFrame.level=15;localFrame.layout=123;remoteFrame.script={};memset(remoteFrame.scriptActors,0,sizeof(remoteFrame.scriptActors));
 localOwner=0;M<int>(0x530064)=0;M<int>(0x530008)=2;M<int>(0x530004)=0;M<int>(0x53000c)=0;
 for(unsigned k=0;k<3;k++){M<short>(0x52c840+k*0x9c+0x7e)=40;M<int>(level15Expected[k])=40;}
 BeginScript();assert(M<int>(0x530008)==2);
 EndScript();   // owner is never guarded
 for(unsigned k=0;k<3;k++){remoteFrame.scriptActors[k]=localFrame.scriptActors[k];remoteFrame.scriptActors[k].damage[1]=12;}
 BeginScript();assert(M<short>(0x52c8be)==28&&scriptIncorporated[0]==12&&M<short>(0x52c840+0x9c+0x7e)==40&&scriptIncorporated[1]==0);
 BeginScript();assert(M<short>(0x52c8be)==28&&scriptIncorporated[0]==12);
 M<int>(0x530004)=2;BeginScript();assert(M<short>(0x52c840+0x9c+0x7e)==28&&scriptIncorporated[1]==12);
 M<int>(0x53000c)=3;BeginScript();assert(M<short>(0x52c840+2*0x9c+0x7e)==40&&scriptIncorporated[2]==12);
 EndScript();assert(CoopValidFrame(localFrame));
 CoopFrame f={};f.level=15;f.script.revision=1;f.script.value[1]=4;assert(!CoopValidFrame(f));
 f.script.value[1]=3;f.script.value[10]=3;assert(!CoopValidFrame(f));f.script.value[10]=2;assert(CoopValidFrame(f));
 f.scriptActors[2]=HostActor(1,1,0,40);assert(!CoopValidFrame(f));f.scriptActors[2].slot=2;assert(CoopValidFrame(f));
 f.scriptActors[3]=HostActor(1,3,0,40);assert(!CoopValidFrame(f));
 testScriptCommands[(0x4e0348-0x4e02fc)/4]=0;
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();memset(testData,0,sizeof(testData));
 puts("PASS: level 15 boss: three owner snapshots with command scripts, observer fight-flag guard blocks local callback defeats, defeat count follows without rewind, requests once, bounds.");
}
static int colliderPlaceCalls;static int meshPositions[128][3];static void __cdecl MockAnyMeshPosition(int id,int* p){assert(id>0&&id<128);memcpy(p,meshPositions[id],12);}static DWORD fakeColliderNow;static DWORD FastColliderClock(){return fakeColliderNow+=200;}
static void __cdecl MockColliderPlace(int id,int x,int y,int z){uintptr_t g=ColliderGeometry(id);assert(g);M<int>(g)=x;M<int>(g+4)=y;M<int>(g+8)=z;++colliderPlaceCalls;}
static void SetCollider(int id,short geometry,int x){M<short>(0x728720+id*0x34+0x30)=geometry;uintptr_t g=0x7295a8+geometry*0x34;M<int>(g)=x;M<int>(g+4)=x+1;M<int>(g+8)=x+2;}
static CoopMover HostMover(uint32_t revision,unsigned owner,unsigned group,int x){
 CoopMover m={};m.revision=revision;m.owner=(uint8_t)owner;m.group=(uint8_t)group;
 m.vars[0]=0x40;m.vars[1]=0x4f1bc0;m.vars[2]=3;m.vars[3]=1;
 for(unsigned k=0;k<2;k++){CoopColliderState& c=m.colliders[k];c.present=1;c.pos[0]=x+k;c.pos[1]=7;c.pos[2]=9;c.rot[2]=0x80;c.vel[1]=-5;c.delta[2]=12;c.flags=0x89;}
 return m;
}
static void TestColliderGroups(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;colliderPlace=MockColliderPlace;colliderPlaceCalls=0;colliderClock=FastColliderClock;meshPosition=MockAnyMeshPosition;
 cachedLevel=4;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=4;localFrame.layout=remoteFrame.layout=123;ResetColliderGroups();
 SetCollider(1,1,100);SetCollider(7,2,200);M<int>(0x52f7f8)=0;M<int>(0x52f874)=0x4f1ba4;M<int>(0x559e80)=-1;M<int>(0x52b81c)=0;
 assert(ColliderGroup(8)&&ColliderGroup(8)->colliders[0]==0x1a&&ColliderGroup(9)->colliders[0]==0x24&&!ColliderGroup(12));
 // Guest observer: the host stands on pulley collider 1 and owns the group.
 localOwner=1;remoteFrame.standing=1;remoteFrame.movers[0]=HostMover(5,0,0,5000);
 SyncColliderGroups(false);
 uintptr_t c1=0x728720+1*0x34;
 assert(localFrame.movers[0].owner==0&&M<int>(0x7295a8+0x34)==5000&&M<int>(0x7295a8+2*0x34)==5001&&M<short>(c1+0x10)==0x80&&M<short>(c1+0x16)==0&&M<short>(c1+0x24)==0&&(M<unsigned short>(c1+0x32)&0x88)==8); // observer: no local prediction
 assert(M<int>(0x52f7f8)==0x40&&M<int>(0x52f874)==0x4f1bc0&&M<int>(0x52f854)==3&&M<int>(0x52f78c)==1&&CoopValidFrame(localFrame));
 // A program pointer outside toy2.exe data is never written.
 remoteFrame.movers[0].vars[1]=0x1234;remoteFrame.movers[0].revision=6;SyncColliderGroups(true);assert(M<int>(0x52f874)==0x4f1bc0);
 // Stale packet is not adopted.
 remoteFrame.movers[0]=HostMover(4,0,0,1);SyncColliderGroups(false);assert(M<int>(0x7295a8+0x34)==5000);
 // Guest steps on the counterweight while the host left: guest takes over from the last state.
 remoteFrame.standing=-1;M<int>(0x559e80)=7;SyncColliderGroups(false);
 assert(localFrame.movers[0].owner==1&&localFrame.movers[0].revision==7&&M<int>(0x7295a8+0x34)==5000&&localFrame.standing==7);
 M<int>(0x7295a8+0x34)=5100;SyncColliderGroups(true);assert(localFrame.movers[0].revision==8&&localFrame.movers[0].colliders[0].pos[0]==5100);
 int places=colliderPlaceCalls;SyncColliderGroups(true);assert(localFrame.movers[0].revision==8&&colliderPlaceCalls==places); // owner never writes, unchanged state not republished
 // Both stand on it: the host wins.
 remoteFrame.standing=1;remoteFrame.movers[0]=HostMover(9,0,0,6000);SyncColliderGroups(false);assert(localFrame.movers[0].owner==0&&M<int>(0x7295a8+0x34)==6000);
 // Nobody stands: the last owner keeps it.
 remoteFrame.standing=-1;M<int>(0x559e80)=-1;SyncColliderGroups(false);assert(localFrame.movers[0].owner==0);
 // Tipping bridge: standing on its deck surface counts as collider 0x1a.
 M<int>(0x52b81c)=0x24;SetCollider(0x1a,3,300);SyncColliderGroups(false);assert(localFrame.standing==0x1a&&localFrame.movers[8].owner==1);
 M<int>(0x52b81c)=0;
 assert(CoopValidFrame(localFrame));
 // Host owner publishes its own groups; a guest record never overrides an owned group.
 ResetColliderGroups();localOwner=0;remoteFrame={};remoteFrame.level=4;remoteFrame.layout=123;remoteFrame.standing=-1;M<int>(0x559e80)=1;
 SyncColliderGroups(false);SyncColliderGroups(true);assert(localFrame.movers[0].owner==0&&localFrame.movers[0].colliders[0].pos[0]==6000);
 // Validation: group index, level, flags and bounds.
 CoopFrame f={};f.level=4;f.standing=-1;f.movers[8]=HostMover(1,0,8,0);assert(CoopValidFrame(f));
 f.movers[12]=HostMover(1,0,12,0);assert(!CoopValidFrame(f));f.movers[12]={};
 f.movers[8].group=7;assert(!CoopValidFrame(f));f.movers[8].group=8;
 f.movers[8].colliders[0].flags=2;assert(!CoopValidFrame(f));f.movers[8].colliders[0].flags=0x88;f.movers[8].requestVar=8;assert(!CoopValidFrame(f));f.movers[8].requestVar=0;
 f.movers[8].vars[4]=0x10000001;assert(!CoopValidFrame(f));f.movers[8].vars[4]=0;
 f.standing=256;assert(!CoopValidFrame(f));f.standing=-1;
 f.level=6;assert(!CoopValidFrame(f));f.level=5;f.movers[8]={};f.movers[5]=HostMover(1,1,5,0);assert(CoopValidFrame(f));
 f.level=13;f.movers[7]=HostMover(1,1,7,0);assert(!CoopValidFrame(f));f.movers[7]={};f.movers[5]={};f.movers[1]=HostMover(1,1,1,0);assert(CoopValidFrame(f));
 // Group table and the wire count table agree for every level.
 for(unsigned level=0;level<16;level++){unsigned n=0;for(unsigned i=0;i<colliderGroupCount;i++)if(colliderGroups[i].level==level)++n;assert(n==CoopMoverGroupCounts[level]&&n<=24);}
 // Trigger request: an observer's native tick raises the level 4 bridge state; the owner applies it once (CAS).
 cachedLevel=4;ResetColliderGroups();localOwner=1;remoteFrame={};remoteFrame.level=4;remoteFrame.layout=123;remoteFrame.standing=-1;M<int>(0x559e80)=-1;
 remoteFrame.movers[8]=HostMover(3,0,8,300);remoteFrame.movers[8].vars[0]=0;M<int>(0x52f7e0)=0;
 SyncColliderGroups(false);assert(M<int>(0x52f7e0)==0);M<int>(0x52f7e0)=1;SyncColliderGroups(true);
 assert(localFrame.movers[8].requestCount==1&&localFrame.movers[8].requestFrom==0&&localFrame.movers[8].requestTo==1&&M<int>(0x52f7e0)==0);
 SyncColliderGroups(false);M<int>(0x52f7e0)=1;SyncColliderGroups(true);assert(localFrame.movers[8].requestCount==1); // pending: no repeat
 CoopMover request=localFrame.movers[8];
 ResetColliderGroups();localOwner=0;localFrame.movers[8]=request;remoteFrame.movers[8]=request;M<int>(0x52f7e0)=0;M<int>(0x559e80)=0x1a;
 SyncColliderGroups(false);assert(M<int>(0x52f7e0)==1);M<int>(0x52f7e0)=5;SyncColliderGroups(false);assert(M<int>(0x52f7e0)==5); // applied once
 ResetColliderGroups();localFrame.movers[8]=request;M<int>(0x52f7e0)=7;SyncColliderGroups(false);assert(M<int>(0x52f7e0)==7);   // owner moved on: CAS refused
 // Observer clears its pending request once the owner's value moved on.
 ResetColliderGroups();localOwner=1;M<int>(0x559e80)=-1;remoteFrame.movers[8]=HostMover(3,0,8,300);remoteFrame.movers[8].vars[0]=0;
 SyncColliderGroups(false);M<int>(0x52f7e0)=1;SyncColliderGroups(true);remoteFrame.movers[8].vars[0]=1;remoteFrame.movers[8].revision=4;
 SyncColliderGroups(false);assert(!localFrame.movers[8].requestPending&&localFrame.movers[8].requestCount==1&&M<int>(0x52f7e0)==1);
 cachedLevel=4;
 {FILE* fixture=fopen("controls-src/coop/native-frame4.bin","wb");assert(fixture);CoopFrame g={};g.level=4;g.standing=7;g.movers[3]=HostMover(11,1,3,-777);g.movers[3].vars[2]=0x123;
  assert(CoopValidFrame(g));assert(fwrite(&g,1,sizeof(g),fixture)==sizeof(g));fclose(fixture);}
 colliderPlace=(ColliderPlaceFn)0x488510;colliderClock=ColliderTickCount;
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetColliderGroups();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: moving collider groups (levels 4,5,13): standing player owns, host wins ties, sticky owner, handoff adopts the last state, pointer/stale guards, bridge surface alias, bounds.");
}
static int anchorMeshX,puzzleDraws;static bool anchorPuzzle,anchorCan;
static void __cdecl MockMeshPosition(int id,int* p){assert(id>0&&id<128);if(id!=(anchorCan?0x24:anchorPuzzle?0x23:0xa))return;p[0]=anchorMeshX;p[1]=0;p[2]=0;}   // other anchors: not placed
static void __cdecl MockPuzzleLamps(){++puzzleDraws;}
static void TestMechanismProximityAndSwitches(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;colliderPlace=MockColliderPlace;meshPosition=MockMeshPosition;colliderClock=FastColliderClock;
 // Level 7 target gate (group 1, collider 0xf): the nearest player within the radius owns it, with hysteresis.
 cachedLevel=7;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=7;localFrame.layout=remoteFrame.layout=123;ResetColliderGroups();
 SetCollider(0xf,1,0);M<int>(0x559e80)=-1;remoteFrame.standing=-1;localOwner=1;
 M<int>(0x52f300)=0x1000;M<int>(0x52f304)=0;M<int>(0x52f308)=0;remoteFrame.pose.x=0x100000;   // guest near, host far
 SyncColliderGroups(false);assert(localFrame.movers[1].owner==1);
 remoteFrame.pose.x=0x1200;SyncColliderGroups(false);assert(localFrame.movers[1].owner==1);   // similar distance: keep
 remoteFrame.pose.x=0x100;M<int>(0x52f300)=0x8000;SyncColliderGroups(false);assert(localFrame.movers[1].owner==0); // host clearly nearer
 M<int>(0x52f300)=0x100000;remoteFrame.pose.x=0x100000;SyncColliderGroups(false);assert(localFrame.movers[1].owner==0); // nobody near: sticky
 // Level 8 button lift (mesh anchor 0xa).
 cachedLevel=8;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=8;localFrame.layout=remoteFrame.layout=123;ResetColliderGroups();remoteFrame.standing=-1;
 anchorMeshX=0x20000;M<int>(0x52f300)=0x20100;remoteFrame.pose.x=0x200000;M<int>(0x52fb88)=2;
 SyncColliderGroups(false);SyncColliderGroups(true);assert(localFrame.movers[0].owner==1&&localFrame.movers[0].vars[0]==2&&CoopValidFrame(localFrame));
 // Level 7 NPC door, level 10 targets and level 11 bridges merge one-way with one-shot replay.
 scriptColliderRotate=MockColliderRotate;mechRotate=MockMechRotate;memset(colliderRotations,0,sizeof(colliderRotations));colliderRotateCalls=0;
 cachedLevel=10;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=10;localFrame.layout=remoteFrame.layout=123;ResetScript();localOwner=1;
 M<int>(0x52fc14)=1;remoteFrame.script={};remoteFrame.script.revision=1;remoteFrame.script.owner=2;remoteFrame.script.value[1]=3;
 BeginScript();assert(M<int>(0x52fc14)==3&&colliderRotateCalls==1&&colliderRotations[5][2]==0x180&&mechRotations[0x10][2]==0x180);
 BeginScript();assert(colliderRotateCalls==1);
 cachedLevel=11;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=11;localFrame.layout=remoteFrame.layout=123;ResetScript();
 M<int>(0x52fd54)=0;M<int>(0x52fd38)=0;remoteFrame.script={};remoteFrame.script.revision=1;remoteFrame.script.owner=2;remoteFrame.script.value[1]=0x200;remoteFrame.script.value[2]=1;
 BeginScript();assert(M<int>(0x52fd54)==0x200&&M<int>(0x52fd38)==1&&colliderRotateCalls==2&&colliderRotations[0x17][1]==0x3ae);  // value 1 runs natively
 remoteFrame.script.value[1]=0x100;remoteFrame.script.revision=2;BeginScript();assert(M<int>(0x52fd54)==0x200&&colliderRotateCalls==2);
 cachedLevel=7;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=7;localFrame.layout=remoteFrame.layout=123;ResetScript();
 M<int>(0x52fa10)=0;remoteFrame.script={};remoteFrame.script.revision=1;remoteFrame.script.owner=2;remoteFrame.script.value[1]=0x4000;
 BeginScript();assert(M<int>(0x52fa10)==0x4000);EndScript();assert(localFrame.script.value[1]==0x4000&&CoopValidFrame(localFrame));
 CoopFrame f={};f.level=7;f.script.revision=1;f.script.owner=2;f.script.value[1]=0x4001;assert(!CoopValidFrame(f));
 // Level 13 platform records: the collider index is read from the record at run time; standing on it owns it.
 cachedLevel=13;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=13;localFrame.layout=remoteFrame.layout=123;ResetColliderGroups();localOwner=1;
 M<int>(0x52fe98)=0xb;SetCollider(0xb,2,0x900);M<int>(0x52fe88)=3;M<int>(0x559e80)=0xb;M<int>(0x52f300)=0x100000;remoteFrame.pose.x=0x100000;remoteFrame.standing=-1;
 SyncColliderGroups(false);SyncColliderGroups(true);
 assert(localFrame.movers[4].owner==1&&localFrame.movers[4].vars[0]==3&&localFrame.movers[4].colliders[0].pos[0]==0x900&&localFrame.movers[3].owner==0&&CoopValidFrame(localFrame));
 M<int>(0x52fe98)=99;SyncColliderGroups(true);assert(localFrame.movers[4].colliders[0].present==0);   // invalid run-time collider ignored
 // Level 5 button bit: collider 4 raised by 0xe10 and meshes 0x10/0x11 scaled once.
 mechScale=MockMechScale;memset(mechScales,0,sizeof(mechScales));colliderPlaceCalls=0;
 cachedLevel=5;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=5;localFrame.layout=remoteFrame.layout=123;ResetScript();
 SetCollider(4,1,0x2000);M<int>(0x52f938)=2;remoteFrame.script={};remoteFrame.script.revision=1;remoteFrame.script.owner=2;remoteFrame.script.value[1]=1;
 BeginScript();assert(M<int>(0x52f938)==3&&M<int>(0x7295a8+0x34)==0x2000&&M<int>(0x7295a8+0x34+4)==0x2001+0xe10&&mechScales[0x10][1]==0x960&&mechScales[0x11][1]==0x960&&colliderPlaceCalls==1);
 BeginScript();assert(colliderPlaceCalls==1);EndScript();assert(localFrame.script.value[1]==1&&CoopValidFrame(localFrame));
 // Level 14 lamp puzzle: one shared puzzle; observer press -> CAS request, owner spends the move once.
 cachedLevel=14;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=14;localFrame.layout=remoteFrame.layout=123;ResetColliderGroups();
 puzzleLamps=MockPuzzleLamps;puzzleLampsAlt=MockPuzzleLamps;puzzleDraws=0;memset(mechScales,0,sizeof(mechScales));
 localOwner=1;M<int>(0x559e80)=-1;remoteFrame.standing=-1;anchorPuzzle=true;anchorMeshX=0;M<int>(0x52f300)=0x100000;remoteFrame.pose.x=0x100;
 M<int>(0x52ffd0)=0x77;M<int>(0x52ffb4)=0;M<int>(0x52ff70)=0;
 CoopMover host={};host.revision=4;host.owner=0;host.group=0;host.vars[0]=0x5a;host.vars[1]=3;host.vars[2]=1;remoteFrame.movers[0]=host;
 SyncColliderGroups(false);
 assert(localFrame.movers[0].owner==0&&M<int>(0x52ffd0)==0x5a&&M<int>(0x52ffb4)==1&&M<int>(0x52ff70)==3&&puzzleDraws==2&&mechScales[0x39][0]==0x1000&&mechScales[0x36][0]==0);
 SyncColliderGroups(false);assert(puzzleDraws==2);                    // unchanged values: no redraw
 M<int>(0x52ffd0)=0x53;M<int>(0x52ff70)=2;SyncColliderGroups(true);    // local press toggled lamps 1|2 (42d710)
 assert(localFrame.movers[0].requestCount==1&&localFrame.movers[0].requestFrom==0x5a&&localFrame.movers[0].requestTo==0x53&&M<int>(0x52ffd0)==0x5a&&M<int>(0x52ff70)==3);
 CoopMover press=localFrame.movers[0];
 ResetColliderGroups();localOwner=0;localFrame.movers[0]=press;remoteFrame.movers[0]=press;M<int>(0x52f300)=0x100;remoteFrame.pose.x=0x100000;puzzleDraws=0;
 M<int>(0x52ffd0)=0x5a;M<int>(0x52ff70)=3;SyncColliderGroups(false);
 assert(M<int>(0x52ffd0)==0x53&&M<int>(0x52ff70)==2&&puzzleDraws==2&&mechScales[0x38][0]==0x1000);
 SyncColliderGroups(false);assert(M<int>(0x52ff70)==2);               // applied once
 // A solved puzzle blinking 0xff/0 is not a press: no move is spent.
 ResetColliderGroups();CoopMover blink=press;blink.requestCount=9;blink.requestFrom=0xff;blink.requestTo=0;localFrame.movers[0]=blink;remoteFrame.movers[0]=blink;M<int>(0x52ffd0)=0xff;M<int>(0x52ff70)=1;
 SyncColliderGroups(false);assert(M<int>(0x52ffd0)==0&&M<int>(0x52ff70)==1&&CoopValidFrame(localFrame));
 anchorPuzzle=false;puzzleLamps=(PuzzleDrawFn)0x42d8b0;puzzleLampsAlt=(PuzzleDrawFn)0x42d9c0;
 colliderPlace=(ColliderPlaceFn)0x488510;meshPosition=(MeshPositionFn)0x4ccef0;
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();ResetColliderGroups();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: proximity-owned mechanism groups with hysteresis (level 7 gate, level 8 button lift), level 7 NPC door, level 10 targets, level 11 bridges and level 5 button merge once with one-shot replay; level 13 run-time collider records; one shared level 14 lamp puzzle.");
}
static void TestSyncDiagnostics(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();cachedLevel=4;localOwner=1;localFrame={};remoteFrame={};
 localFrame.level=remoteFrame.level=4;localFrame.layout=remoteFrame.layout=123;ResetColliderGroups();ResetScript();syncStats=SyncStats();
 localFrame.movers[0].revision=1;localFrame.movers[0].owner=1;localFrame.movers[1].revision=1;localFrame.movers[1].owner=0;
 char line[64];assert(CoopStatus(line,sizeof(line))==1&&!strcmp(line,"L4 guest linked s- g1/2 x0")&&strlen(line)<=38);
 assert(strstr(syncLastLine,"summary linked 1")&&strstr(syncLastLine,"groups GH"));
 // A rejected frame is counted once per sequence and names the failing section.
 CoopFrame bad={};bad.sequence=7;bad.level=4;bad.movers[20].revision=1;bad.movers[20].group=20;
 SyncObserveFrame(bad,CoopValidFrame(bad));SyncObserveFrame(bad,CoopValidFrame(bad));
 assert(syncStats.framesRejected==1&&strstr(syncLastLine,"mechanism groups"));
 bad={};bad.sequence=8;bad.level=6;bad.script.revision=1;bad.script.value[0]=9;SyncObserveFrame(bad,CoopValidFrame(bad));assert(syncStats.framesRejected==2);
 CoopFrame good={};good.sequence=9;SyncObserveFrame(good,CoopValidFrame(good));assert(syncStats.framesIn==1);
 assert(!strcmp(FrameRejectReason(bad),"script values/actors"));
 shared=NULL;assert(CoopStatus(line,sizeof(line))==1&&!strcmp(line,"co-op not running"));
 helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetColliderGroups();syncStats=SyncStats();
 puts("PASS: sync diagnostics: console status line, summary, rejected-frame reasons counted once per sequence.");
}
// One player died and the level reloaded on that PC only: its native level state starts from
// scratch while the other PC kept its progress. Shared progress must come back without rewinding
// the other player, one-shot effects must replay once, and moving groups adopt the owner's state.
static void TestLevelReloadResync(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;colliderPlace=MockColliderPlace;
 scriptColliderRotate=MockColliderRotate;mechRotate=MockMechRotate;memset(colliderRotations,0,sizeof(colliderRotations));colliderRotateCalls=0;
 cachedLevel=10;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=10;localFrame.layout=remoteFrame.layout=123;
 ResetScript();ResetColliderGroups();localOwner=1;M<int>(0x559e80)=-1;remoteFrame.standing=-1;
 // Fresh native state after the reload.
 M<int>(0x52fc14)=0;M<int>(0x52fc4c)=0;M<int>(0x52fcf4)=0x4f2000;M<int>(0x52fccc)=0;M<int>(0x52fce8)=0;SetCollider(1,1,0);
 // The other PC: both targets down, friend reward finished, mover 0 (collider 1) far along its path.
 remoteFrame.script={};remoteFrame.script.revision=40;remoteFrame.script.owner=2;remoteFrame.script.value[0]=0xc8;remoteFrame.script.value[1]=3;
 CoopMover mover={};mover.revision=900;mover.owner=0;mover.group=0;mover.vars[0]=0x4f2040;mover.vars[1]=5;mover.vars[2]=2;
 mover.colliders[0].present=1;mover.colliders[0].pos[0]=0x7000;mover.colliders[0].flags=1;remoteFrame.movers[0]=mover;
 BeginScript();SyncColliderGroups(false);
 assert(M<int>(0x52fc14)==3&&M<int>(0x52fc4c)==0xc8&&colliderRotateCalls==2);
 assert(M<int>(0x52fcf4)==0x4f2040&&M<int>(0x52fccc)==5&&M<int>(0x7295a8+0x34)==0x7000&&M<short>(0x7295a8+0x34+0x2c)==8);
 SyncColliderGroups(true);EndScript();
 // The reloaded PC never pushes the other PC backwards.
 assert(localFrame.script.value[1]==3&&localFrame.script.value[0]==0xc8&&localFrame.movers[0].revision==900&&localFrame.movers[0].owner==0);
 BeginScript();assert(colliderRotateCalls==2);   // replayed once only
 colliderPlace=(ColliderPlaceFn)0x488510;colliderClock=ColliderTickCount;
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();ResetColliderGroups();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: level reload on one PC: shared progress and moving groups resynchronize, one-shot effects replay once, no rewind of the other PC.");
}
static DWORD manualColliderNow;static DWORD ManualColliderClock(){return manualColliderNow;}
static void TestColliderSmoothing(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;colliderPlace=MockColliderPlace;colliderClock=ManualColliderClock;meshPosition=MockAnyMeshPosition;
 cachedLevel=4;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=4;localFrame.layout=remoteFrame.layout=123;ResetColliderGroups();
 SetCollider(0xe,1,0);localOwner=1;M<int>(0x559e80)=-1;remoteFrame.standing=-1;M<int>(0x52f300)=0x100000;remoteFrame.pose.x=0x100000;
 uintptr_t geometry=0x7295a8+0x34,c=0x728720+0xe*0x34;
 CoopMover m={};m.revision=10;m.owner=0;m.group=4;m.colliders[0].present=1;m.colliders[0].pos[0]=0x1000;m.colliders[0].rot[1]=100;m.colliders[0].vel[0]=50;remoteFrame.movers[4]=m;
 manualColliderNow=1000;SyncColliderGroups(false);assert(M<int>(geometry)==0x1000&&M<short>(c+0xe)==100&&M<short>(c+0x14)==0);   // first sample snaps
 m.revision=11;m.colliders[0].pos[0]=0x1800;m.colliders[0].rot[1]=200;remoteFrame.movers[4]=m;
 manualColliderNow=1100;SyncColliderGroups(false);assert(M<int>(geometry)==0x1000);                      // new target, interval 100 ms
 manualColliderNow=1150;SyncColliderGroups(true);assert(M<int>(geometry)==0x1400&&M<short>(c+0xe)==150); // halfway
 manualColliderNow=1250;SyncColliderGroups(false);assert(M<int>(geometry)==0x1800&&M<short>(c+0xe)==200);
 m.revision=12;m.colliders[0].pos[0]=0x900000;remoteFrame.movers[4]=m;manualColliderNow=1300;SyncColliderGroups(false);assert(M<int>(geometry)==0x900000); // teleport snaps
 // Level 4 crumbling platform (group 9): timer, collider enable and the mesh position the native script
 // only updates near the local player all come from the owner.
 mechPosition=MockMechPosition;memset(mechPositions,0,sizeof(mechPositions));SetCollider(0x24,2,0x5000);
 CoopMover crumble={};crumble.revision=3;crumble.owner=0;crumble.group=9;crumble.vars[0]=0x60;crumble.vars[1]=0x400;crumble.vars[2]=0x20;
 crumble.vars[3]=0x20000;crumble.vars[4]=-0x40000;crumble.vars[5]=0x60000;crumble.colliders[0].present=1;crumble.colliders[0].pos[0]=0x5000;crumble.colliders[0].flags=0;remoteFrame.movers[9]=crumble;
 SyncColliderGroups(false);
 assert(M<int>(0x52f7a4)==0x60&&M<int>(0x52f7b8)==0x400&&M<int>(0x52f7ac)==0x20&&mechPositions[0xa][0]==0x1000&&mechPositions[0xa][1]==-0x2000&&mechPositions[0xa][2]==0x3000&&M<short>(0x7295a8+2*0x34+0x2c)==0);
 // Owner side reads the mesh position into the group values.
 ResetColliderGroups();localOwner=0;remoteFrame.movers[9]={};meshPositions[0xa][0]=0x111;meshPositions[0xa][1]=0x222;meshPositions[0xa][2]=0x333;
 SyncColliderGroups(false);SyncColliderGroups(true);assert(localFrame.movers[9].vars[3]==0x111&&localFrame.movers[9].vars[5]==0x333&&CoopValidFrame(localFrame));
 colliderClock=ColliderTickCount;colliderPlace=(ColliderPlaceFn)0x488510;
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetColliderGroups();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: level 4 crumbling platforms share timer, collider and mesh position; observer collider smoothing: first snap, interpolation over the packet interval, rotation, no local velocity, teleport snap.");
}
static int paintColourCalls,paintButtonCalls;
static void __cdecl MockPaintColour(int v){++paintColourCalls;M<int>(0x52fd14)=(M<int>(0x52fd14)&0x800)|(v&~0x800);}
static void __cdecl MockPaintButtons(int v){++paintButtonCalls;M<int>(0x52fd3c)=v;}
static void TestPaintCan(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;colliderPlace=MockColliderPlace;meshPosition=MockMeshPosition;colliderClock=FastColliderClock;
 paintColour=MockPaintColour;paintLevelButtons=MockPaintButtons;paintColourCalls=paintButtonCalls=0;anchorPuzzle=false;
 cachedLevel=11;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=11;localFrame.layout=remoteFrame.layout=123;ResetColliderGroups();ResetScript();
 // Guest observer, host stands at the can: colour and level buttons arrive through the native transitions.
 localOwner=1;M<int>(0x559e80)=-1;remoteFrame.standing=-1;anchorMeshX=0;anchorCan=true;M<int>(0x52f300)=0x100000;remoteFrame.pose.x=0x100;
 M<int>(0x52fd14)=0x800;M<int>(0x52fd3c)=0;M<int>(0x52fd30)=0;M<int>(0x52fdfc)=0;
 CoopMover host={};host.revision=5;host.owner=0;host.group=0;host.vars[0]=3;host.vars[1]=0x20;host.vars[2]=-0x4000;host.vars[3]=-0x1000;remoteFrame.movers[0]=host;
 SyncColliderGroups(false);
 assert(paintColourCalls==1&&M<int>(0x52fd14)==0x803&&paintButtonCalls==1&&M<int>(0x52fd3c)==0x20&&M<int>(0x52fd30)==-0x4000&&M<int>(0x52fdfc)==-0x1000);
 M<int>(0x52fd14)=3;SyncColliderGroups(false);assert(paintColourCalls==1);   // local blink bit is not shared and not fought
 // The guest mixes a colour locally: one compare-and-swap request, value rewritten to the owner's.
 M<int>(0x52fd14)=0x807;SyncColliderGroups(true);
 assert(localFrame.movers[0].requestCount==1&&localFrame.movers[0].requestFrom==3&&localFrame.movers[0].requestTo==7&&paintColourCalls==2&&(M<int>(0x52fd14)&~0x800)==3);
 CoopMover press=localFrame.movers[0];
 ResetColliderGroups();localOwner=0;localFrame.movers[0]=press;remoteFrame.movers[0]=press;M<int>(0x52f300)=0x100;remoteFrame.pose.x=0x100000;M<int>(0x52fd14)=0x803;paintColourCalls=0;
 SyncColliderGroups(false);assert(paintColourCalls==1&&(M<int>(0x52fd14)&~0x800)==7);
 SyncColliderGroups(false);assert(paintColourCalls==1);   // applied once
 // A guest level-button press: the owner moves the buttons and derives the target fill.
 CoopMover level=press;level.requestCount=2;level.requestVar=1;level.requestFrom=0;level.requestTo=0x40;remoteFrame.movers[0]=level;
 M<int>(0x52fd3c)=0;M<int>(0x52fd30)=0;SyncColliderGroups(false);assert(paintButtonCalls==2&&M<int>(0x52fd3c)==0x40&&M<int>(0x52fd30)==-64000);
 // Pour switches: one-way, the press is replayed once.
 mechScale=MockMechScale;memset(mechScales,0xff,sizeof(mechScales));colliderPlaceCalls=0;SetCollider(0x12,1,0x3000);localOwner=1;
 M<int>(0x52fd60)=0;remoteFrame.script={};remoteFrame.script.revision=1;remoteFrame.script.owner=2;remoteFrame.script.value[5]=1;   // progress 1 = timer 0x3c
 BeginScript();assert(M<int>(0x52fd60)==0x3c&&mechScales[0x39][1]==0x800&&mechScales[0x53][0]==0&&M<int>(0x7295a8+0x34+4)==0x3001+0xe10&&colliderPlaceCalls==1);
 BeginScript();assert(colliderPlaceCalls==1);EndScript();assert(localFrame.script.value[5]==1&&CoopValidFrame(localFrame));
 CoopFrame f={};f.level=11;f.script.revision=1;f.script.owner=2;f.script.value[8]=0x3e;assert(!CoopValidFrame(f));f.script.value[8]=0x3d;assert(CoopValidFrame(f));
 anchorCan=false;paintColour=(PaintStateFn)0x428890;paintLevelButtons=(PaintStateFn)0x4292c0;colliderPlace=(ColliderPlaceFn)0x488510;meshPosition=(MeshPositionFn)0x4ccef0;colliderClock=ColliderTickCount;
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();ResetColliderGroups();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: level 11 paint can: colour and level buttons reach observers through the native transitions, local blink kept, observer mixing applied once by the owner, pour switches replayed once.");
}
// Campaign audit 2026.11.7.13: mechanisms found in level helpers/tick branches that still ran per PC.
static int pourCalls,pourArg;static void __cdecl MockPour(int offset){++pourCalls;pourArg=offset;}
static void SetGroupLevel(unsigned level){cachedLevel=level;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=level;localFrame.layout=remoteFrame.layout=123;ResetColliderGroups();ResetScript();remoteFrame.standing=-1;M<int>(0x559e80)=-1;}
static void TestCampaignAudit(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;colliderPlace=MockColliderPlace;meshPosition=MockAnyMeshPosition;colliderClock=FastColliderClock;
 mechScale=MockMechScale;mechDisable=MockMechDisable;mechEnable=MockMechEnable;scriptColliderRotate=MockColliderRotate;mechRotate=MockMechRotate;level11Pour=MockPour;
 memset(meshPositions,0,sizeof(meshPositions));memset(mechToggles,0,sizeof(mechToggles));memset(mechScales,0xff,sizeof(mechScales));
 // Level 11 pour switches: countdown progress. Press replayed once, completion (pour + turret bit) replayed once,
 // a peer still counting never pulls a finished switch back, and a finished switch is never re-armed.
 SetGroupLevel(11);localOwner=1;SetCollider(0x11,1,0x3000);pourCalls=0;colliderPlaceCalls=0;
 M<int>(0x52fd5c)=0;M<int>(0x52fd48)=0;remoteFrame.script.revision=1;remoteFrame.script.owner=2;remoteFrame.script.value[4]=1;
 BeginScript();assert(M<int>(0x52fd5c)==0x3c&&colliderPlaceCalls==1&&mechScales[0x38][1]==0x800&&pourCalls==0);
 M<int>(0x52fd5c)=5;remoteFrame.script.value[4]=0x10;remoteFrame.script.revision=2;BeginScript();assert(M<int>(0x52fd5c)==5);   // we are ahead
 remoteFrame.script.value[4]=0x3d;remoteFrame.script.revision=3;BeginScript();
 assert(M<int>(0x52fd5c)==-1&&pourCalls==1&&pourArg==4&&(M<int>(0x52fd48)&2)&&colliderPlaceCalls==1);
 remoteFrame.script.value[4]=0x20;remoteFrame.script.revision=4;BeginScript();assert(M<int>(0x52fd5c)==-1&&pourCalls==1);   // never re-armed
 EndScript();assert(localFrame.script.value[4]==0x3d&&CoopValidFrame(localFrame));
 // Finished directly from idle (late observer): press and pour both replayed.
 SetCollider(0xa,2,0x5000);M<int>(0x52fd6c)=0;remoteFrame.script.value[8]=0x3d;remoteFrame.script.revision=5;colliderPlaceCalls=0;
 BeginScript();assert(M<int>(0x52fd6c)==-1&&colliderPlaceCalls==1&&pourCalls==2&&pourArg==20&&(M<int>(0x52fd48)&0x20));
 assert(CountdownProgress(0)==0&&CountdownProgress(0x3c)==1&&CountdownProgress(1)==0x3c&&CountdownProgress(-1)==0x3d&&CountdownValue(0x3c)==1&&CountdownValue(0x3d)==-1);
 // Level 11 paint-route toy and collider 0x1b: owner's route/position/enable reach the observer; the toy's
 // proximity bit 0x400 and the button blink 0x200 are local and never become requests.
 SetGroupLevel(11);localOwner=1;SetCollider(0x1b,3,0x7000);M<short>(0x7295a8+3*0x34+0x2c)=0;
 CoopMover toy={};toy.revision=3;toy.owner=0;toy.group=1;toy.vars[0]=4;toy.vars[1]=2;toy.vars[2]=1;toy.vars[3]=0x1111;toy.vars[4]=-0x2222;toy.vars[5]=0x3333;toy.vars[6]=0x400;toy.vars[7]=0x80;
 toy.colliders[0].present=1;toy.colliders[0].pos[0]=0x7000;toy.colliders[0].flags=1;remoteFrame.movers[1]=toy;
 SyncColliderGroups(false);
 assert(localFrame.movers[1].owner==0&&M<int>(0x52fd4c)==4&&M<int>(0x52fd84)==2&&M<int>(0x52fdbc)==1&&M<int>(0x52fd98)==0x1111&&M<int>(0x52fd9c)==-0x2222&&M<int>(0x52fd0c)==0x400&&M<int>(0x52fdf0)==0x80);
 assert(M<short>(0x7295a8+3*0x34+0x2c)==8);   // collider 0x1b enabled as on the owner (route end)
 CoopMover can={};can.revision=2;can.owner=0;can.group=0;can.vars[0]=3;can.vars[1]=0x20;remoteFrame.movers[0]=can;
 paintColour=MockPaintColour;paintLevelButtons=MockPaintButtons;M<int>(0x52fd14)=3;M<int>(0x52fd3c)=0x20;SyncColliderGroups(false);
 M<int>(0x52fd14)=0x403;M<int>(0x52fd3c)=0x220;SyncColliderGroups(true);assert(localFrame.movers[0].requestCount==0);
 // Floating platforms: whoever stands on collider 0xe owns its bobbing state.
 SetCollider(0xe,4,0x100);M<int>(0x559e80)=0xe;SyncColliderGroups(false);M<int>(0x52fd7c)=-0x300;M<int>(0x52fd1c)=0x200;SyncColliderGroups(true);   // native tick moved it
 assert(localFrame.movers[3].owner==1&&localFrame.movers[3].vars[0]==-0x300&&localFrame.movers[3].vars[1]==0x200&&localFrame.movers[3].colliders[0].pos[0]==0x100&&CoopValidFrame(localFrame));
 paintColour=(PaintStateFn)0x428890;paintLevelButtons=(PaintStateFn)0x4292c0;
 // Level 10 floor switches: packed counters; an observer landing (two counters change) is one request,
 // applied once by the owner; the solved bit 4 is replayed once (collider 0x13 off, lamps).
 SetGroupLevel(10);localOwner=1;meshPositions[0x12][0]=0;M<int>(0x52f300)=0x100000;remoteFrame.pose.x=0x100;
 M<int>(0x52fc50)=0;M<int>(0x52fc30)=2;M<int>(0x52fcd8)=0;
 CoopMover sw={};sw.revision=2;sw.owner=0;sw.group=2;sw.vars[0]=1|1<<2|0<<4;remoteFrame.movers[2]=sw;
 SyncColliderGroups(false);assert(M<int>(0x52fc50)==1&&M<int>(0x52fc30)==1&&M<int>(0x52fcd8)==0&&Level10SwitchRead()==5);
 M<int>(0x52fc50)=0;M<int>(0x52fcd8)=1;SyncColliderGroups(true);   // native: surface 0x22 landing
 assert(localFrame.movers[2].requestCount==1&&localFrame.movers[2].requestFrom==5&&localFrame.movers[2].requestTo==0x14&&M<int>(0x52fc50)==1&&M<int>(0x52fcd8)==0);
 CoopMover landing=localFrame.movers[2];
 SetGroupLevel(10);localOwner=0;localFrame.movers[2]=landing;remoteFrame.movers[2]=landing;M<int>(0x52f300)=0x100;remoteFrame.pose.x=0x100000;
 M<int>(0x52fc50)=1;M<int>(0x52fc30)=1;M<int>(0x52fcd8)=0;SyncColliderGroups(false);
 assert(M<int>(0x52fc50)==0&&M<int>(0x52fc30)==1&&M<int>(0x52fcd8)==1);
 M<int>(0x52fc50)=2;SyncColliderGroups(false);assert(M<int>(0x52fc50)==2);   // applied once
 SetGroupLevel(10);localOwner=1;M<int>(0x52fc14)=3;memset(mechToggles,0,sizeof(mechToggles));
 remoteFrame.script.revision=1;remoteFrame.script.owner=2;remoteFrame.script.value[1]=7;
 BeginScript();assert(M<int>(0x52fc14)==7&&mechToggles[0x13]==-1&&mechScales[0x46][0]==0x1000&&mechScales[0x47][0]==0&&mechScales[0x4e][0]==0x1000&&mechScales[0x4f][0]==0);
 mechToggles[0x13]=0;BeginScript();assert(mechToggles[0x13]==0);EndScript();assert(localFrame.script.value[1]==7&&CoopValidFrame(localFrame));
 M<int>(0x52fc14)=0xf;EndScript();assert(localFrame.script.value[1]==7);   // local blink bit 8 not sent
 // Level 8 crane prize: the owner's grab result and prize position reach the observer (same owner as the lift).
 SetGroupLevel(8);localOwner=1;meshPositions[0xa][0]=0x20000;M<int>(0x52f300)=0x200000;remoteFrame.pose.x=0x20100;
 CoopMover lift={};lift.revision=3;lift.owner=0;lift.group=0;lift.vars[0]=6;remoteFrame.movers[0]=lift;
 CoopMover prize={};prize.revision=3;prize.owner=0;prize.group=1;prize.vars[0]=1;prize.vars[1]=0x1000;prize.vars[2]=-0x5000;prize.vars[3]=0x2000;prize.vars[5]=1;remoteFrame.movers[1]=prize;
 M<int>(0x52fb74)=0;M<int>(0x52fb08)=0x9999;SyncColliderGroups(false);
 assert(localFrame.movers[0].owner==0&&localFrame.movers[1].owner==0&&M<int>(0x52fb88)==6&&M<int>(0x52fb74)==1&&M<int>(0x52fb08)==0x1000&&M<int>(0x52fb0c)==-0x5000&&M<int>(0x52fb18)==1);
 // Level 5 bubble rides: phase and bubble state of the nearest player; level 7 press velocity follows its owner.
 SetGroupLevel(5);localOwner=1;meshPositions[0xc][0]=0;meshPositions[0x12][0]=0;M<int>(0x52f300)=0x100;remoteFrame.pose.x=0x200000;
 M<int>(0x52f8f8)=0x24;M<int>(0x52f91c)=0x200;M<int>(0x52f914)=-0x3000;M<int>(0x52f92c)=0;
 SyncColliderGroups(false);SyncColliderGroups(true);
 assert(localFrame.movers[6].owner==1&&localFrame.movers[6].vars[0]==0x24&&localFrame.movers[6].vars[1]==0x200&&localFrame.movers[6].vars[3]==-0x3000&&localFrame.movers[7].owner==1&&CoopValidFrame(localFrame));
 CoopMover bubble=localFrame.movers[6];SetGroupLevel(5);localOwner=0;remoteFrame.movers[6]=bubble;M<int>(0x52f300)=0x200000;remoteFrame.pose.x=0x100;M<int>(0x52f8f8)=0x1f0;
 SyncColliderGroups(false);assert(localFrame.movers[6].owner==1&&M<int>(0x52f8f8)==0x24&&M<int>(0x52f91c)==0x200);
 SetGroupLevel(7);localOwner=1;SetCollider(0xa,1,0x500);remoteFrame.standing=0xa;
 CoopMover press={};press.revision=2;press.owner=0;press.group=2;press.vars[0]=0x300;press.vars[1]=-0x40;press.colliders[0].present=1;press.colliders[0].pos[0]=0x600;remoteFrame.movers[2]=press;
 M<int>(0x52f9bc)=0x1000;SyncColliderGroups(false);assert(M<int>(0x52fa4c)==0x300&&M<int>(0x52f9bc)==-0x40);
 // Level 14 carousel: collider 0 and its angle follow the player standing on it.
 SetGroupLevel(14);localOwner=1;SetCollider(0,5,0x800);remoteFrame.standing=0;
 CoopMover ride={};ride.revision=2;ride.owner=0;ride.group=1;ride.vars[0]=0x123;ride.colliders[0].present=1;ride.colliders[0].pos[0]=0x800;ride.colliders[0].rot[1]=0x456;remoteFrame.movers[1]=ride;
 SyncColliderGroups(false);assert(localFrame.movers[1].owner==0&&M<int>(0x52ff84)==0x123&&M<short>(0x728720+0xe)==0x456&&CoopValidFrame(localFrame));
 // Wire tables: new group counts and bounds.
 assert(CoopMoverGroupCounts[5]==8&&CoopMoverGroupCounts[8]==2&&CoopMoverGroupCounts[10]==3&&CoopMoverGroupCounts[11]==6&&CoopMoverGroupCounts[14]==2&&CoopVersion==23);
 CoopFrame f={};f.level=10;f.script.revision=1;f.script.owner=2;f.script.value[1]=7;assert(CoopValidFrame(f));f.script.value[1]=8;assert(!CoopValidFrame(f));
 f={};f.level=11;f.standing=-1;f.movers[5]=HostMover(1,0,5,0);assert(CoopValidFrame(f));f.movers[6]=HostMover(1,0,6,0);assert(!CoopValidFrame(f));
 level11Pour=(PourFn)0x429800;colliderPlace=(ColliderPlaceFn)0x488510;meshPosition=(MeshPositionFn)0x4ccef0;colliderClock=ColliderTickCount;
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();ResetColliderGroups();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: campaign audit adapters: level 11 pour countdown (press/pour once, never re-armed), route toy + collider 0x1b, floating platforms, local blink bits; level 10 floor-switch puzzle (one packed request, solved bit replayed once); level 8 crane prize; level 5 bubbles; level 7 press velocity; level 14 carousel.");
}
// Generic regression over the whole group table, in both directions: a mechanism started only on the host
// and only on the guest reaches the other PC; duplicate packets change nothing; stale packets never roll it
// back; a press on the observer is one compare-and-swap request applied once by the owner.
static int GroupTestValue(const ColliderVarDesc& v,unsigned k){
 if(v.pointer)return 0x4f1000+(int)k*4;
 if(v.read)return 1|2<<2|1<<4;
 int x=(int)(k+1)*0x10;return v.mask?x&v.mask:x;
}
static void GroupTestStore(const ColliderVarDesc& v,int value){if(v.apply)v.apply(value);else M<int>(v.address)=value;}
static void GroupTestPrepare(const ColliderGroupDesc& g,int position){
 for(unsigned k=0;k<8;k++)if(g.vars[k].address)M<int>(g.vars[k].address)=0;
 if(g.colliders[0]==-2)M<int>(g.colliderFrom[0])=0xb;
 for(unsigned k=0;k<2;k++){int c=GroupCollider(g,k);if(c>=0)SetCollider(c,(short)(k+1),position+(int)k*0x2000);}
}
// starter == localOwner: the local player stands on the group (or next to its anchor mesh); otherwise the peer does.
static void GroupTestPlayers(const ColliderGroupDesc& g,bool local){
 int c=GroupCollider(g,0);const int near_=0x2000,far_=0x1000000;
 M<int>(0x52b81c)=0;M<int>(0x559e80)=local?c:-1;remoteFrame.standing=local?-1:c;
 if(g.anchorMesh>=0)meshPositions[g.anchorMesh][0]=near_;
 bool anchored=c<0;
 M<int>(0x52f300)=anchored&&local?near_:far_;remoteFrame.pose.x=anchored&&!local?near_:far_;
}
static void TestGroupMatrix(){
 CoopShared page={};shared=&page;helperTick=GetTickCount();pickupCount=0;colliderPlace=MockColliderPlace;meshPosition=MockAnyMeshPosition;colliderClock=FastColliderClock;
 mechPosition=MockMechPosition;mechScale=MockMechScale;paintColour=MockPaintColour;paintLevelButtons=MockPaintButtons;puzzleLamps=MockPuzzleLamps;puzzleLampsAlt=MockPuzzleLamps;
 unsigned tested=0,triggers=0;
 for(unsigned level=4;level<=14;level++)for(unsigned i=0;i<CoopMoverGroupCounts[level];i++)for(unsigned starter=0;starter<2;starter++){
  // ---- the starter's PC: its native script ran the mechanism ----
  SetGroupLevel(level);localOwner=starter;const ColliderGroupDesc& g=*ColliderGroup(i);
  memset(meshPositions,0,sizeof(meshPositions));memset(mechPositions,0,sizeof(mechPositions));
  GroupTestPrepare(g,0x4000);GroupTestPlayers(g,true);
  SyncColliderGroups(false);
  int values[8]={};
  for(unsigned k=0;k<8;k++){const ColliderVarDesc& v=g.vars[k];
   if(v.mesh>0){values[k]=(int)(k+1)*0x100;meshPositions[v.mesh][v.axis]=values[k];}
   else if(v.address){values[k]=GroupTestValue(v,k);GroupTestStore(v,values[k]);}}
  SyncColliderGroups(true);
  {const CoopMover& m=localFrame.movers[i];assert(m.owner==starter&&m.group==i&&m.revision>=2&&CoopValidFrame(localFrame));
   for(unsigned k=0;k<8;k++)if(g.vars[k].address||g.vars[k].mesh>0)assert(m.vars[k]==values[k]);
   if(GroupCollider(g,0)>=0)assert(m.colliders[0].present==1&&m.colliders[0].pos[0]==0x4000);}
  CoopMover packet=localFrame.movers[i];
  // ---- the other PC, which never ran this mechanism ----
  SetGroupLevel(level);localOwner=1-starter;memset(mechPositions,0,sizeof(mechPositions));
  GroupTestPrepare(g,0);GroupTestPlayers(g,false);remoteFrame.movers[i]=packet;
  paintColourCalls=paintButtonCalls=puzzleDraws=0;
  SyncColliderGroups(false);
  auto arrived=[&](){for(unsigned k=0;k<8;k++){const ColliderVarDesc& v=g.vars[k];
    if(v.mesh>0)assert(mechPositions[v.mesh][v.axis]==values[k]>>5);else if(v.address)assert(GroupVarValue(v)==values[k]);}};
  assert(localFrame.movers[i].owner==starter);arrived();
  if(GroupCollider(g,0)>=0)assert(M<int>(ColliderGeometry(GroupCollider(g,0)))==0x4000);
  // Duplicate packets: no new side effect, no request, no new revision.
  int colour=paintColourCalls,buttons=paintButtonCalls,draws=puzzleDraws;uint32_t revision=localFrame.movers[i].revision;
  for(int repeat=0;repeat<3;repeat++){SyncColliderGroups(false);SyncColliderGroups(true);}
  assert(colour==paintColourCalls&&buttons==paintButtonCalls&&draws==puzzleDraws&&localFrame.movers[i].requestCount==0&&localFrame.movers[i].revision==revision);arrived();
  // A stale packet never rolls the observer back.
  {CoopMover stale=packet;stale.revision=packet.revision-1;for(unsigned k=0;k<8;k++)stale.vars[k]=0x7777;if(GroupCollider(g,0)>=0)stale.colliders[0].pos[0]=0x123;
   remoteFrame.movers[i]=stale;SyncColliderGroups(false);arrived();assert(localFrame.movers[i].revision==revision);remoteFrame.movers[i]=packet;}
  ++tested;
  // ---- a press on the observer ----
  int t=-1;for(unsigned k=0;k<8&&t<0;k++)if(g.vars[k].address&&g.vars[k].trigger&&!g.vars[k].pointer)t=(int)k;
  if(t<0)continue;
  const ColliderVarDesc& v=g.vars[t];int from=values[t],to=v.read?(2|1<<2|1<<4):v.mask?(from^0x40)&v.mask:from^0x40;assert(to!=from);
  GroupTestStore(v,to);SyncColliderGroups(true);
  {const CoopMover& m=localFrame.movers[i];assert(m.requestCount==1&&m.requestVar==(uint8_t)t&&m.requestFrom==from&&m.requestTo==to&&m.requestPending==1);assert(GroupVarValue(v)==from);}
  GroupTestStore(v,to);SyncColliderGroups(true);assert(localFrame.movers[i].requestCount==1&&GroupVarValue(v)==from);   // pending: never repeated
  CoopMover request=localFrame.movers[i];assert(CoopValidFrame(localFrame));
  // ---- the owner applies it exactly once ----
  SetGroupLevel(level);localOwner=starter;GroupTestPrepare(g,0x4000);GroupTestPlayers(g,true);
  for(unsigned k=0;k<8;k++)if(g.vars[k].address)GroupTestStore(g.vars[k],values[k]);
  localFrame.movers[i]=packet;remoteFrame.movers[i]=request;
  SyncColliderGroups(false);assert(GroupVarValue(v)==to);
  GroupTestStore(v,from);SyncColliderGroups(false);assert(GroupVarValue(v)==from);   // duplicate request: not applied again
  // CAS: a request based on a value the owner has moved past is refused.
  SetGroupLevel(level);localOwner=starter;GroupTestPrepare(g,0x4000);GroupTestPlayers(g,true);
  for(unsigned k=0;k<8;k++)if(g.vars[k].address)GroupTestStore(g.vars[k],values[k]);
  {int moved=v.read?(1|1<<2|2<<4):v.mask?(from^0x80)&v.mask:from^0x80;GroupTestStore(v,moved);localFrame.movers[i]=packet;remoteFrame.movers[i]=request;SyncColliderGroups(false);assert(GroupVarValue(v)==moved);}
  ++triggers;
 }
 assert(tested==2*50&&triggers==2*10);
 // A PC that reloaded the level resumes its own last record instead of republishing the fresh native state.
 SetGroupLevel(4);localOwner=1;M<int>(0x52f300)=0x1000000;remoteFrame.pose.x=0x1000000;SetCollider(0x1a,1,0);M<int>(0x52f7e0)=0;
 {CoopMover own={};own.revision=9;own.owner=1;own.group=8;own.vars[0]=1;own.colliders[0].present=1;own.colliders[0].pos[0]=0x4444;own.colliders[0].rot[2]=-0x999;own.colliders[0].flags=1;remoteFrame.movers[8]=own;
  SyncColliderGroups(false);assert(localFrame.movers[8].owner==1&&M<int>(0x52f7e0)==1&&M<int>(0x7295a8+0x34)==0x4444&&M<short>(0x728720+0x1a*0x34+0x10)==-0x999);
  SyncColliderGroups(true);assert(localFrame.movers[8].vars[0]==1&&localFrame.movers[8].colliders[0].rot[2]==-0x999);}
 // A request still pending when its sender becomes the owner must not block later presses.
 SetGroupLevel(4);localOwner=1;M<int>(0x52f300)=0x1000000;remoteFrame.pose.x=0x1000000;SetCollider(0x1a,1,0);M<int>(0x52f7e0)=0;
 {CoopMover host={};host.revision=3;host.owner=0;host.group=8;remoteFrame.movers[8]=host;SyncColliderGroups(false);M<int>(0x52f7e0)=1;SyncColliderGroups(true);assert(localFrame.movers[8].requestPending==1);
  M<int>(0x559e80)=0x1a;SyncColliderGroups(false);assert(localFrame.movers[8].owner==1&&!localFrame.movers[8].requestPending);}
 // One-way script progress on either PC: side effects once, no rewind.
 scriptColliderRotate=MockColliderRotate;mechRotate=MockMechRotate;level11Pour=MockPour;mechDisable=MockMechDisable;
 for(unsigned owner=0;owner<2;owner++){
  auto feed=[&](unsigned index,int value,uint32_t revision){remoteFrame.script={};remoteFrame.script.revision=revision;remoteFrame.script.owner=2;remoteFrame.script.value[index]=value;BeginScript();BeginScript();};
  SetGroupLevel(5);localOwner=owner;SetCollider(4,1,0x2000);M<int>(0x52f938)=0;colliderPlaceCalls=0;
  feed(1,1,1);assert((M<int>(0x52f938)&1)&&colliderPlaceCalls==1);feed(1,0,2);assert((M<int>(0x52f938)&1)&&colliderPlaceCalls==1);
  SetGroupLevel(7);localOwner=owner;M<int>(0x52fa10)=0;feed(1,0x4000,1);assert(M<int>(0x52fa10)==0x4000);feed(1,0,2);assert(M<int>(0x52fa10)==0x4000);
  SetGroupLevel(10);localOwner=owner;M<int>(0x52fc14)=0;colliderRotateCalls=0;memset(mechToggles,0,sizeof(mechToggles));
  feed(1,7,1);assert(M<int>(0x52fc14)==7&&colliderRotateCalls==2&&mechToggles[0x13]==-1);mechToggles[0x13]=0;feed(1,0,2);assert(M<int>(0x52fc14)==7&&colliderRotateCalls==2&&mechToggles[0x13]==0);
  SetGroupLevel(11);localOwner=owner;M<int>(0x52fd54)=0;M<int>(0x52fd38)=0;colliderRotateCalls=0;
  feed(1,0x200,1);assert(M<int>(0x52fd54)==0x200&&colliderRotateCalls==1);feed(1,0x100,2);assert(M<int>(0x52fd54)==0x200&&colliderRotateCalls==1);
  SetCollider(0x12,1,0x3000);M<int>(0x52fd60)=0;M<int>(0x52fd48)=0;colliderPlaceCalls=0;pourCalls=0;
  feed(5,1,3);assert(M<int>(0x52fd60)==0x3c&&colliderPlaceCalls==1&&!pourCalls);feed(5,0x3d,4);assert(M<int>(0x52fd60)==-1&&pourCalls==1&&(M<int>(0x52fd48)&4));feed(5,0x20,5);assert(M<int>(0x52fd60)==-1&&pourCalls==1);
 }
 level11Pour=(PourFn)0x429800;paintColour=(PaintStateFn)0x428890;paintLevelButtons=(PaintStateFn)0x4292c0;puzzleLamps=(PuzzleDrawFn)0x42d8b0;puzzleLampsAlt=(PuzzleDrawFn)0x42d9c0;
 colliderPlace=(ColliderPlaceFn)0x488510;meshPosition=(MeshPositionFn)0x4ccef0;colliderClock=ColliderTickCount;
 shared=NULL;helperTick=0;cachedLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();ResetColliderGroups();memset(testData,0,sizeof(testData));memset(testCollision,0,sizeof(testCollision));
 puts("PASS: mechanism matrix: all 50 groups of levels 4-14 started only on the host and only on the guest; duplicates add no side effect, stale packets never roll back, observer presses are one compare-and-swap request applied once; level reload resumes its own record; script progress on both PCs once and never rewinds.");
}
static int livesDeathModes[3],livesRespawns,livesNativeRespawn;
static void __cdecl MockLivesDeath(uintptr_t actor,int mode){assert(mode==1||mode==2);++livesDeathModes[mode];if(mode==2)M<short>(actor+0x7e)=0;}
static void __cdecl MockLivesRespawn(uintptr_t actor,int arg){assert(arg==0);++livesRespawns;M<short>(actor+0x14)=4;M<short>(actor+0x7e)=5;M<short>(actor+0x7a)=30;}
static void __cdecl MockLivesActors(){if(livesNativeRespawn){uintptr_t p=0x52c840+10*0x9c;M<short>(p+0x7e)=5;M<short>(p+0x7a)=30;livesNativeRespawn=0;}}
static void TestActorLivesAndRejoin(){
 static unsigned char descriptor[0x20]={};
 uintptr_t p=0x52c840+10*0x9c;
 auto setup=[&](unsigned owner){
  memset(testData,0,sizeof(testData));CoopShared page={};(void)page;helperTick=GetTickCount();cachedLevel=7;testLevel=7;pickupTable=0;pickupCount=0;localOwner=owner;
  localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=7;localFrame.layout=remoteFrame.layout=123;
  memset(actorSeen,0,sizeof(actorSeen));ResetEnemySync();ResetScript();ResetColliderGroups();
  for(unsigned i=0;i<64;i++){dropSource[i]=-1;dropLifeTag[i]=0;enemyPrevHealth[i]=0x7fff;hostLifeSeen[i]=0;}hostLifeReady=false;actorCensus=true;
  M<uintptr_t>(p+0x98)=(uintptr_t)descriptor;M<short>(p+0x14)=4;M<short>(p+0x7e)=5;M<short>(p+0x7a)=30;
  memset(livesDeathModes,0,sizeof(livesDeathModes));livesRespawns=0;livesNativeRespawn=0;
 };
 CoopShared page={};shared=&page;
 nativeActors=MockLivesActors;remoteActorDeath=MockLivesDeath;respawnActor=MockLivesRespawn;nativeLevelTick=MockLevelTick;
 // Host: a level-script removal (health > 0 -> 0 inside the level tick) is a removal, and the
 // native respawn after its delay starts a new life that clears the death/removal/drop bits.
 setup(0);Actors();assert(actorSeen[10]&&localFrame.enemyLife[10]==0);
 BeginRemovalWatch();M<short>(p+0x7e)=0;EndRemovalWatch();
 assert(CoopBit(localFrame.deadActors,10)&&CoopBit(localFrame.removedActors,10));
 Actors();assert(localFrame.enemyLife[10]==0);
 CoopSetBit(localFrame.drops,10);livesNativeRespawn=1;Actors();
 assert(localFrame.enemyLife[10]==1&&!CoopBit(localFrame.deadActors,10)&&!CoopBit(localFrame.removedActors,10)&&!CoopBit(localFrame.drops,10));
 Actors();assert(localFrame.enemyLife[10]==1);
 // Guest: the host's removal is replayed as a removal (mode 2: no death animation, no coin),
 // the guest's own native respawn is held, and the host's respawn is replayed exactly once.
 setup(1);Actors();
 CoopSetBit(remoteFrame.deadActors,10);CoopSetBit(remoteFrame.removedActors,10);
 Actors();assert(livesDeathModes[2]==1&&livesDeathModes[1]==0&&CoopBit(localFrame.removedActors,10)&&M<short>(p+0x7e)==0&&M<short>(p+0x14)==4);
 Actors();assert(M<short>(p+0x7a)==0x1388&&livesDeathModes[2]==1);
 memset(remoteFrame.deadActors,0,sizeof(remoteFrame.deadActors));memset(remoteFrame.removedActors,0,sizeof(remoteFrame.removedActors));remoteFrame.enemyLife[10]=1;
 Actors();assert(livesRespawns==1&&localFrame.enemyLife[10]==1&&M<short>(p+0x7e)==5&&!CoopBit(localFrame.deadActors,10));
 Actors();assert(livesRespawns==1);
 // A death bit of an older life is ignored; a coin from an older life is not deduplicated.
 CoopSetBit(remoteFrame.deadActors,10);remoteFrame.enemyLife[10]=1;localFrame.enemyLife[10]=1;
 dropSource[0]=10;dropLifeTag[0]=0;assert(CollectDrop(0x529e58)==1&&!CoopBit(localFrame.drops,10));
 dropLifeTag[0]=1;assert(CollectDrop(0x529e58)==1&&CoopBit(localFrame.drops,10));
 // The host reloaded the level (life 0 everywhere): the guest follows the new numbering.
 memset(remoteFrame.deadActors,0,sizeof(remoteFrame.deadActors));remoteFrame.enemyLife[10]=0;Actors();assert(localFrame.enemyLife[10]==0);
 // A generic death replay (mode 1) still plays once and stays permanent.
 CoopSetBit(remoteFrame.deadActors,10);Actors();assert(livesDeathModes[1]==1&&M<short>(p+0x7a)==0);
 // ---- Encounter owner after a level reload on one PC (level 6 boss) ----
 cachedLevel=6;localFrame.level=remoteFrame.level=6;const ScriptLevelDesc* d=ScriptLevel();M<int>(0x52f9a4)=2;
 ResetScript();localOwner=0;scriptOwner=1;localFrame.scriptActors[0].revision=5;localFrame.scriptActors[0].owner=1;remoteFrame.scriptActors[0]={};
 ResolveScriptOwner(d);assert(scriptOwner==0);   // the stayer continues: its live boss is never overwritten by the stale snapshot
 ResetScript();localOwner=1;M<int>(0x52f9a4)=0;remoteFrame.scriptActors[0]={};remoteFrame.scriptActors[0].revision=9;remoteFrame.scriptActors[0].owner=1;
 ResolveScriptOwner(d);assert(scriptOwner==2);   // never resume from our own echoed record
 remoteFrame.scriptActors[0].revision=10;remoteFrame.scriptActors[0].owner=0;ResolveScriptOwner(d);assert(scriptOwner==0);
 remoteFrame.script={};remoteFrame.script.revision=3;remoteFrame.script.owner=0;remoteFrame.script.value[0]=2;MergeScriptVars(d);assert(M<int>(0x52f9a4)==2); // follows the fight phase
 // Level 6 boss chases the nearest living player on its owner; observers keep the native local target.
 scriptOwner=0;localOwner=0;M<int>(0x52c840)=1000;M<int>(0x52f300)=900000;M<short>(0x52f396)=14;remoteFrame.pose.x=2000;remoteFrame.pose.health=14;
 bossChaseChoice=0;UpdateBossChaseTarget();assert(bossChaseChoice==1&&bossChaseTarget[0]==2000);
 remoteFrame.pose.health=0;UpdateBossChaseTarget();assert(bossChaseChoice==0&&bossChaseTarget[0]==900000);
 remoteFrame.pose.health=14;scriptOwner=1;UpdateBossChaseTarget();assert(bossChaseTarget[0]==900000);
 // Tin Robot (level 1): the same rule in BossSync.
 cachedLevel=1;localFrame.level=remoteFrame.level=1;ResetScript();ResetBossPresentation();
 localOwner=0;localFrame.boss={};localFrame.boss.revision=3;localFrame.boss.owner=1;remoteFrame.boss={};BeginBossTick();
 assert(localFrame.boss.owner==0&&localFrame.boss.revision==4);
 localOwner=1;localFrame.boss={};M<unsigned>(0x52f578)=0;remoteFrame.boss={};remoteFrame.boss.revision=7;remoteFrame.boss.owner=1;BeginBossTick();
 assert(localFrame.boss.revision==0);
 shared=NULL;helperTick=0;cachedLevel=0;testLevel=0;localOwner=0;localFrame={};remoteFrame={};ResetScript();ResetEnemySync();memset(actorSeen,0,sizeof(actorSeen));
 memset(testData,0,sizeof(testData));nativeActors=MockActors;respawnActor=(RespawnActorFn)0x406cd0;diagnosticPhase="startup";memset(dropLifeTag,0,sizeof(dropLifeTag));for(unsigned i=0;i<64;i++)dropSource[i]=-1;
 puts("PASS: actor lives: script removals replayed as removals, guest respawn held, host respawn replayed once, per-life deaths/coins; encounter and Tin Robot owner after a one-PC level reload.");
}
int main(int argc,char** argv){
 TestActorLivesAndRejoin();
 TestCampaignAudit();
 TestGroupMatrix();
 TestMechanisms();
 TestPaintCan();
 TestColliderSmoothing();
 TestLevelReloadResync();
 TestSyncDiagnostics();
 TestMechanismProximityAndSwitches();
 TestColliderGroups();
 TestScriptBoss9();
 TestScriptBoss15();
 TestScriptBoss3();
 TestScriptBoss6();
 TestScriptBoss12();
 TestScriptLevel4();
 TestRewardStages();
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
 assert(strstr(text,"exception=c0000094")||strstr(text,"exception=C0000094"));assert(strstr(text,"build=2026.11.7.7")&&strstr(text,"phase=startup"));DeleteFileA(diagnosticCrash);diagnosticCrash[0]=0;diagnosticExceptions=0;
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
 { CoopShared page={};shared=&page;helperTick=GetTickCount();revealToken=MockReveal;hideToken=MockHide;
 unsigned char table[4+5*16]={};pickupTable=(uintptr_t)table;pickupCount=5;
 for(unsigned i=0;i<5;i++)M<uintptr_t>(0x830cd4+i*16)=pickupTable+8+i*16;
 for(unsigned level=1;level<=15;level++){
 cachedLevel=level;localFrame={};remoteFrame={};localFrame.level=remoteFrame.level=level;localFrame.layout=remoteFrame.layout=123;remoteFrame.pickupCount=5;
 for(unsigned i=0;i<5;i++)M<int>(0x830cd0+i*16)=0;
 tokenNativeReady=false;tokenReveals=tokenHides=0;
 // The other PC revealed token 1 (event rev 1, shown): replayed once.
 remoteFrame.tokenEvents[1]=1|0x10000|0x20000;SyncTokenSpawns();assert(tokenReveals==(level%3?1:0));SyncTokenSpawns();assert(tokenReveals==(level%3?1:0));
 // Its timed challenge ran out and took the token back (4a0e60): taken back here too, never re-revealed.
 remoteFrame.tokenEvents[1]=2|0x20000;SyncTokenSpawns();SyncTokenSpawns();
 assert(tokenHides==(level%3?1:0)&&tokenReveals==(level%3?1:0)&&M<int>(0x830ce0)==0);
 if(level%3){
  // A local reveal and a local take-back become events with a newer revision.
  M<int>(0x830cf0)=1;SyncTokenSpawns();assert(localFrame.tokenEvents[2]==(1|0x10000)&&(localFrame.spawnedTokens&4));
  M<int>(0x830cf0)=0;SyncTokenSpawns();assert(localFrame.tokenEvents[2]==2&&!(localFrame.spawnedTokens&4));
  // An older remote event never overrides a newer local one.
  remoteFrame.tokenEvents[2]=1|0x10000|0x20000;SyncTokenSpawns();assert(M<int>(0x830cf0)==0&&localFrame.tokenEvents[2]==2);
  // A collected token is final: no reveal or take-back.
  remoteFrame.tokenEvents[1]=5|0x10000|0x20000;remoteFrame.tokens=2;SyncTokenSpawns();assert(tokenReveals==1&&M<int>(0x830ce0)==0);
  assert(CoopValidFrame(localFrame));
 }
 M<unsigned char>(0x52f0d7+level)=0;remoteFrame.tokens=0x80;SyncTokens();assert(localFrame.tokens==0x80);
 }
 localFrame.spawnedTokens=32;assert(!CoopValidFrame(localFrame));localFrame.spawnedTokens=0;
 localFrame.tokenEvents[0]=0x40000;assert(!CoopValidFrame(localFrame));localFrame.tokenEvents[0]=0;
 shared=NULL;helperTick=0;cachedLevel=0;pickupTable=0;pickupCount=0;localFrame={};remoteFrame={};memset(testData,0,sizeof(testData));memset(testSession,0,sizeof(testSession));
 puts("PASS: token reveal and take-back events across every ordinary campaign level (timed challenges 7/10/14), ordering, collected-token finality and boss completion bits."); }

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
 // Personal effects (health, held power-up, life, charges, timers): a replay removes the pickup
 // without awarding the other player; a local touch and shared categories (coins) still award.
 assert(pickupSkipAward==1);                                        // slot 0, category 0 (health), replayed
 RecordPickup(pickupTable+20);assert(pickupSkipAward==0);           // a personal pickup (category 1) touched locally awards
 remoteFrame.collections[3]=5;assert(RemotePickup(pickupTable+52));RecordPickup(pickupTable+52);assert(pickupSkipAward==0); // coin stays a shared award
 for(int c=-1;c<=16;c++)assert(PersonalCategory(c)==(c==0||c==1||c==3||c==6||c==7||c==8||c==10));
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
 frame.mechanisms[0]={5,1,{0x1234,0x1fff,0,0}};frame.mechanisms[1]={3,0,{0x800,0,0,0}};assert(CoopValidFrame(frame));
 FILE* golden=fopen("controls-src/coop/native-frame.bin","wb");assert(golden);
 assert(fwrite(&frame,1,sizeof(frame),golden)==sizeof(frame));fclose(golden);
 // Level-3 script golden frame and the native per-level whitelist for managed mirroring.
 {CoopFrame g={};g.level=3;g.script.revision=7;g.script.owner=1;g.script.value[0]=0x3e7;g.script.value[1]=0x2800;g.script.value[4]=100;
  g.scriptActors[0]=HostBoss(9,-4321,100,3);g.scriptActors[0].owner=1;g.scriptActors[0].commandOffset=0x9e;g.scriptActors[0].motion[9]=77;assert(CoopValidFrame(g));
  FILE* fixture=fopen("controls-src/coop/native-frame3.bin","wb");assert(fixture);assert(fwrite(&g,1,sizeof(g),fixture)==sizeof(g));fclose(fixture);
  fixture=fopen("controls-src/coop/native-rules.bin","wb");assert(fixture);
  int32_t n=(int32_t)CoopScriptBoundCount;fwrite(&n,4,1,fixture);
  for(unsigned i=0;i<CoopScriptBoundCount;i++){int32_t v[4]={CoopScriptBounds[i].level,CoopScriptBounds[i].index,CoopScriptBounds[i].low,CoopScriptBounds[i].high};fwrite(v,4,4,fixture);}
  n=(int32_t)CoopActorRuleCount;fwrite(&n,4,1,fixture);
  for(unsigned i=0;i<CoopActorRuleCount;i++){const CoopActorRule& r=CoopActorRules[i];int32_t v[7]={r.level,r.index,r.slot,r.slotHigh,r.healthLow,r.healthHigh,r.commandHigh};fwrite(v,4,7,fixture);}
  fclose(fixture);}
 puts("PASS: native ABI, bounded render list/local-last, frame rejection, coin bits, animation limits, portable asset path responses. No game launched.");
 return 0;
}


