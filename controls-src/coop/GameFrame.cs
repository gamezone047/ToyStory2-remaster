using System;
using System.IO;

namespace Toy2Coop {
    // Exact 8984-byte ABI shared with CoopFrame.h. Immutable after validation.
    public sealed class GameFrame {
        public const int Size=8984;
        readonly byte[] data;
        public readonly uint Sequence,Level,Layout,PickupCount;
        public GameFrame(byte[] bytes) {
            if(bytes==null||bytes.Length!=Size)throw new InvalidDataException("Invalid game frame size");
            data=(byte[])bytes.Clone();
            for(int index=0;index<64;index++){
                int offset=2112+index*32;if(BitConverter.ToUInt32(data,offset)==0)continue;
                for(int axis=0;axis<3;axis++){int v=BitConverter.ToInt32(data,offset+4+axis*4);if(v< -0x10000000||v>0x10000000)throw new InvalidDataException("Invalid enemy position");}
                for(int axis=0;axis<3;axis++){short v=BitConverter.ToInt16(data,offset+16+axis*2);if(v< -4096||v>4096)throw new InvalidDataException("Invalid enemy rotation");}
                short model=BitConverter.ToInt16(data,offset+22),health=BitConverter.ToInt16(data,offset+24);
                if(model<=0||model>255||health<0||health>=80||(BitConverter.ToUInt16(data,offset+26)&~0x10c)!=0||BitConverter.ToUInt32(data,offset+28)>1000000)throw new InvalidDataException("Invalid enemy state");
            }
            for(int i=0;i<64;i++){
                int o=4340+i*12;short a=BitConverter.ToInt16(data,o),b=BitConverter.ToInt16(data,o+2);int f=BitConverter.ToInt32(data,o+4),g=BitConverter.ToInt32(data,o+8);
                if(a< -1||a>=100||b< -1||b>=100||f<0||f>0x10000000||g<0||g>0x10000000)throw new InvalidDataException("Invalid enemy animation");
            }
            if(BitConverter.ToUInt32(data,5116)>4095)throw new InvalidDataException("Invalid RC contact");
            if(BitConverter.ToUInt32(data,5120)!=0){
                byte[] pose=new byte[Size];Buffer.BlockCopy(data,5124,pose,16,32);new GameFrame(pose);
                for(int i=0;i<10;i++){int v=BitConverter.ToInt32(data,5156+i*4);if(v< -0x10000000||v>0x10000000)throw new InvalidDataException("Invalid burrow motion");}
                if((BitConverter.ToUInt32(data,5196)&~0x1acu)!=0||BitConverter.ToUInt32(data,5200)>=32||BitConverter.ToUInt32(data,5204)>=32||BitConverter.ToUInt32(data,5208)>100)throw new InvalidDataException("Invalid burrow state");
            }
            if(BitConverter.ToUInt32(data,5216)!=0){
                byte[] pose=new byte[Size];Buffer.BlockCopy(data,5220,pose,16,32);new GameFrame(pose);
                for(int i=0;i<10;i++){int v=BitConverter.ToInt32(data,5252+i*4);if(v< -0x10000000||v>0x10000000)throw new InvalidDataException("Invalid mower motion");}
                if((BitConverter.ToUInt32(data,5292)&~0x19cu)!=0)throw new InvalidDataException("Invalid mower flags");
            }
            if(BitConverter.ToUInt32(data,5296)!=0){
                if(BitConverter.ToUInt32(data,5300)>1)throw new InvalidDataException("Invalid duck owner");
                for(int i=0;i<10;i++){int v=BitConverter.ToInt32(data,5304+i*4);int limit=(i==3||i==6||i==7)?4096:i==8?30:i==9?1:0x10000000;if(v>limit||v<(limit==0x10000000?-limit:0))throw new InvalidDataException("Invalid duck state");}
            }
            if(BitConverter.ToUInt32(data,5344)!=0&&(BitConverter.ToInt32(data,5348)<0||BitConverter.ToInt32(data,5348)>581||BitConverter.ToInt32(data,5352)<0||BitConverter.ToInt32(data,5352)>4096))throw new InvalidDataException("Invalid tree plank");
            if(BitConverter.ToUInt32(data,5356)!=0&&(BitConverter.ToUInt32(data,5360)>1||BitConverter.ToInt32(data,5364)<1||BitConverter.ToInt32(data,5364)>15))throw new InvalidDataException("Invalid level command");
            for(int i=0;i<2;i++){int o=5368+i*20;if(BitConverter.ToUInt32(data,o)!=0){int v=BitConverter.ToInt32(data,o+8),a=BitConverter.ToInt32(data,o+12),d=BitConverter.ToInt32(data,o+16);if(BitConverter.ToUInt32(data,o+4)>1||v< -65536||v>65536||a< -1792||a>(i==0?1792:896)||d< -4096||d>4096)throw new InvalidDataException("Invalid backyard platform");}}
            ValidateMechanisms(data);
            ValidateScript(data);
            ValidateMovers(data);
            uint context=BitConverter.ToUInt32(data,5108);
            if(context!=0&&((context&0x10000)==0||(context&~0x10fffu)!=0||(context&255)>31||((context&0x100)!=0&&(context&255)!=5)))throw new InvalidDataException("Invalid world context");
            if(BitConverter.ToUInt32(data,4336)>31)throw new InvalidDataException("Invalid token reveal flags");
            // 2026.11.7.14 tail: enemyLife[64] at 8892, removedActors[2] at 8956, tokenEvents[5] at 8964 (revision, shown, owner).
            for(int i=0;i<5;i++)if((BitConverter.ToUInt32(data,8964+i*4)&~0x3ffffu)!=0)throw new InvalidDataException("Invalid token event");
            if(BitConverter.ToUInt32(data,4332)>180)throw new InvalidDataException("Invalid damage flash");
            foreach(int o in new[]{4308,4320})if(BitConverter.ToUInt32(data,o)!=0){int v=BitConverter.ToInt32(data,o+8);if(BitConverter.ToUInt32(data,o+4)>1||v<0||v>=(o==4308?2:22))throw new InvalidDataException("Invalid session event");}
            if(BitConverter.ToUInt32(data,4292)!=0){int v=BitConverter.ToInt32(data,4300),h=BitConverter.ToInt32(data,4304);if(BitConverter.ToUInt32(data,4296)>1||v< -1||v>0x1000000||h< -0x10000000||h>0x10000000)throw new InvalidDataException("Invalid shovel");}
            if(BitConverter.ToUInt32(data,4180)!=0){
                byte[] pose=new byte[Size];Buffer.BlockCopy(data,4188,pose,16,32);new GameFrame(pose);
                if(BitConverter.ToUInt32(data,4184)>1||BitConverter.ToInt16(data,4206)>=5||BitConverter.ToInt16(data,4208)>=5)throw new InvalidDataException("Invalid RC pose");
                for(int i=0;i<10;i++){int v=BitConverter.ToInt32(data,4220+i*4);if(v< -0x10000000||v>0x10000000)throw new InvalidDataException("Invalid RC motion");}
                int hp=BitConverter.ToInt32(data,4284);
                if(BitConverter.ToUInt32(data,4260)>3||BitConverter.ToUInt32(data,4264)>(BitConverter.ToUInt32(data,4)==2?64u:44u)||BitConverter.ToUInt32(data,4268)>3||BitConverter.ToUInt32(data,4272)>3||BitConverter.ToUInt32(data,4276)>15||BitConverter.ToUInt32(data,4280)>1||(hp!=1&&hp!=202)||(BitConverter.ToUInt32(data,4288)&~0x18cu)!=0)throw new InvalidDataException("Invalid race state");
            }
            if(BitConverter.ToUInt32(data,4160)!=0){
                int velocity=BitConverter.ToInt32(data,4168),angle=BitConverter.ToInt32(data,4172),delta=BitConverter.ToInt32(data,4176);
                if(BitConverter.ToUInt32(data,4164)>1||velocity< -65536||velocity>65536||angle< -1792||angle>1792||delta< -4096||delta>4096)throw new InvalidDataException("Invalid balance platform state");
            }
            if(BitConverter.ToUInt32(data,1984)>3)throw new InvalidDataException("Invalid level interaction flags");
            if(BitConverter.ToUInt32(data,1996)!=0){
                if(BitConverter.ToUInt32(data,2000)>1||BitConverter.ToUInt32(data,2004)>3)throw new InvalidDataException("Invalid boss owner/phase");
                // Validate the actor pose using the same value-only pose parser.
                if(BitConverter.ToInt16(data,2026)>=8||BitConverter.ToInt16(data,2028)>=8)throw new InvalidDataException("Invalid Tin Robot animation");
                byte[] poseFrame=new byte[Size];Buffer.BlockCopy(data,2008,poseFrame,16,32);new GameFrame(poseFrame);
                for(int i=0;i<10;i++){int v=BitConverter.ToInt32(data,2040+i*4);if(v< -0x10000000||v>0x10000000)throw new InvalidDataException("Invalid boss motion");}
                int command=BitConverter.ToInt32(data,2080);
                if(command<0||command>2048||BitConverter.ToUInt32(data,2084)>1000000||BitConverter.ToUInt32(data,2088)>1000000||
                   BitConverter.ToUInt32(data,2096)>3600||BitConverter.ToUInt32(data,2100)>100||BitConverter.ToUInt32(data,2104)>1||BitConverter.ToUInt32(data,2108)>255)
                    throw new InvalidDataException("Invalid boss script state");
            }
            using(var r=new BinaryReader(new MemoryStream(data))) {
                Sequence=r.ReadUInt32();Level=r.ReadUInt32();Layout=r.ReadUInt32();PickupCount=r.ReadUInt32();
                if(Level>16||PickupCount>512)throw new InvalidDataException("Invalid level/pickup count");
                for(int i=0;i<3;i++){int v=r.ReadInt32();if(v< -0x10000000||v>0x10000000)throw new InvalidDataException("Invalid position");}
                for(int i=0;i<3;i++){short v=r.ReadInt16();if(v< -4096||v>4096)throw new InvalidDataException("Invalid rotation");}
                for(int i=0;i<2;i++){short v=r.ReadInt16();if(v< -1||v>=35)throw new InvalidDataException("Invalid animation");}
                for(int i=0;i<2;i++){int v=r.ReadInt32();if(v<0||v>0x10000000)throw new InvalidDataException("Invalid animation time");}
                short health=r.ReadInt16();if(health< -1||health>100)throw new InvalidDataException("Invalid health");
                r.BaseStream.Position=120;
                if(r.ReadUInt32()>255||r.ReadUInt32()!=0)throw new InvalidDataException("Invalid reserved frame fields");
                r.BaseStream.Position=136;
                for(int shot=0;shot<8;shot++){
                    uint sequence=r.ReadUInt32();
                    for(int axis=0;axis<9;axis++){int v=r.ReadInt32();int bound=axis<6?0x10000000:0x1000000;if(sequence!=0&&(v< -bound||v>bound))throw new InvalidDataException("Invalid laser coordinates");}
                    short life=r.ReadInt16(),length=r.ReadInt16();r.ReadUInt32();
                    if(sequence!=0&&(life<0||life>64||length<0))throw new InvalidDataException("Invalid laser lifetime");
                }
                for(int item=0;item<10;item++){
                    uint revision=r.ReadUInt32(),owner=r.ReadUInt32();
                    for(int axis=0;axis<5;axis++){int v=r.ReadInt32();int bound=axis<3?0x10000000:0x1000000;if(revision!=0&&(v< -bound||v>bound))throw new InvalidDataException("Invalid movable object position");}
                    for(int field=0;field<5;field++)r.ReadInt16();
                    short node=r.ReadInt16(),minimum=r.ReadInt16(),path=r.ReadInt16();
                    if(revision!=0&&(owner>1||node<0||minimum<0||minimum>node||path<0))throw new InvalidDataException("Invalid movable object state");
                }
            }
        }
        // Mirrors CoopValidMechanisms: level-specific slots at 5408 + i*24.
        static void ValidateMechanisms(byte[] data){
            uint level=BitConverter.ToUInt32(data,4);
            for(int i=0;i<4;i++){
                int o=5408+i*24;if(BitConverter.ToUInt32(data,o)==0)continue;
                int[] v=new int[4];for(int k=0;k<4;k++)v[k]=BitConverter.ToInt32(data,o+8+k*4);
                bool ok=BitConverter.ToUInt32(data,o+4)<=1;
                if(level==1){
                    if(i==0){for(int k=0;k<4;k++)ok&=v[k]>=0&&v[k]<=0x1fff;}
                    else if(i==1)ok&=v[0]>=0&&v[0]<=0x1000&&v[1]==0&&v[2]==0&&v[3]==0;
                    else ok=false;
                }else if(level==2){
                    if(i==0)ok&=v[0]>=0&&v[0]<=0x1000&&v[1]>=0&&v[1]<=0x1000&&v[2]>=0&&v[2]<=2&&v[3]==0;
                    if(i==1)ok&=(v[0]==Int32.MinValue||(v[0]>=-0x10000&&v[0]<=0x10000))&&v[1]>=-0x1000&&v[1]<=0x1000&&v[2]==0&&v[3]==0;
                    if(i==2)ok&=v[0]>=-1&&v[0]<=0x800&&v[1]>=-0x10000000&&v[1]<=0x10000000&&v[2]==0&&v[3]==0;
                    if(i==3)ok&=v[0]>=0&&v[0]<=0xc8&&v[1]==0&&v[2]==0&&v[3]==0;
                }else ok=false;
                if(!ok)throw new InvalidDataException("Invalid level mechanism state");
            }
        }
        // Mirrors CoopScriptBounds/CoopActorRules in CoopFrame.h. CoopTests compares
        // these tables with the native dump (native-rules.bin).
        // {level,index,low,high}
        internal static readonly int[][] ScriptBounds={
            // level 3 boss: phase, damage, height target, height, expected health, flash, cutscene length
            new[]{3,0,0,0x500},new[]{3,1,0x1000,0x3800},new[]{3,2,0,0x3800},new[]{3,3,-0x100,0x4000},new[]{3,4,-1,1000},new[]{3,5,0,4},new[]{3,6,0,0x400},
            // level 4 progress: target bits, sequence rounds, actor 24 stage
            new[]{4,0,0,7},new[]{4,1,0,7},new[]{4,2,0,0xc8},
            // rescued-friend reward stages
            new[]{5,0,0,0xc8},new[]{7,0,0,0xc8},new[]{8,0,0,0xc8},new[]{10,0,0,0xc8},new[]{11,0,0,0xc8},new[]{13,0,0,0xc8},new[]{14,0,0,0xc8},
            // level 7 NPC door target; level 10 target bits; level 11 bridges
            new[]{5,1,0,1},new[]{7,1,0,0x4000},new[]{10,1,0,7},new[]{11,1,0,0x10000},new[]{11,2,0,0x10000},new[]{11,3,0,0x3d},new[]{11,4,0,0x3d},new[]{11,5,0,0x3d},new[]{11,6,0,0x3d},new[]{11,7,0,0x3d},new[]{11,8,0,0x3d},
            // level 6 boss: phase, expected health, hurt timer, attack cycle, waypoint x, oscillation, outside flag
            new[]{6,0,0,4},new[]{6,1,-1,1000},new[]{6,2,-0x100,0x12c},new[]{6,3,-0x100,0x258},new[]{6,4,-0x40000,0x40000},new[]{6,5,0,0x800},new[]{6,6,0,1},
            // level 12 boss: phase, expected health, hurt timer, rise velocity
            new[]{12,0,0,4},new[]{12,1,-1,1000},new[]{12,2,-0x100,0x3c},new[]{12,3,0,0x100000},
            // level 9 boss: phase, hit count, pair index, pair slots A/B, expected health, release timer,
            // release flag, orbit angle, bob angle, orbit frame, boss height
            new[]{9,0,0,5},new[]{9,1,0,7},new[]{9,2,0,12},new[]{9,3,7,11},new[]{9,4,7,11},new[]{9,5,-1,1000},new[]{9,6,-0x100,0x258},new[]{9,7,0,1},
            new[]{9,8,0,0xfff},new[]{9,9,0,0xfff},new[]{9,10,0,0x180000},new[]{9,11,-0x10000000,0x10000000},
            // level 15 boss: defeats, fight flags 0/1/2, expected health 0/1/2, hurt timers 0/1/2, last defeated
            new[]{15,0,0,5},new[]{15,1,0,3},new[]{15,2,0,3},new[]{15,3,0,3},new[]{15,4,-1,1000},new[]{15,5,-1,1000},new[]{15,6,-1,1000},
            new[]{15,7,-0x100,0x3c},new[]{15,8,-0x100,0x3c},new[]{15,9,-0x100,0x3c},new[]{15,10,0,2},
        };
        // {level,index,slot,slotHigh,healthLow,healthHigh,commandHigh}
        internal static readonly int[][] ActorRules={
            new[]{3,0,0,0,-1,1000,4096},new[]{6,0,0,0,-1,1000,4096},new[]{12,0,0,0,-1,1000,4096},
            new[]{9,0,0,0,-1,1000,4096},new[]{9,1,7,11,-1,1000,4096},new[]{9,2,7,11,-1,1000,4096},
            new[]{15,0,0,0,-1,1000,4096},new[]{15,1,1,1,-1,1000,4096},new[]{15,2,2,2,-1,1000,4096},
        };
        // Script actors at 5504 + i*116; script variables at 5968.
        static void ValidateScript(byte[] data){
            uint level=BitConverter.ToUInt32(data,4);
            uint revision=BitConverter.ToUInt32(data,5968),owner=BitConverter.ToUInt32(data,5972);
            if(revision!=0){
                if(owner>2)throw new InvalidDataException("Invalid script owner");
                for(int i=0;i<32;i++){
                    int v=BitConverter.ToInt32(data,5976+i*4);bool listed=false;
                    foreach(int[] b in ScriptBounds)if(b[0]==level&&b[1]==i){listed=true;if(v<b[2]||v>b[3])throw new InvalidDataException("Invalid script value");}
                    if(!listed&&v!=0)throw new InvalidDataException("Unexpected script value");
                }
            }else if(owner!=0)throw new InvalidDataException("Invalid script owner");
            for(int i=0;i<4;i++){
                int o=5504+i*116;if(BitConverter.ToUInt32(data,o)==0)continue;
                int[] rule=null;foreach(int[] r in ActorRules)if(r[0]==level&&r[1]==i)rule=r;
                if(rule==null||data[o+4]<rule[2]||data[o+4]>rule[3]||data[o+5]>1||data[o+7]>1)throw new InvalidDataException("Invalid script actor");
                const int bound=0x10000000;
                for(int a=0;a<3;a++){int v=BitConverter.ToInt32(data,o+8+a*4);if(v< -bound||v>bound)throw new InvalidDataException("Invalid script actor position");}
                for(int a=0;a<3;a++){short v=BitConverter.ToInt16(data,o+20+a*2);if(v< -4096||v>4096)throw new InvalidDataException("Invalid script actor rotation");}
                for(int a=0;a<2;a++){short v=BitConverter.ToInt16(data,o+26+a*2);if(v< -1||v>=100)throw new InvalidDataException("Invalid script actor animation");}
                for(int a=0;a<2;a++){int v=BitConverter.ToInt32(data,o+30+a*4);if(v<0||v>bound)throw new InvalidDataException("Invalid script actor frame");}
                short health=BitConverter.ToInt16(data,o+38);if(health<rule[4]||health>rule[5])throw new InvalidDataException("Invalid script actor health");
                for(int k=0;k<10;k++){int v=BitConverter.ToInt32(data,o+40+k*4);if(v< -bound||v>bound)throw new InvalidDataException("Invalid script actor motion");}
                int command=BitConverter.ToInt32(data,o+100);short timer=BitConverter.ToInt16(data,o+104);
                if(command< -1||command>rule[6]||timer< -1||BitConverter.ToUInt32(data,o+108)>1000000||BitConverter.ToUInt32(data,o+112)>1000000)throw new InvalidDataException("Invalid script actor state");
            }
        }
        // Mirrors CoopValidMovers: standing at 6104, CoopMover movers[24] at 6108 (116 bytes each).
        internal static readonly int[] MoverGroupCounts={0,0,0,0,12,8,0,10,2,0,3,6,0,7,2,0};
        static int MoverGroups(uint level){return level<16?MoverGroupCounts[level]:0;}
        static void ValidateMovers(byte[] data){
            uint level=BitConverter.ToUInt32(data,4);int standing=BitConverter.ToInt32(data,6104);
            if(standing< -1||standing>255)throw new InvalidDataException("Invalid standing collider");
            for(int i=0;i<24;i++){
                int o=6108+i*116;if(BitConverter.ToUInt32(data,o)==0)continue;
                if(i>=MoverGroups(level)||data[o+5]!=i||data[o+4]>1||data[o+7]>7||data[o+16]>1||data[o+17]!=0||data[o+18]!=0||data[o+19]!=0)throw new InvalidDataException("Invalid collider group");
                for(int k=0;k<10;k++){int v=BitConverter.ToInt32(data,k<2?o+8+k*4:o+20+(k-2)*4);if(v< -0x10000000||v>0x10000000)throw new InvalidDataException("Invalid collider group value");}
                for(int k=0;k<2;k++){int c=o+52+k*32;
                    if(data[c+31]>1||(data[c+30]&~0x89)!=0)throw new InvalidDataException("Invalid collider state");
                    for(int a=0;a<3;a++){int v=BitConverter.ToInt32(data,c+a*4);if(v< -0x10000000||v>0x10000000)throw new InvalidDataException("Invalid collider position");}
                }
            }
        }
        public byte[] Bytes(){return (byte[])data.Clone();}
        internal void Write(BinaryWriter writer){writer.Write(data);}
        internal static GameFrame Read(BinaryReader reader){return new GameFrame(reader.ReadBytes(Size));}
    }
}
