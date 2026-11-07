// Development-only readback of the game's own render target, never desktop capture.
#include <ddraw.h>
static unsigned probeFrames;
static unsigned ProbeChannel(unsigned value,unsigned mask){if(!mask)return 0;unsigned shift=0;while(!(mask&1)){mask>>=1;++shift;}return ((value>>shift)&mask)*255/mask;}
static void ProbeSurface(unsigned object,unsigned offset,const char* label,unsigned frame){
 IDirectDrawSurface4* surface=*(IDirectDrawSurface4**)(object+offset);if(!surface)return;
 DDSURFACEDESC2 desc={};desc.dwSize=sizeof(desc);
 HRESULT result=surface->Lock(NULL,&desc,DDLOCK_READONLY|DDLOCK_WAIT,NULL);
 char message[300];sprintf(message,"Probe %s frame %u lock=%08x size=%lux%lu pitch=%ld bpp=%lu",label,frame,(unsigned)result,desc.dwWidth,desc.dwHeight,desc.lPitch,desc.ddpfPixelFormat.dwRGBBitCount);Log(message);
 if(FAILED(result))return;
 unsigned w=desc.dwWidth,h=desc.dwHeight,bpp=desc.ddpfPixelFormat.dwRGBBitCount;
 if(w&&h&&w<=8192&&h<=8192&&(bpp==16||bpp==32)){
  unsigned stride=(w*3+3)&~3u,bytes=stride*h;unsigned char* pixels=(unsigned char*)calloc(1,bytes);
  if(pixels){for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
   unsigned char* src=(unsigned char*)desc.lpSurface+y*desc.lPitch+x*(bpp/8);unsigned value=bpp==32?*(unsigned*)src:*(unsigned short*)src;
   unsigned char* dst=pixels+(h-1-y)*stride+x*3;dst[0]=(unsigned char)ProbeChannel(value,desc.ddpfPixelFormat.dwBBitMask);dst[1]=(unsigned char)ProbeChannel(value,desc.ddpfPixelFormat.dwGBitMask);dst[2]=(unsigned char)ProbeChannel(value,desc.ddpfPixelFormat.dwRBitMask);
  }
  char path[MAX_PATH];strcpy(path,logPath);char* slash=strrchr(path,'\\');sprintf(slash+1,"probe-%s-%u.bmp",label,frame);
  BITMAPFILEHEADER fh={};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(BITMAPINFOHEADER);fh.bfSize=fh.bfOffBits+bytes;
  BITMAPINFOHEADER ih={};ih.biSize=sizeof(ih);ih.biWidth=w;ih.biHeight=h;ih.biPlanes=1;ih.biBitCount=24;
  FILE* f=fopen(path,"wb");if(f){fwrite(&fh,sizeof(fh),1,f);fwrite(&ih,sizeof(ih),1,f);fwrite(pixels,bytes,1,f);fclose(f);}free(pixels);}
 }
 surface->Unlock(NULL);
}
static int __cdecl ProbePresent(){
 unsigned frame=++probeFrames;unsigned object=mem<unsigned>(0x884008);
 bool sample=mem<int>(0x88278c)==1&&(frame%300==0);
 if(sample&&object)ProbeSurface(object,0x34,"back",frame%600);
 int result=((int(__cdecl*)())0x4abd40)();
 if(sample&&object){RECT rc={};GetClientRect(GameWindow(),&rc);char detail[300];int* dest=(int*)(object+0x10);int* src=(int*)(object+0x20);sprintf(detail,"Probe present=%08x client=%ldx%ld nativeFull=%d dst=%d,%d,%d,%d src=%d,%d,%d,%d visible=%d focused=%d",result,rc.right,rc.bottom,*(int*)(object+4),dest[0],dest[1],dest[2],dest[3],src[0],src[1],src[2],src[3],IsWindowVisible(GameWindow()),Focused());Log(detail);}
 return result;
}
