using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net;
using System.Security.Cryptography;
using System.Threading;
using System.Windows.Forms;
using Toy2Coop;

sealed class CoopLauncher : Form {
 readonly string root;
 readonly int testSlot;
 readonly TextBox address=new TextBox(),name=new TextBox(),publicIp=new TextBox();
 readonly Label networkStatus=new Label(),status=new Label(),players=new Label();
 readonly Button hostButton=new Button(),joinButton=new Button(),start=new Button(),localTest=new Button();
 readonly System.Windows.Forms.Timer timer=new System.Windows.Forms.Timer();
 CoopHost host; CoopClient client; NativeBridge bridge; Process game;
 Thread pump; volatile bool closing; volatile string pumpError,pumpStatus;
 bool launched,connecting,configuring,autoStart; uint previousSequence; bool haveSequence;
 LocalTestFiles localFiles; CoopLauncher testHost,testClient; int children;
 public CoopLauncher(string runtimeRoot=null,int slot=0) {
  root=runtimeRoot??AppDomain.CurrentDomain.BaseDirectory;testSlot=slot;
  Text="Toy Story 2 - Multiplayer (experimental)";ClientSize=new Size(640,700);
  Font=new Font("Segoe UI",10);FormBorderStyle=FormBorderStyle.FixedDialog;MaximizeBox=false;
  AddLabel("MULTIPLAYER - TWO PLAYERS",24,18,590,30).Font=new Font(Font.FontFamily,16,FontStyle.Bold);
  AddLabel("Experimental first-level co-op: shared pickups, enemy movement, objects and combat.\nCampaign-wide synchronization is still in development.",24,58,590,55);
  AddLabel("Your name",24,123,150,28);name.SetBounds(180,123,220,28);name.Text="Buzz";name.MaxLength=24;Controls.Add(name);
  AddLabel("Host IP address",24,163,150,28);address.SetBounds(180,163,220,28);address.Text="127.0.0.1";address.MaxLength=255;Controls.Add(address);
  var paste=new Button();SetupButton(paste,"Paste IP",410,158,105,delegate{try{IPAddress ip;if(!IPAddress.TryParse(Clipboard.GetText().Trim(),out ip))throw new Exception("Copy a valid IP address first.");address.Text=ip.ToString();}catch(Exception e){MessageBox.Show(this,e.Message);}});
  SetupButton(hostButton,"Host",24,211,130,delegate{Connect(true);});SetupButton(joinButton,"Join",170,211,130,delegate{Connect(false);});SetupButton(start,"Start game",410,211,200,delegate{StartGame();});start.Enabled=false;
  players.SetBounds(24,265,590,45);Controls.Add(players);status.SetBounds(24,318,590,70);Controls.Add(status);status.Text="Host creates a lobby; the second player joins its IP address.";
  AddLabel("TCP port 27922 must be reachable on the host. Keep this lobby open.\nStart game loads Andy's House on both PCs.",24,398,590,55);
  AddLabel("Your public IPv4",24,466,150,28);publicIp.SetBounds(180,463,220,28);publicIp.ReadOnly=true;Controls.Add(publicIp);
  var copy=new Button();SetupButton(copy,"Copy IP",410,458,105,delegate{FetchPublicIp(true);});
  var setup=new Button();SetupButton(setup,"Set up hosting",24,505,165,delegate{ConfigureNetwork();});
  networkStatus.SetBounds(200,505,410,115);networkStatus.Text="Use Set up hosting once if needed (administrator approval).\nNormal Host/Join and game launch do not request elevation.";Controls.Add(networkStatus);
  SetupButton(localTest,"Test two players on this PC",24,635,320,delegate{StartLocalTest();});localTest.Visible=slot==0;
  if(slot==0){string[] saved=CoopPreferences.Load(CoopPreferences.FilePath);if(saved[0]!=null)name.Text=saved[0];if(saved[1]!=null)address.Text=saved[1];}
  timer.Interval=200;timer.Tick+=delegate{RefreshSession();};timer.Start();
  FormClosing+=delegate{closing=true;timer.Stop();if(testHost!=null&&!testHost.IsDisposed)testHost.Close();if(testClient!=null&&!testClient.IsDisposed)testClient.Close();if(testSlot!=0)StopOwnedGame();if(client!=null)client.Dispose();if(host!=null)host.Dispose();if(testSlot==0)try{CoopPreferences.Save(CoopPreferences.FilePath,name.Text,address.Text);}catch(Exception e){networkStatus.Text=e.Message;}};
  FormClosed+=delegate{if(pump!=null)pump.Join(1500);if(bridge!=null&&(pump==null||!pump.IsAlive))bridge.Dispose();CleanupLocalTest();};
 }
 Label AddLabel(string text,int x,int y,int w,int h){var l=new Label{Text=text};l.SetBounds(x,y,w,h);Controls.Add(l);return l;}
 void SetupButton(Button b,string text,int x,int y,int width,EventHandler action){b.Text=text;b.SetBounds(x,y,width,38);b.Click+=action;Controls.Add(b);}
 void Connect(bool asHost){
  if(connecting||host!=null||client!=null)return;string player=name.Text.Trim(),ip=address.Text.Trim();connecting=true;hostButton.Enabled=joinButton.Enabled=localTest.Enabled=false;status.Text=asHost?"Creating lobby...":"Connecting...";
  ThreadPool.QueueUserWorkItem(delegate{CoopHost h=null;CoopClient c=null;Exception failure=null;try{if(asHost)h=new CoopHost(player,IPAddress.Any,CoopHost.DefaultPort,2);else c=new CoopClient(ip,CoopHost.DefaultPort,player);}catch(Exception e){failure=e;}
   if(closing){if(h!=null)h.Dispose();if(c!=null)c.Dispose();return;}
   try{BeginInvoke((Action)delegate{connecting=false;if(failure!=null){status.Text=failure.Message;hostButton.Enabled=joinButton.Enabled=localTest.Enabled=true;return;}host=h;client=c;name.Enabled=address.Enabled=false;if(asHost)FetchPublicIp(false);status.Text=asHost?"Lobby open. Waiting for the second player on TCP 27922.":"Joined. Waiting for the host to start.";});}catch(InvalidOperationException){if(h!=null)h.Dispose();if(c!=null)c.Dispose();}
  });
 }
 void FetchPublicIp(bool copy){ThreadPool.QueueUserWorkItem(delegate{string result=null,error=null;try{ServicePointManager.SecurityProtocol=(SecurityProtocolType)3072;var request=WebRequest.Create("https://api.ipify.org");request.Timeout=10000;using(var response=request.GetResponse())using(var reader=new StreamReader(response.GetResponseStream()))result=reader.ReadToEnd().Trim();IPAddress ip;if(!IPAddress.TryParse(result,out ip)||ip.AddressFamily!=System.Net.Sockets.AddressFamily.InterNetwork)throw new Exception("Public IPv4 unavailable.");}catch(Exception e){error=e.Message;}if(closing)return;try{BeginInvoke((Action)delegate{if(error!=null){networkStatus.Text=error;return;}publicIp.Text=result;if(copy)try{Clipboard.SetText(result);}catch(Exception e){networkStatus.Text=e.Message;}});}catch(InvalidOperationException){}});}
 void ConfigureNetwork(){if(configuring)return;configuring=true;networkStatus.Text="Configuring hosting...";ThreadPool.QueueUserWorkItem(delegate{string report=Path.Combine(Path.GetTempPath(),"Toy2-hosting-"+Guid.NewGuid().ToString("N")+".txt"),result;try{var info=new ProcessStartInfo("powershell.exe","-NoProfile -NonInteractive -ExecutionPolicy Bypass -File \""+Path.Combine(root,"NetworkSetup.ps1")+"\" -ReportPath \""+report+"\""){UseShellExecute=true,Verb="runas",WindowStyle=ProcessWindowStyle.Hidden};using(var p=Process.Start(info)){p.WaitForExit();result=p.ExitCode==0?"Hosting setup finished. Verify with an external player.":"Hosting setup failed.";}if(File.Exists(report))foreach(string line in File.ReadAllLines(report))if(line.StartsWith("Firewall:")||line.StartsWith("Router:"))result+="\n"+line;}catch(Exception e){result=e.Message;}finally{try{File.Delete(report);}catch{}}if(closing)return;try{BeginInvoke((Action)delegate{configuring=false;networkStatus.Text=result;});}catch(InvalidOperationException){}});}
 void StartGame(){if(host!=null&&!host.StartCampaign(1))status.Text="Two players are required to start.";}
 void RefreshSession(){if(closing)return;if(testSlot!=0&&game!=null&&game.HasExited){Close();return;}LobbyState s=host!=null?host.Snapshot():client!=null?client.Snapshot():null;if(s!=null){var lines=new System.Collections.Generic.List<string>();foreach(var p in s.Players)lines.Add((p.Key==0?"Host: ":"Player 2: ")+p.Value);players.Text=String.Join("\n",lines);start.Enabled=host!=null&&!s.Started&&s.Players.Count==2;if(autoStart&&start.Enabled){autoStart=false;StartGame();}if(s.Started&&!launched)Launch();}if(pumpError!=null)status.Text=pumpError;else if(client!=null&&client.Error!=null)status.Text="Disconnected: "+client.Error;else if(launched&&game!=null&&game.HasExited){status.Text="Game closed. Reopen this lobby to play again.";if(testSlot!=0)Close();}else if(pumpStatus!=null)status.Text=pumpStatus;}
 void VerifyFiles(){string manifest=Path.Combine(root,"Coop-build.sha256");if(!File.Exists(manifest))throw new IOException("Co-op build manifest missing. Repair the installation first.");foreach(string line in File.ReadAllLines(manifest)){if(String.IsNullOrWhiteSpace(line))continue;int split=line.IndexOf("  ",StringComparison.Ordinal);if(split!=64)throw new IOException("Invalid co-op manifest.");string relative=line.Substring(split+2),path=LocalTestFiles.Contained(root,relative);using(var sha=SHA256.Create())using(var stream=File.OpenRead(path)){string hash=BitConverter.ToString(sha.ComputeHash(stream)).Replace("-","");if(!hash.Equals(line.Substring(0,64),StringComparison.OrdinalIgnoreCase))throw new IOException("Build mismatch: "+relative);}}}
 void Launch(){launched=true;start.Enabled=false;try{if(testSlot==0)VerifyFiles();bridge=new NativeBridge();var info=GameLaunch.Create(root);info.EnvironmentVariables["TOY2_COOP_BRIDGE"]=bridge.Name;var s=host!=null?host.Snapshot():client.Snapshot();info.EnvironmentVariables["TOY2_COOP_ROLE"]=host!=null?"host":"client";byte peer=(byte)(host!=null?1:0);if(s.Players.ContainsKey(peer))info.EnvironmentVariables["TOY2_COOP_PEER"]=s.Players[peer];if(testSlot!=0)info.EnvironmentVariables["TOY2_LOCAL_TEST"]=testSlot.ToString();game=Process.Start(info);pump=new Thread(Pump){IsBackground=true,Name="Coop native bridge"};pump.Start();status.Text="Loading Andy's House...";}catch(Exception e){pumpError="Unable to launch: "+e.Message;if(host!=null)host.Dispose();if(client!=null)client.Dispose();}}
 void SessionDiagnostic(string message){try{
  string directory=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"ToyStory2","Diagnostics");Directory.CreateDirectory(directory);
  string file=Path.Combine(directory,(host!=null?"host":"client")+"-launcher.log");
  if(File.Exists(file)&&new FileInfo(file).Length>1024*1024)File.WriteAllText(file,"");
  File.AppendAllText(file,DateTime.UtcNow.ToString("o")+" build=2026.11.7.5 pid="+(game==null?0:game.Id)+" "+message+Environment.NewLine);
 }catch{} }
 void Pump(){bool stalled=false;SessionDiagnostic("Bridge started");try{
  while(!closing&&game!=null&&!game.HasExited){
   bool connected=host!=null?host.Snapshot().Players.Count==2:client.Error==null;
   GameFrame remote=host!=null?host.LatestFrame(1):client.LatestFrame();GameFrame local=bridge.Exchange(remote,connected);
   uint age=unchecked((uint)Environment.TickCount-bridge.GameTick);
   bool nowStalled=bridge.GameTick!=0&&age>10000;
   if(nowStalled!=stalled){stalled=nowStalled;SessionDiagnostic(stalled?"No native game heartbeat for 10 seconds; possible hang":"Native game heartbeat resumed");}
   pumpStatus=stalled?"Game heartbeat stopped. Diagnostics saved.":!connected?"Other player disconnected.":remote==null?"Waiting for the other player to enter the level...":"Co-op connected. Keep this lobby open.";
   if(local!=null&&(!haveSequence||local.Sequence!=previousSequence)){previousSequence=local.Sequence;haveSequence=true;if(host!=null)host.PublishFrame(local);else client.SendFrame(local);}
   Thread.Sleep(33);
  }
 }catch(Exception e){SessionDiagnostic("Bridge exception: "+e);if(!closing)pumpError=e.Message;}
 finally{try{if(game!=null&&game.HasExited)SessionDiagnostic("Game exited with code 0x"+game.ExitCode.ToString("X8"));else SessionDiagnostic("Bridge stopped; closing="+closing);bridge.Exchange(null,false);}catch{}if(host!=null)host.Dispose();if(client!=null)client.Dispose();}}

 void StopOwnedGame(){if(game==null)return;try{if(!game.HasExited){game.CloseMainWindow();if(!game.WaitForExit(1500)){game.Kill();game.WaitForExit(1500);}}}catch(Exception e){status.Text=e.Message;}}
 void StartLocalTest(){if(connecting||localFiles!=null||host!=null||client!=null)return;connecting=true;hostButton.Enabled=joinButton.Enabled=localTest.Enabled=false;status.Text="Preparing two temporary windowed copies. This may take a minute...";
  ThreadPool.QueueUserWorkItem(delegate{LocalTestFiles files=null;Exception failure=null;try{VerifyFiles();files=new LocalTestFiles(root);}catch(Exception e){failure=e;}if(closing){if(files!=null)files.Dispose();return;}try{BeginInvoke((Action)delegate{connecting=false;if(closing){if(files!=null)files.Dispose();return;}if(failure!=null){status.Text=failure.Message;hostButton.Enabled=joinButton.Enabled=localTest.Enabled=true;return;}localFiles=files;try{testHost=new CoopLauncher(files.Players[0],1);testClient=new CoopLauncher(files.Players[1],2);testHost.host=new CoopHost("Player 1",IPAddress.Loopback,0,2);testClient.client=new CoopClient("127.0.0.1",testHost.host.Port,"Player 2");children=2;ConfigureChild(testHost,1);ConfigureChild(testClient,2);testHost.autoStart=true;testHost.Show();testClient.Show();status.Text="Local test running over 127.0.0.1. Alt-Tab between players.\nClose both game windows to finish and remove temporary copies.";}catch(Exception e){status.Text=e.Message;if(testHost!=null){testHost.Close();testHost.Dispose();}if(testClient!=null){testClient.Close();testClient.Dispose();}children=0;CleanupLocalTest();}});}catch(InvalidOperationException){if(files!=null)files.Dispose();}});
 }
 void ConfigureChild(CoopLauncher child,int slot){child.Text="Local co-op - Player "+slot;child.name.Text="Player "+slot;child.address.Text="127.0.0.1";child.name.Enabled=child.address.Enabled=child.hostButton.Enabled=child.joinButton.Enabled=false;child.networkStatus.Text="Local TCP test. No public IP or router configuration required.";child.FormClosed+=delegate{children--;CleanupLocalTest();};}
 void CleanupLocalTest(){if(children>0||localFiles==null)return;try{localFiles.Dispose();localFiles=null;testHost=testClient=null;status.Text="Local test finished. Temporary files removed.";hostButton.Enabled=joinButton.Enabled=localTest.Enabled=true;}catch(Exception e){status.Text="Could not remove temporary test files: "+e.Message;}}
 [STAThread] static void Main(string[] args){Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);if(args.Length==2&&args[0]=="--render-preview"){using(var f=new CoopLauncher())using(var bitmap=new Bitmap(f.Width,f.Height)){f.Opacity=0;f.ShowInTaskbar=false;f.Show();Application.DoEvents();f.DrawToBitmap(bitmap,new Rectangle(0,0,bitmap.Width,bitmap.Height));bitmap.Save(args[1]);f.Close();}return;}if(UpdateGate.HandOff(args))return;Application.Run(new CoopLauncher());}
}
