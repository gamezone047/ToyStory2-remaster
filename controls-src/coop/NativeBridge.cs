using System;
using System.IO.MemoryMappedFiles;
using System.Threading;

namespace Toy2Coop {
    public sealed class NativeBridge : IDisposable {
        public readonly string Name="Local\\Toy2Coop_"+Guid.NewGuid().ToString("N");
        readonly Mutex mutex;
        readonly MemoryMappedFile map;
        readonly MemoryMappedViewAccessor view;
        public uint GameTick {get;private set;}
        public NativeBridge(){
            mutex=new Mutex(false,Name+"_lock");
            map=MemoryMappedFile.CreateNew(Name,16+2*GameFrame.Size);
            view=map.CreateViewAccessor(0,16+2*GameFrame.Size);
            view.Write(0,0x32504354u);view.Write(4,14u);
        }
        public GameFrame Exchange(GameFrame remote,bool connected){
            bool entered=false;
            try{
                try{entered=mutex.WaitOne(0);}catch(AbandonedMutexException){entered=true;}
                if(!entered)return null;
                byte[] bytes=new byte[GameFrame.Size];view.ReadArray(16,bytes,0,GameFrame.Size);
                GameTick=view.ReadUInt32(12);
                if(remote!=null)view.WriteArray(16+GameFrame.Size,remote.Bytes(),0,GameFrame.Size);
                view.Write(8,connected?unchecked((uint)Environment.TickCount):0u);
                return GameTick==0?null:new GameFrame(bytes);
            }finally{if(entered)mutex.ReleaseMutex();}
        }
        public void Dispose(){view.Dispose();map.Dispose();mutex.Dispose();}
    }
}
