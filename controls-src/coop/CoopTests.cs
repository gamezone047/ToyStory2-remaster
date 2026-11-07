using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Diagnostics;
using Toy2Coop;

static class CoopTests {
    static void Check(bool condition,string message){if(!condition)throw new Exception(message);}
    static void Wait(Func<bool> condition,string message){
        var until=DateTime.UtcNow.AddSeconds(5);
        while(DateTime.UtcNow<until){if(condition())return;Thread.Sleep(10);}
        throw new Exception("Timeout: "+message);
    }
    static int Main(){try{
        LocalTestTests.Run();
        string preferences=Path.GetFullPath("controls-src/coop/preferences-test.txt");
        try{
            CoopPreferences.Save(preferences,"Player Two","94.30.213.139");var saved=CoopPreferences.Load(preferences);
            Check(saved[0]=="Player Two"&&saved[1]=="94.30.213.139","multiplayer entries round trip");
            File.WriteAllText(preferences,"corrupt");Check(CoopPreferences.Load(preferences)[0]=="Buzz","damaged preferences fallback");
        }finally{if(File.Exists(preferences))File.Delete(preferences);}
        byte[] golden=File.ReadAllBytes("controls-src/coop/native-frame.bin");
        var nativeFrame=new GameFrame(golden);
        Check(BitConverter.ToUInt32(golden,4264)==12&&BitConverter.ToInt32(golden,4304)==1234&&BitConverter.ToInt32(golden,4328)==7&&BitConverter.ToUInt32(golden,4332)==90,"garage/session ABI");
        Check(BitConverter.ToUInt32(golden,4336)==5,"token reveal ABI");
        Check(BitConverter.ToInt16(golden,5096)==3&&BitConverter.ToInt32(golden,5100)==0x12345,"last enemy animation ABI");
        Check(BitConverter.ToUInt32(golden,5108)==0x10105,"room and plank contact ABI");
        Check(BitConverter.ToUInt32(golden,5112)==9&&BitConverter.ToUInt32(golden,5116)==1024&&BitConverter.ToUInt32(golden,5200)==2&&BitConverter.ToUInt32(golden,5208)==60&&BitConverter.ToUInt32(golden,5212)==3,"RC push and backyard ABI");
        Check(BitConverter.ToInt32(golden,5220)==7654&&BitConverter.ToInt32(golden,5316)==4096&&BitConverter.ToInt32(golden,5348)==581&&BitConverter.ToInt32(golden,5364)==15,"backyard and travel ABI");
        foreach(int o in new[]{5372,5376,5380,5384,5392,5396,5400,5404,5220,5252,5292,5300,5316,5336,5340,5348,5352,5360,5364,5116,5124,5156,5196,5200,5204,5208,5108,5096,5100,5104,4336,4184,4206,4260,4264,4284,4296,4300,4304,4316,4328,4332}){byte[] bad=(byte[])golden.Clone();Array.Copy(BitConverter.GetBytes(Int32.MaxValue),0,bad,o,4);bool refusedEvent=false;try{new GameFrame(bad);}catch(InvalidDataException){refusedEvent=true;}Check(refusedEvent,"invalid garage/session frame");}

        Check(BitConverter.ToInt32(golden,4172)==-256&&BitConverter.ToInt32(golden,4176)==8,"platform ABI");
        foreach(int offset in new[]{4164,4168,4172,4176}){byte[] bad=(byte[])golden.Clone();Array.Copy(BitConverter.GetBytes(Int32.MaxValue),0,bad,offset,4);bool platformRejected=false;try{new GameFrame(bad);}catch(InvalidDataException){platformRejected=true;}Check(platformRejected,"platform bounds");}

        Check(BitConverter.ToInt32(golden,4132)==4321&&BitConverter.ToInt16(golden,4152)==4,"last enemy ABI");
        foreach(int offset in new[]{2112+63*32+4,2112+63*32+28}){byte[] bad=(byte[])golden.Clone();Array.Copy(BitConverter.GetBytes(Int32.MaxValue),0,bad,offset,4);bool refused=false;try{new GameFrame(bad);}catch(InvalidDataException){refused=true;}Check(refused,"invalid enemy data rejected");}
        Check(BitConverter.ToUInt32(golden,1992)==0x80000000&&BitConverter.ToUInt32(golden,2004)==2,"quest actor and boss extension ABI");
        Check(BitConverter.ToUInt16(golden,1982)==123,"last collectible generation survives native ABI");
        byte[] backyard=(byte[])golden.Clone();Array.Copy(BitConverter.GetBytes(2u),0,backyard,4,4);Array.Copy(BitConverter.GetBytes(0u),0,backyard,5108,4);Array.Copy(BitConverter.GetBytes(64u),0,backyard,4264,4);new GameFrame(backyard);
        Array.Copy(BitConverter.GetBytes(65u),0,backyard,4264,4);bool badRc=false;try{new GameFrame(backyard);}catch(InvalidDataException){badRc=true;}Check(badRc,"backyard RC path bounds");
        Check(nativeFrame.Sequence==42&&nativeFrame.Level==1&&nativeFrame.Layout==0x12345678&&nativeFrame.PickupCount==512,"native/managed ABI round trip");
        Check(BitConverter.ToInt32(nativeFrame.Bytes(),16)==12345&&BitConverter.ToInt32(nativeFrame.Bytes(),20)==-9876,"native pose offsets");
        Check(BitConverter.ToUInt32(golden,128)==0x18&&BitConverter.ToUInt32(golden,136)==11&&BitConverter.ToUInt32(golden,520)==12,"drop/laser/object ABI offsets");
        foreach(int offset in new[]{140,528,1984,2000,2004,2080,2084,2096,2100,2104,2108}){
            byte[] bad=(byte[])golden.Clone();Array.Copy(BitConverter.GetBytes(Int32.MaxValue),0,bad,offset,4);
            bool refused=false;try{new GameFrame(bad);}catch(InvalidDataException){refused=true;}Check(refused,"invalid shot/object frame rejected");
        }
        using(var bridge=new NativeBridge()){
            bridge.Exchange(nativeFrame,true);
            using(var native=Process.Start(new ProcessStartInfo("controls-src/coop/NativeTests.exe","--ipc "+bridge.Name){UseShellExecute=false,CreateNoWindow=true})){
                Check(native.WaitForExit(5000)&&native.ExitCode==0,"native bridge subprocess");
            }
            var received=bridge.Exchange(nativeFrame,true);
            Check(received!=null&&received.Sequence==1234&&BitConverter.ToInt32(received.Bytes(),16)==67890,"cross-process native to managed frame");
            using(var mutex=Mutex.OpenExisting(bridge.Name+"_lock")){
                var held=new ManualResetEvent(false);var release=new ManualResetEvent(false);
                var holder=new Thread(()=>{mutex.WaitOne();held.Set();release.WaitOne();mutex.ReleaseMutex();});holder.Start();held.WaitOne();
                var watch=Stopwatch.StartNew();Check(bridge.Exchange(nativeFrame,true)==null&&watch.ElapsedMilliseconds<100,"busy bridge must not block");
                release.Set();holder.Join();held.Dispose();release.Dispose();
            }
        }
        byte[] invalid=(byte[])golden.Clone();invalid[34]=35;invalid[35]=0;
        bool rejected=false;try{new GameFrame(invalid);}catch(InvalidDataException){rejected=true;}Check(rejected,"invalid animation rejected");
        using(var host=new CoopHost("Host",IPAddress.Loopback,0)){
            Check(host.Snapshot().Players.Count==1,"host must occupy slot 0");
            Check(!host.StartCampaign(1),"one player cannot start");
            Check(!host.CollectCoins(1,10),"no lobby coin awards");
            using(var a=new CoopClient("127.0.0.1",host.Port,"Buzz 2"))
            using(var b=new CoopClient("127.0.0.1",host.Port,"Buzz 3"))
            using(var c=new CoopClient("127.0.0.1",host.Port,"Buzz 4")){
                Wait(()=>a.Snapshot()!=null&&b.Snapshot()!=null&&c.Snapshot()!=null&&c.Snapshot().Players.Count==4,"four-player lobby");
                Check(a.Snapshot().LocalPlayer!=b.Snapshot().LocalPlayer&&b.Snapshot().LocalPlayer!=c.Snapshot().LocalPlayer,"unique slot IDs");
                using(var extra=new CoopClient("127.0.0.1",host.Port,"Fifth")){
                    Wait(()=>extra.Error!=null,"fifth player rejected");Check(extra.Error.Contains("full"),"clear lobby-full response");
                }
                Check(host.StartCampaign(1),"host can start with 2-4 players");
                Check(!host.StartCampaign(1),"start is single-shot");
                Wait(()=>a.Snapshot().Started&&b.Snapshot().Started&&c.Snapshot().Started,"campaign start broadcast");
                host.PublishFrame(nativeFrame);
                Wait(()=>a.LatestFrame()!=null&&b.LatestFrame()!=null&&c.LatestFrame()!=null,"host native frame broadcast");
                Check(a.LatestFrame().Bytes()[48]==255,"coin bits delivered");
                Check(BitConverter.ToInt32(a.LatestFrame().Bytes(),4172)==-256,"plank state delivered over TCP");
                Check(BitConverter.ToUInt16(a.LatestFrame().Bytes(),1982)==123&&BitConverter.ToUInt32(a.LatestFrame().Bytes(),1984)==3,"collectible generations and crib events delivered across TCP");
                Check(BitConverter.ToUInt32(a.LatestFrame().Bytes(),128)==0x18&&BitConverter.ToUInt32(a.LatestFrame().Bytes(),136)==11&&BitConverter.ToUInt32(a.LatestFrame().Bytes(),520)==12,"drop, shot and pushable state delivered");
                a.SendFrame(nativeFrame);
                Wait(()=>host.LatestFrame(a.Snapshot().LocalPlayer)!=null,"client native frame delivered");
                byte[] stale=nativeFrame.Bytes();stale[0]=41;a.SendFrame(new GameFrame(stale));
                Thread.Sleep(50);Check(host.LatestFrame(a.Snapshot().LocalPlayer).Sequence==42,"stale native frame ignored");
                Check(host.CollectCoins(10,10)&&host.CollectCoins(20,10),"two distinct host-validated pickups");
                Check(!host.CollectCoins(10,10),"duplicate pickup must not count twice");
                Wait(()=>a.Snapshot().Coins==20&&b.Snapshot().Coins==20&&c.Snapshot().Coins==20,"shared 10+10=20 coins");
                byte player=a.Snapshot().LocalPlayer;
                a.SendInput(new PlayerInput{Sequence=2,Buttons=0x10,Yaw=1024,Pitch=32});
                Wait(()=>host.LatestInput(player)!=null,"remote input delivered");
                a.SendInput(new PlayerInput{Sequence=1,Buttons=0,Yaw=0,Pitch=0});
                Thread.Sleep(50);Check(host.LatestInput(player).Sequence==2,"stale input ignored");
                c.Dispose();Wait(()=>host.Snapshot().Players.Count==3,"disconnect frees lobby slot");
                using(var late=new CoopClient("127.0.0.1",host.Port,"Late"))Wait(()=>late.Error!=null,"late join rejected once started");
                Check(host.Snapshot().Coins==20,"disconnect preserves shared items");
            }
        }
        using(var host=new CoopHost("Host",IPAddress.Loopback,0)){
            // Oversized and wrong-version packets must not crash the host or allocate arbitrary buffers.
            using(var raw=new TcpClient("127.0.0.1",host.Port)){
                var w=new BinaryWriter(raw.GetStream());w.Write(Wire.Magic);w.Write(Wire.Version);w.Write(Int32.MaxValue);w.Flush();
            }
            using(var a=new CoopClient("127.0.0.1",host.Port,"Valid")){
                Wait(()=>a.Snapshot()!=null,"host survives malformed traffic");
                Check(host.StartCampaign(1),"exactly two players can start");
            }
        }
        using(var host=new CoopHost("Host",IPAddress.Loopback,0,2))
        using(var a=new CoopClient("127.0.0.1",host.Port,"Second")){
            Wait(()=>a.Snapshot()!=null,"two-player test lobby");
            using(var third=new CoopClient("127.0.0.1",host.Port,"Third"))Wait(()=>third.Error!=null,"two-player build rejects a third player");
        }
        Console.WriteLine("PASS: real loopback host + 3 clients, unique IDs, full lobby rejection, minimum 2 players, start broadcast, shared coins/deduplication, ordered input, disconnect, no late join, malformed traffic.");
        Console.WriteLine("PASS: native/managed frame ABI, host/client frame transport, invalid animation, stale frames, two-player capacity. No game launched; native engine hooks still need in-game validation.");
        return 0;
    }catch(Exception e){Console.Error.WriteLine(e);return 1;}}
}

