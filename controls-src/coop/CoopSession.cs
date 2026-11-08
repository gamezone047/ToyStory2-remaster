using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace Toy2Coop {
    // Development networking module. It deliberately has no native game-memory
    // writes: a host-authoritative game adapter must supply pickup events/input.
    public sealed class LobbyState {
        public int Revision, Coins, Level = 1;
        public bool Started;
        public byte LocalPlayer;
        public readonly Dictionary<byte,string> Players = new Dictionary<byte,string>();
    }
    public sealed class PlayerInput {
        public uint Sequence;
        public ushort Buttons, Yaw;
        public short Pitch;
    }
    static class Wire {
        public const int Magic = 0x324f4f43, Version = 24, MaxPacket = 16384;
        public const byte Hello=1, State=2, Heartbeat=3, Input=4, Reject=5, Frame=6;
        public static byte[] Packet(byte type, Action<BinaryWriter> body) {
            using(var m=new MemoryStream()) {
                var w=new BinaryWriter(m,Encoding.UTF8);w.Write(type);body(w);w.Flush();
                if(m.Length>MaxPacket)throw new InvalidDataException("Packet too large");
                return m.ToArray();
            }
        }
        public static void Send(NetworkStream s, byte[] packet) {
            lock(s) {
                var w=new BinaryWriter(s,Encoding.UTF8);
                w.Write(Magic);w.Write(Version);w.Write(packet.Length);w.Write(packet);w.Flush();
            }
        }
        public static BinaryReader Read(NetworkStream s) {
            var r=new BinaryReader(s,Encoding.UTF8);
            if(r.ReadInt32()!=Magic||r.ReadInt32()!=Version)throw new InvalidDataException("Incompatible multiplayer protocol");
            int size=r.ReadInt32();if(size<1||size>MaxPacket)throw new InvalidDataException("Invalid packet size");
            byte[] bytes=r.ReadBytes(size);if(bytes.Length!=size)throw new EndOfStreamException();
            return new BinaryReader(new MemoryStream(bytes),Encoding.UTF8);
        }
        public static void End(BinaryReader r) {
            if(r.BaseStream.Position!=r.BaseStream.Length)throw new InvalidDataException("Trailing packet data");
        }
        public static string Name(string name) {
            if(String.IsNullOrWhiteSpace(name)||name.Length>24)throw new InvalidDataException("Player name must be 1-24 characters");
            foreach(char c in name)if(Char.IsControl(c))throw new InvalidDataException("Invalid player name");
            return name;
        }
        public static void Configure(TcpClient socket) {
            socket.NoDelay=true;socket.ReceiveTimeout=5000;socket.SendTimeout=500;
        }
    }
    public sealed class CoopHost : IDisposable {
        public const int DefaultPort=27922;
        sealed class Peer { public byte Id; public string Name; public TcpClient Socket; public PlayerInput Input; public GameFrame Frame; public bool Ready; }
        // Players present when the campaign started; only they may rejoin after a dropped connection.
        readonly Dictionary<byte,string> roster=new Dictionary<byte,string>();
        readonly object gate=new object();
        readonly Dictionary<byte,Peer> peers=new Dictionary<byte,Peer>();
        readonly HashSet<uint> collected=new HashSet<uint>();
        readonly string hostName;
        readonly int maximumPlayers;
        readonly TcpListener listener;
        readonly HashSet<TcpClient> sockets=new HashSet<TcpClient>();
        volatile bool disposed;
        int revision,coins,level=1;
        bool started;
        public int Port {get{return ((IPEndPoint)listener.LocalEndpoint).Port;}}
        public CoopHost(string name,IPAddress bindAddress,int port,int maximumPlayers=4) {
            if(maximumPlayers<2||maximumPlayers>4)throw new ArgumentOutOfRangeException("maximumPlayers");
            this.maximumPlayers=maximumPlayers;
            hostName=Wire.Name(name);listener=new TcpListener(bindAddress,port);listener.Start(4);
            new Thread(Accept){IsBackground=true,Name="Coop accept"}.Start();
        }
        public LobbyState Snapshot() {lock(gate)return SnapshotLocked();}
        LobbyState SnapshotLocked() {
            var s=new LobbyState{Revision=revision,Coins=coins,Level=level,Started=started};s.Players[0]=hostName;
            foreach(var p in peers.Values)s.Players[p.Id]=p.Name;return s;
        }
        void Accept() {
            while(!disposed)try {
                TcpClient socket=listener.AcceptTcpClient();Wire.Configure(socket);
                lock(gate){if(sockets.Count>=4||disposed){socket.Close();continue;}sockets.Add(socket);}
                new Thread(()=>Serve(socket)){IsBackground=true,Name="Coop peer"}.Start();
            } catch(SocketException){if(!disposed)Thread.Sleep(20);}catch(ObjectDisposedException){break;}
        }
        void Serve(TcpClient socket) {
            Peer peer=null;
            try {
                NetworkStream stream=socket.GetStream();string name;
                using(var r=Wire.Read(stream)){
                    if(r.ReadByte()!=Wire.Hello)throw new InvalidDataException("Hello required");
                    name=Wire.Name(r.ReadString());Wire.End(r);
                }
                bool rejoin=false;
                lock(gate){
                    if(started){
                        byte slot=0;foreach(var r in roster)if(r.Value==name)slot=r.Key;
                        if(slot==0)throw new InvalidDataException("Campaign already started");
                        Peer stale;if(peers.TryGetValue(slot,out stale)){peers.Remove(slot);stale.Socket.Close();}   // its old connection is dead
                        peer=new Peer{Id=slot,Name=name,Socket=socket};peers.Add(slot,peer);revision++;rejoin=true;
                    }else{
                        if(peers.Count>=maximumPlayers-1)throw new InvalidDataException("Lobby full ("+maximumPlayers+" players maximum)");
                        byte id=1;while(peers.ContainsKey(id))id++;
                        peer=new Peer{Id=id,Name=name,Socket=socket,Ready=true};peers.Add(id,peer);revision++;
                    }
                }
                // A rejoining player receives the started lobby state before any game frame.
                if(rejoin){SendState(peer,Snapshot());lock(gate)peer.Ready=true;}
                Publish();
                while(!disposed){using(var r=Wire.Read(stream)){
                    byte type=r.ReadByte();
                    if(type==Wire.Input){
                        var input=new PlayerInput{Sequence=r.ReadUInt32(),Buttons=r.ReadUInt16(),Yaw=r.ReadUInt16(),Pitch=r.ReadInt16()};
                        if(input.Yaw>=4096||input.Pitch < -1024||input.Pitch>1024)throw new InvalidDataException("Invalid input angles");
                        Wire.End(r);
                        lock(gate){if(started&&(peer.Input==null||unchecked((int)(input.Sequence-peer.Input.Sequence))>0))peer.Input=input;}
                    }else if(type==Wire.Frame){
                        GameFrame frame=GameFrame.Read(r);Wire.End(r);
                        lock(gate){if(started&&(peer.Frame==null||unchecked((int)(frame.Sequence-peer.Frame.Sequence))>0))peer.Frame=frame;}
                    }else if(type==Wire.Heartbeat){Wire.End(r);SendState(peer,Snapshot());}
                    else throw new InvalidDataException("Clients may only send input, game frames or heartbeat");
                }}
            }catch(Exception e){
                try{Wire.Send(socket.GetStream(),Wire.Packet(Wire.Reject,w=>w.Write(e.Message)));}catch{}
            }finally{
                lock(gate){sockets.Remove(socket);Peer current;if(peer!=null&&peers.TryGetValue(peer.Id,out current)&&current==peer){peers.Remove(peer.Id);revision++;}}
                socket.Close();if(peer!=null&&!disposed)Publish();
            }
        }
        static void SendState(Peer peer,LobbyState s) {
            Wire.Send(peer.Socket.GetStream(),Wire.Packet(Wire.State,w=>{
                w.Write(peer.Id);w.Write(s.Revision);w.Write(s.Started);w.Write(s.Level);w.Write(s.Coins);w.Write((byte)s.Players.Count);
                foreach(var p in s.Players){w.Write(p.Key);w.Write(p.Value);}
            }));
        }
        void Publish() {
            Peer[] copy;LobbyState state;lock(gate){copy=new List<Peer>(peers.Values).FindAll(p=>p.Ready).ToArray();state=SnapshotLocked();}
            foreach(var p in copy)try{SendState(p,state);}catch{p.Socket.Close();}
        }
        public int ConnectedPeers {get{lock(gate)return new List<Peer>(peers.Values).FindAll(p=>p.Ready).Count;}}
        public bool StartCampaign(int selectedLevel) {
            if(selectedLevel<1||selectedLevel>15)throw new ArgumentOutOfRangeException("selectedLevel");
            lock(gate){if(started||peers.Count<1)return false;started=true;level=selectedLevel;revision++;foreach(var p in peers.Values)roster[p.Id]=p.Name;}
            Publish();return true;
        }
        // Called only by the host simulation AFTER collision/pickup validation.
        // Clients have no packet capable of awarding coins or claiming pickups.
        public bool CollectCoins(uint pickupId,int amount) {
            if(amount<1||amount>100)throw new ArgumentOutOfRangeException("amount");
            lock(gate){if(!started||collected.Contains(pickupId))return false;
                if(coins>Int32.MaxValue-amount)throw new OverflowException();
                collected.Add(pickupId);coins+=amount;revision++;}
            Publish();return true;
        }
        public PlayerInput LatestInput(byte player) {
            lock(gate){Peer p;if(!peers.TryGetValue(player,out p)||p.Input==null)return null;
                return new PlayerInput{Sequence=p.Input.Sequence,Buttons=p.Input.Buttons,Yaw=p.Input.Yaw,Pitch=p.Input.Pitch};}
        }
        public GameFrame LatestFrame(byte player){lock(gate){Peer p;return peers.TryGetValue(player,out p)?p.Frame:null;}}
        // Prototype peer-owned player simulation. This is NOT a validated,
        // host-authoritative movement/physics implementation.
        public void PublishFrame(GameFrame frame){
            if(frame==null)throw new ArgumentNullException("frame");
            Peer[] copy;lock(gate){if(!started)return;copy=new List<Peer>(peers.Values).FindAll(p=>p.Ready).ToArray();}
            byte[] packet=Wire.Packet(Wire.Frame,w=>frame.Write(w));
            foreach(var peer in copy)try{Wire.Send(peer.Socket.GetStream(),packet);}catch{peer.Socket.Close();}
        }
        public void Dispose() {disposed=true;listener.Stop();lock(gate){foreach(var p in sockets)p.Close();}}
    }
    public sealed class CoopClient : IDisposable {
        readonly TcpClient socket;
        readonly object gate=new object();
        LobbyState state;
        GameFrame frame;
        volatile bool disposed;
        public string Error {get;private set;}
        public CoopClient(string address,int port,string name) {
            IPAddress ip;if(!IPAddress.TryParse(address,out ip))throw new ArgumentException("Enter the host's IP address");
            Wire.Name(name);socket=new TcpClient(ip.AddressFamily);Wire.Configure(socket);
            var attempt=socket.BeginConnect(ip,port,null,null);
            using(attempt.AsyncWaitHandle){if(!attempt.AsyncWaitHandle.WaitOne(5000)){socket.Close();throw new IOException("Host connection timed out");}}
            socket.EndConnect(attempt);Wire.Send(socket.GetStream(),Wire.Packet(Wire.Hello,w=>w.Write(name)));
            new Thread(Read){IsBackground=true,Name="Coop receive"}.Start();
            new Thread(Heartbeat){IsBackground=true,Name="Coop heartbeat"}.Start();
        }
        public LobbyState Snapshot(){lock(gate){if(state==null)return null;
            var s=new LobbyState{Revision=state.Revision,LocalPlayer=state.LocalPlayer,Coins=state.Coins,Level=state.Level,Started=state.Started};
            foreach(var p in state.Players)s.Players[p.Key]=p.Value;return s;}}
        void Read(){try{while(!disposed)using(var r=Wire.Read(socket.GetStream())){
            byte type=r.ReadByte();if(type==Wire.Reject){Error=r.ReadString();break;}
            if(type==Wire.Frame){var next=GameFrame.Read(r);Wire.End(r);lock(gate){if(state==null||!state.Started)throw new InvalidDataException("Frame before start");if(frame==null||unchecked((int)(next.Sequence-frame.Sequence))>0)frame=next;}continue;}
            if(type!=Wire.State)throw new InvalidDataException("Unexpected server packet");
            var s=new LobbyState{LocalPlayer=r.ReadByte(),Revision=r.ReadInt32(),Started=r.ReadBoolean(),Level=r.ReadInt32(),Coins=r.ReadInt32()};
            byte count=r.ReadByte();if(count<1||count>4||s.LocalPlayer>3||s.Level<1||s.Level>15||s.Coins<0)throw new InvalidDataException("Invalid lobby state");
            for(int i=0;i<count;i++){byte id=r.ReadByte();if(id>3||s.Players.ContainsKey(id))throw new InvalidDataException("Invalid player ID");s.Players.Add(id,Wire.Name(r.ReadString()));}
            Wire.End(r);if(!s.Players.ContainsKey(0)||!s.Players.ContainsKey(s.LocalPlayer))throw new InvalidDataException("Missing player");
            lock(gate){if(state==null||s.Revision>=state.Revision)state=s;}
        }}catch(Exception e){if(!disposed||Error==null)Error=e.Message;}finally{if(Error==null)Error="Connection closed";Dispose();}}
        void Heartbeat(){while(!disposed){Thread.Sleep(1000);if(disposed)break;try{Wire.Send(socket.GetStream(),Wire.Packet(Wire.Heartbeat,w=>{}));}catch(Exception e){Fail(e.Message);}}}
        // Marks the connection lost (Error is never null afterwards) and closes it.
        public void Fail(string reason){if(Error==null)Error=String.IsNullOrEmpty(reason)?"Connection lost":reason;Dispose();}
        public void SendInput(PlayerInput input){Wire.Send(socket.GetStream(),Wire.Packet(Wire.Input,w=>{w.Write(input.Sequence);w.Write(input.Buttons);w.Write(input.Yaw);w.Write(input.Pitch);}));}
        public GameFrame LatestFrame(){lock(gate)return frame;}
        public void SendFrame(GameFrame frame){if(frame==null)throw new ArgumentNullException("frame");Wire.Send(socket.GetStream(),Wire.Packet(Wire.Frame,w=>frame.Write(w)));}
        public void Dispose(){disposed=true;socket.Close();}
    }
}
