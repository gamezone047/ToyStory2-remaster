// Backyard's pop-up army collectible (slot 4, model 13, health 102) is
// deliberately excluded from ordinary enemies. Native script: 419519..41991f.
static const uintptr_t BurrowActor=0x52cab0;
static bool BurrowActive(){return cachedLevel==2&&SameLevel()&&M<short>(BurrowActor+0x14)==13&&M<short>(BurrowActor+0x7e)==102&&!CoopBit(localFrame.actorCollections,4)&&!CoopBit(remoteFrame.actorCollections,4);}
static uintptr_t BurrowPath(unsigned& count){
 uintptr_t path=M<uintptr_t>(0x559c78);count=0;
 if(!Readable(path,4))return 0;count=M<unsigned short>(path);
 if(!count||count>32||!Readable(path,4+count*12)){count=0;return 0;}return path;
}
static uint32_t ReadClosedHoles(){unsigned count;uintptr_t path=BurrowPath(count);uint32_t bits=0;
 if(path)for(unsigned i=0;i<count;i++)if(M<int>(path+8+i*12)==(-2147483647-1))bits|=1u<<i;return bits;
}
static void MergeClosedHoles(){
 unsigned count;uintptr_t path=BurrowPath(count);if(!path)return;
 uint32_t bits=ReadClosedHoles()|localFrame.burrow.closed|remoteFrame.burrow.closed;
 if(count<32)bits&=(1u<<count)-1;localFrame.burrow.closed=bits;
 unsigned total=0;for(unsigned i=0;i<count;i++)if(bits&(1u<<i)){M<int>(path+8+i*12)=(-2147483647-1);++total;}
 M<unsigned>(0x52f6d0)=total;
 unsigned previous=M<unsigned>(0x52f5e8);if(previous<count&&(bits&(1u<<previous)))M<unsigned>(0x52f61c)=0;
 if(total>=6)M<unsigned short>(BurrowActor+0x40)=(M<unsigned short>(BurrowActor+0x40)&~0x20)|0x84;
}
static void CaptureBurrow(){
 CoopBurrow& b=localFrame.burrow;++b.revision;if(!b.revision)++b.revision;
 memcpy(&b.pose.x,&M<unsigned char>(BurrowActor),12);b.pose.pitch=M<short>(BurrowActor+12);b.pose.yaw=M<short>(BurrowActor+14);b.pose.roll=M<short>(BurrowActor+16);b.pose.health=1;
 b.pose.animation=M<short>(BurrowActor+18);b.pose.secondary=M<short>(BurrowActor+22);
 b.pose.frame=NativeEnemyFrame(13,b.pose.animation,M<int>(BurrowActor+24));b.pose.secondaryFrame=NativeEnemyFrame(13,b.pose.secondary,M<int>(BurrowActor+28));
 if(b.pose.frame<0){b.pose.animation=-1;b.pose.frame=0;}if(b.pose.secondaryFrame<0){b.pose.secondary=-1;b.pose.secondaryFrame=0;}
 memcpy(b.motion,&M<unsigned char>(BurrowActor+0x44),40);b.flags=M<unsigned short>(BurrowActor+0x40)&0x1ac;
 b.hole=M<unsigned>(0x52f5dc);b.previous=M<unsigned>(0x52f5e8);b.timer=min(100u,M<unsigned>(0x52f61c));
}
static void ApplyBurrow(){
 const CoopBurrow& b=remoteFrame.burrow;unsigned count;uintptr_t path=BurrowPath(count);
 if(!b.revision||!path||b.hole>=count||b.previous>=count)return;
 memcpy(&M<unsigned char>(BurrowActor),&b.pose.x,12);M<short>(BurrowActor+12)=b.pose.pitch;M<short>(BurrowActor+14)=b.pose.yaw;M<short>(BurrowActor+16)=b.pose.roll;
 int frame=NativeEnemyFrame(13,b.pose.animation,b.pose.frame),secondary=NativeEnemyFrame(13,b.pose.secondary,b.pose.secondaryFrame);
 if(frame>=0&&secondary>=0){M<short>(BurrowActor+18)=b.pose.animation;M<short>(BurrowActor+22)=b.pose.secondary;M<int>(BurrowActor+24)=frame;M<int>(BurrowActor+28)=secondary;}
 memcpy(&M<unsigned char>(BurrowActor+0x44),b.motion,40);
 M<unsigned short>(BurrowActor+0x40)=(M<unsigned short>(BurrowActor+0x40)&~0x1ac)|(b.flags&0x1ac);
 M<unsigned>(0x52f5dc)=b.hole;M<unsigned>(0x52f5e8)=b.previous;M<unsigned>(0x52f61c)=b.timer;
}
static void SyncBurrow(){if(!BurrowActive())return;MergeClosedHoles();if(localOwner==0)CaptureBurrow();else{ApplyBurrow();MergeClosedHoles();}}
static void BeginBurrow(){
 SyncBurrow();if(!BurrowActive()||localOwner!=0||remoteFrame.pose.health<=0)return;
 // Native emergence reacts to the nearest Buzz, including the distant guest.
 // Do not move the real local actor or replace any process-local pointers.
 if(!(M<unsigned short>(BurrowActor+0x40)&0x20)||M<int>(BurrowActor+0x60)==0x5000)return;
 int64_t distance=0;const int peer[3]={remoteFrame.pose.x,remoteFrame.pose.y,remoteFrame.pose.z};
 for(unsigned a=0;a<3;a++){int64_t d=((int64_t)peer[a]-M<int>(BurrowActor+4*a))>>5;distance+=d*d;}
 if(distance<0x90000){M<int>(BurrowActor+0x60)=0x5000;M<int>(0x52f61c)=100;M<int>(0x52f5e8)=M<int>(0x52f5dc);}
}
