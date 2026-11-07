using System;
using System.IO;

namespace Toy2Coop {
    // Exact 5408-byte ABI shared with CoopFrame.h. Immutable after validation.
    public sealed class GameFrame {
        public const int Size=5408;
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
            uint context=BitConverter.ToUInt32(data,5108);
            if(context!=0&&((context&0x10000)==0||(context&~0x10fffu)!=0||(context&255)>31||((context&0x100)!=0&&(context&255)!=5)))throw new InvalidDataException("Invalid world context");
            if(BitConverter.ToUInt32(data,4336)>31)throw new InvalidDataException("Invalid token reveal flags");
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
        public byte[] Bytes(){return (byte[])data.Clone();}
        internal void Write(BinaryWriter writer){writer.Write(data);}
        internal static GameFrame Read(BinaryReader reader){return new GameFrame(reader.ReadBytes(Size));}
    }
}
