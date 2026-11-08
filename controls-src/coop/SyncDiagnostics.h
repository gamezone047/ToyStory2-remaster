// Synchronization diagnostics for live two-player tests. Writes compact, rate
// limited lines to CoopBridge.log ("sync ..."), keeps counters for the periodic
// summary and serves the console command "coop" (CoopStatus export).
// Diagnostics only: nothing here changes synchronized state.
#include <stdarg.h>
static unsigned scriptOwnerDiagnostic();
struct SyncStats {uint32_t framesIn,framesRejected,scriptOwnerChanges,varAdoptions,groupOwnerChanges,requestsSent,requestsApplied,requestsRefused,lastRemoteSequence;};
static SyncStats syncStats;static DWORD syncSummaryAt,syncRejectAt;static char syncLastLine[256];static unsigned syncLines;
static void SyncLog(const char* format,...){
 char line[240];va_list args;va_start(args,format);_vsnprintf(line,sizeof(line)-1,format,args);va_end(args);line[sizeof(line)-1]=0;
 snprintf(syncLastLine,sizeof(syncLastLine),"sync L%u %s %s",cachedLevel,localOwner?"guest":"host",line);++syncLines;
#ifndef COOP_TEST
 Log(syncLastLine);
#endif
}
static const char* FrameRejectReason(const CoopFrame& f){
 if(!CoopValidMechanisms(f))return "level 1-2 mechanisms";
 if(!CoopValidScript(f))return "script values/actors";
 if(!CoopValidMovers(f))return "mechanism groups";
 return "core fields (pose, enemies, objects, events)";
}
// Called for every frame read from the helper; counts new frames and reports rejections.
static void SyncObserveFrame(const CoopFrame& f,bool valid){
 if(f.sequence==syncStats.lastRemoteSequence)return;syncStats.lastRemoteSequence=f.sequence;
 if(valid){++syncStats.framesIn;return;}
 ++syncStats.framesRejected;DWORD now=GetTickCount();
 if(!syncRejectAt||now-syncRejectAt>=5000){syncRejectAt=now;SyncLog("REJECTED remote frame seq %u level %u: %s (total %u)",f.sequence,f.level,FrameRejectReason(f),syncStats.framesRejected);}
}
static char SyncOwnerChar(unsigned owner){return owner==0?'H':owner==1?'G':'-';}
static void SyncGroupOwners(char* out,unsigned size){
 unsigned n=0;for(unsigned i=0;i<24&&n+1<size;i++){const CoopMover& m=localFrame.movers[i];if(!m.revision)break;out[n++]=SyncOwnerChar(m.owner);}out[n]=0;
}
static void SyncSummary(bool force){
 DWORD now=GetTickCount();if(!force&&syncSummaryAt&&now-syncSummaryAt<30000)return;syncSummaryAt=now;
 char groups[32];SyncGroupOwners(groups,sizeof(groups));
 SyncLog("summary linked %d helper %lums frames %u rejected %u script owner %c groups %s requests sent %u applied %u refused %u",
  SameLevel()?1:0,helperTick?GetTickCount()-helperTick:0,syncStats.framesIn,syncStats.framesRejected,SyncOwnerChar(scriptOwnerDiagnostic()),
  groups[0]?groups:"none",syncStats.requestsSent,syncStats.requestsApplied,syncStats.requestsRefused);
}
// Console "coop": one 38-column line; the full state goes to the log.
extern "C" __declspec(dllexport) int __cdecl CoopStatus(char* out,int size){
 if(!out||size<8)return 0;
 if(!shared){snprintf(out,size,"co-op not running");return 1;}
 char groups[32];SyncGroupOwners(groups,sizeof(groups));unsigned mine=0;for(unsigned i=0;groups[i];i++)mine+=groups[i]==(localOwner?'G':'H');
 snprintf(out,size,"L%u %s %s s%c g%u/%u x%u",cachedLevel,localOwner?"guest":"host",SameLevel()?"linked":"NOT LINKED",
  SyncOwnerChar(scriptOwnerDiagnostic()),mine,(unsigned)strlen(groups),syncStats.framesRejected);
 SyncSummary(true);return 1;
}
