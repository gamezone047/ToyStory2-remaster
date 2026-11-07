#include <math.h>
// Use the same camera basis and projection scale as the world renderer.
// Only rotation participates: the backdrop is infinitely far away.
struct SkyUV {float u,v;};
struct SkyRay {float x,y,z;};
static int SkyPitch(int angle){return (short)(angle&65535);}
static int SkyRepeats(float tileWidth){return Clamp((int)(6.28318530718f/(1.5f*tileWidth)+.5f),1,16);}
static SkyRay SkyWorldRay(float x,float y,float scaleX,float scaleY,const float* basis){
 float rx=(2*x-1)*scaleX,ry=(2*y-1)*scaleY;
 SkyRay ray={rx*basis[0]+ry*basis[4]+basis[8],
             rx*basis[1]+ry*basis[5]+basis[9],
             rx*basis[2]+ry*basis[6]+basis[10]};
 return ray;
}
static SkyUV SkyRayUV(SkyRay ray,float tileWidth,float tileHeight){
 const float pi=3.14159265358979323846f;
 SkyUV uv={atan2f(ray.x,ray.z)*SkyRepeats(tileWidth)/(2*pi),
  (.375f+atan2f(ray.y,sqrtf(ray.x*ray.x+ray.z*ray.z))/(2*tanf(.625f)))/tileHeight};
 return uv;
}
#ifdef CONTROLS_TEST
static SkyUV ProjectSky(float x,float y,int pitch,int yaw,float tanY,float aspect,float tw,float th){
 float p=SkyPitch(pitch)*6.28318530718f/65536,yawRad=-yaw*6.28318530718f/65536;
 float c=cosf(yawRad),s=sinf(yawRad),cp=cosf(p),sp=sinf(p);
 float basis[16]={c,0,-s,0,sp*s,cp,sp*c,0,cp*s,-sp,cp*c,0,0,0,0,1};
 return SkyRayUV(SkyWorldRay(x,y,tanY/aspect,tanY,basis),tw,th);
}
#endif
struct SkyVertex {float x,y,z,rhw;unsigned color,specular;float u,v;};
static const int SkyColumns=32,SkyRows=18,SkyVertexCount=SkyColumns*SkyRows*4,SkyIndexCount=SkyColumns*SkyRows*6;
static SkyVertex skyVertices[SkyVertexCount];
static unsigned short skyIndices[SkyIndexCount];
static void BuildSkyMesh(const float* basis,float scaleX,float scaleY,float width,float height,float tw,float th,float depth){
 const float offsets[4][2]={{0,0},{1,0},{0,1},{1,1}};
 const unsigned short order[6]={0,1,2,2,1,3};
 float repeats=(float)SkyRepeats(tw);
 for(int row=0;row<SkyRows;row++)for(int col=0;col<SkyColumns;col++){
  int cell=row*SkyColumns+col;float reference=0,origin=0;
  for(int corner=0;corner<4;corner++){
   float x=(col+offsets[corner][0])/SkyColumns,y=(row+offsets[corner][1])/SkyRows;
   SkyUV uv=SkyRayUV(SkyWorldRay(x,y,scaleX,scaleY,basis),tw,th);
   if(!corner){reference=uv.u;origin=floorf(reference);}
   // Unwrap the atan2 branch before interpolation. Integer texture periods
   // preserve continuity between cells, including either turn direction.
   uv.u-=floorf((uv.u-reference)/repeats+.5f)*repeats;
   SkyVertex& vertex=skyVertices[cell*4+corner];
   vertex={x*width,y*height,depth,1,0xffffffff,0,uv.u-origin,max(.0001f,min(.9999f,uv.v))};
  }
  for(int n=0;n<6;n++)skyIndices[cell*6+n]=(unsigned short)(cell*4+order[n]);
 }
}
static bool drawingSkyMesh;
typedef int (__cdecl *SkyIndexedFn)(int,unsigned,const void*,unsigned,const unsigned short*,unsigned,unsigned);
static int __cdecl DrawSkyIndexed(int primitive,unsigned format,const void* vertices,unsigned count,const unsigned short* indices,unsigned indexCount,unsigned flags){
 SkyIndexedFn native=(SkyIndexedFn)mem<uintptr_t>(0x508724);
 if(!drawingSkyMesh)return native(primitive,format,vertices,count,indices,indexCount,flags);
 // The native background quad establishes all render state and texture.
 // Temporarily wrap U; V is clamped in our vertices. Restore the cached native
 // clamp state before any world/HUD rendering continues.
 typedef void (__cdecl *StageFn)(int,int,int);
 ((StageFn)0x4ac150)(0,12,1);
 int result=native(4,0x1c4,skyVertices,SkyVertexCount,skyIndices,SkyIndexCount,flags);
 ((StageFn)0x4ac150)(0,12,3);
 return result;
}
static DWORD skyIndexedResume=0x4b6a44;
__declspec(naked) static void SkyIndexedHook(){__asm{
 call DrawSkyIndexed
 jmp dword ptr [skyIndexedResume]
}}
static void __cdecl StableSky(int pitch,int yaw){
 mem<int>(0x830c4c)=yaw&65535;
 ((void(__cdecl*)(int,int))0x48f230)(SkyPitch(pitch),yaw);
}
typedef void (__cdecl *SkyQuadFn)(float,float,float,float,const float*,const float*,uintptr_t,unsigned,unsigned);
static void __cdecl DrawPerspectiveSky(int immediate){
 if(mem<int>(0x88278c)<1||mem<int>(0x88278c)>15||mem<int>(0x55a0e0)){
  ((void(__cdecl*)(int))0x48f410)(immediate);return;
 }
 if(!mem<int>(0x500a54)||!mem<int>(0x5d2a90))return;
 uintptr_t texture=((uintptr_t(__cdecl*)(int))0x4ce2c0)(mem<int>(0x500a50));
 uintptr_t projection=mem<uintptr_t>(0xb623fc);
 if(!texture||!projection)return;
 float width=(float)((int(__cdecl*)())0x4abd90)(),height=(float)((int(__cdecl*)())0x4abda0)();
 float px=mem<float>(0xe4d970),py=mem<float>(0xe4d9a0);
 float tw=mem<float>(0x731cd4),th=mem<float>(0x731cd0);
 if(!(width>0&&height>0&&px>0&&py>0&&tw>0&&th>0)){
  ((void(__cdecl*)(int))0x48f410)(immediate);return;
 }
 BuildSkyMesh((const float*)projection,width/px,height/py,width,height,tw,th,mem<float>(0x5087e8));
 float uv0[2]={0,0},uv1[2]={1,1};
 drawingSkyMesh=true;
 ((SkyQuadFn)0x4b8cc0)(0,0,1,1,uv0,uv1,texture,0xffffffff,0x80000000);
 drawingSkyMesh=false;
}
static bool InstallSky(){
 const unsigned char indexed[]={0xff,0x15,0x24,0x87,0x50,0};
 if(memcmp((void*)0x4b6a3e,indexed,sizeof(indexed)))return false;
 return Patch(0x4b6a3e,indexed,sizeof(indexed),SkyIndexedHook)&&
  CallHook(0x441252,0x48f230,StableSky)&&CallHook(0x441260,0x48f410,DrawPerspectiveSky);
}
