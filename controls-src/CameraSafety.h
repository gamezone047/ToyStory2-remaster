#include <math.h>
// Replace the legacy collision-adjusted endpoint with a mouse-driven orbit
// constrained by world raycasts, with immediate inward and damped outward travel.
static float rayCameraDistance;
static DWORD rayCameraTime;
static unsigned rayCameraLevel;
static void __cdecl ResolveFinalCamera(void* camera){
 unsigned level=mem<unsigned>(0x88278c);
 if(!cameraOverride||mem<int>(0x50a13c)!=0||level<1||level>15){rayCameraTime=0;return;}
 int* position=(int*)camera;
 int anchor[3]={mem<int>(0x52f300),mem<int>(0x52f304)-0x1800,mem<int>(0x52f308)};
 // Construct the desired orbit from mouse angles every frame. Never use the
 // position shortened/lifted by native collision as the next frame's input.
 const float radians=6.28318530718f/4096.0f,radius=32768.0f;
 float yaw=thirdYaw*radians,pitch=thirdPitch*radians;
 float direction[3]={-sinf(yaw)*cosf(pitch),-sinf(pitch),-cosf(yaw)*cosf(pitch)};
 float allowed=radius;
 // Sweep the center and near-plane edges with parallel world rays.
 const int edges[5][2]={{0,0},{1,0},{-1,0},{0,1},{0,-1}};
 float right[3]={cosf(yaw),0,-sinf(yaw)};
 float up[3]={sinf(yaw)*sinf(pitch),-cosf(pitch),cosf(yaw)*sinf(pitch)};
 for(int ray=0;ray<5;ray++){
  int origin[3],delta[3];
  for(int axis=0;axis<3;axis++){origin[axis]=anchor[axis]+(int)(768*(right[axis]*edges[ray][0]+up[axis]*edges[ray][1]));delta[axis]=(int)(direction[axis]*radius);}
  if(worldRay(origin,delta,0x8000,1,0x1000)){
   float hit=delta[0]*direction[0]+delta[1]*direction[1]+delta[2]*direction[2]-1024.0f;
   if(hit<allowed)allowed=max(0.0f,hit);
  }
 }
 DWORD now=GetTickCount(),elapsed=now-rayCameraTime;
 if(!rayCameraTime||elapsed>250||rayCameraLevel!=level||allowed<rayCameraDistance)rayCameraDistance=allowed;
 else if(allowed>rayCameraDistance+96.0f){
  float blend=1.0f-expf(-(float)min(elapsed,100u)/150.0f);
  rayCameraDistance+=(allowed-rayCameraDistance)*blend;
 }
 rayCameraTime=now;rayCameraLevel=level;
 for(int axis=0;axis<3;axis++)position[axis]=position[axis+3]=anchor[axis]+(int)(direction[axis]*rayCameraDistance);
 *(short*)((unsigned char*)camera+0x20)=(short)(thirdPitch&4095);
 *(short*)((unsigned char*)camera+0x22)=(short)(thirdYaw&4095);
 *(short*)((unsigned char*)camera+0x24)=0;
 *(short*)((unsigned char*)camera+0x26)=1024;
}
static DWORD finalCameraResume=0x405825;
__declspec(naked) static void FinalCameraHook(){__asm{
 pushfd
 pushad
 push esi
 call ResolveFinalCamera
 add esp,4
 popad
 popfd
 pop edi
 pop esi
 pop ebp
 add esp,0x80
 jmp dword ptr [finalCameraResume]
}}
static void __cdecl GeometryDistance(float farDistance,float otherDistance){
 // Widescreen exposes architecture outside the original short radial range.
 // Double that finite range; keep room, frustum and actor culling intact.
 ((void(__cdecl*)(float,float))0x4bc410)(farDistance,otherDistance);
 if(farDistance>0.0f&&farDistance<1000000.0f)mem<float>(0x5088b0)=farDistance*farDistance*4.0f;
}
// The legacy minimum-distance sphere lifts Y after wall resolution. Against
// a nearby wall that alternates with the final sweep on every frame.
static DWORD minimumCameraResume=0x4057f4,minimumCameraSkip=0x40581c;
__declspec(naked) static void MinimumCameraHook(){__asm{
 cmp byte ptr [cameraOverride],0
 jne skip
 cmp edi,27100h
 jmp dword ptr [minimumCameraResume]
skip:
 jmp dword ptr [minimumCameraSkip]
}}
static bool InstallCameraSafety(){
 const unsigned char end[]={0x5f,0x5e,0x5d,0x81,0xc4,0x80,0,0,0};
 const unsigned char minimum[]={0x81,0xff,0,0x71,2,0};
 return Patch(0x4057ee,minimum,sizeof(minimum),MinimumCameraHook)&&
  Patch(0x40581c,end,sizeof(end),FinalCameraHook)&&
  CallHook(0x4cdd3b,0x4bc410,GeometryDistance)&&
  CallHook(0x4cde6d,0x4bc410,GeometryDistance)&&
  CallHook(0x4cdf4a,0x4bc410,GeometryDistance);
}
