using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Security.Cryptography;
using System.Threading;
using System.Web.Script.Serialization;
using System.Windows.Forms;

sealed class UpdateWindow : Form {
    readonly Label status=new Label(); readonly string root,target; readonly int parent; bool mayClose;
    public UpdateWindow(string r,string t,int p) {
        root=r;target=t;parent=p;Text="Toy Story 2 — Updates";ClientSize=new Size(510,130);
        FormBorderStyle=FormBorderStyle.FixedDialog;MaximizeBox=false;StartPosition=FormStartPosition.CenterScreen;
        status.SetBounds(20,20,470,95);status.Font=new Font("Segoe UI",11);Controls.Add(status);
        status.Text="Checking GitHub for updates…";
        FormClosing+=delegate(object sender,FormClosingEventArgs e){if(!mayClose)e.Cancel=true;};
        Shown+=delegate { ThreadPool.QueueUserWorkItem(delegate { Work(); }); };
    }
    void Say(string text){if(!IsDisposed)try{BeginInvoke((Action)delegate{status.Text=text;});}catch(InvalidOperationException){}}
    void Work() {
        bool found=false;string stage=null;
        try {
            try { using(var p=Process.GetProcessById(parent)) { if(!p.WaitForExit(15000))throw new IOException("Launcher has not closed."); } } catch(ArgumentException){}
            // A second launcher must not patch files concurrently.
            using(var gate=new FileStream(Path.Combine(root,".update-lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None,1,FileOptions.DeleteOnClose)) {
                ServicePointManager.SecurityProtocol=(SecurityProtocolType)3072;
                Dictionary<string,object> release;
                try { release=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(Fetch("https://api.github.com/repos/gamezone047/ToyStory2-remaster/releases/latest")); }
                catch(WebException e) { var response=e.Response as HttpWebResponse;if(response!=null&&response.StatusCode==HttpStatusCode.NotFound){Finish(true);return;}throw; }
                Version latest; if(!Version.TryParse(((string)release["tag_name"]).TrimStart('v'),out latest))throw new InvalidDataException("Release version is invalid.");
                Version current; if(!Version.TryParse(File.ReadAllText(Path.Combine(root,"version.txt")).Trim(),out current))throw new InvalidDataException("Installed version is invalid.");
                if(latest<=current){Finish(true);return;}
                found=true;
                if(Process.GetProcessesByName("toy2").Length>0||Process.GetProcessesByName("Toy Story 2 Co-op").Length>0||Process.GetProcessesByName("Toy Story 2 Launcher").Length>0)
                    throw new IOException("Close the game and all other launchers, then reopen this launcher to update.");
                release=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(Fetch("https://api.github.com/repos/gamezone047/ToyStory2-remaster-files/releases/tags/v"+latest));
                string filename="ToyStory2-remaster-"+latest+".zip",zipUrl=null,hashUrl=null;
                foreach(object item in (System.Collections.IEnumerable)release["assets"]){var a=(Dictionary<string,object>)item;string n=(string)a["name"];if(n==filename)zipUrl=(string)a["browser_download_url"];if(n==filename+".sha256")hashUrl=(string)a["browser_download_url"];}
                if(zipUrl==null||hashUrl==null)throw new InvalidDataException("New release is missing its ZIP or checksum.");
                CheckUrl(zipUrl);CheckUrl(hashUrl);
                string expected=Fetch(hashUrl).Trim().Split(new[]{' ','\t','\r','\n'},StringSplitOptions.RemoveEmptyEntries)[0];
                if(expected.Length!=64||!System.Text.RegularExpressions.Regex.IsMatch(expected,"\\A[0-9a-fA-F]{64}\\z"))throw new InvalidDataException("Invalid release checksum.");
                stage=Path.Combine(Path.GetTempPath(),"Toy2-package-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(stage);
                string zip=Path.Combine(stage,"release.zip");Say("Downloading version "+latest+"…");
                Download(zipUrl,zip,Say);
                if(!String.Equals(Hash(zip),expected,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("Download checksum failed. Installed game was not changed.");
                string unpack=Path.Combine(stage,"unpacked");Extract(zip,unpack);
                if(File.ReadAllText(Path.Combine(unpack,"version.txt")).Trim()!=latest.ToString())throw new InvalidDataException("Package version mismatch.");
                Say("Installing version "+latest+"… Your saves and settings are preserved.");
                Install(unpack,root,-1);Say("Update installed.");
            }
            Finish(true);
        } catch(Exception e) {
            if(IsDisposed)return;
            try { Invoke((Action)delegate {
                mayClose=true;
                if(found){MessageBox.Show(this,"Update was not installed. "+e.Message+"\nReopen the launcher to retry.",Text,MessageBoxButtons.OK,MessageBoxIcon.Error);Close();}
                else {var answer=MessageBox.Show(this,"Could not check for updates: "+e.Message+"\nContinue with the installed version offline?",Text,MessageBoxButtons.YesNo,MessageBoxIcon.Warning);if(answer==DialogResult.Yes)OpenLauncher();Close();}
            }); }catch(InvalidOperationException){}
        } finally { if(stage!=null)try{Directory.Delete(stage,true);}catch{} }
    }
    void Finish(bool open){if(!IsDisposed)try{Invoke((Action)delegate{mayClose=true;if(open)OpenLauncher();Close();});}catch(InvalidOperationException){}}
    void OpenLauncher(){Process.Start(new ProcessStartInfo(Path.Combine(root,target),"--update-checked"){WorkingDirectory=root,UseShellExecute=true});}
    static HttpWebRequest Request(string url){var r=(HttpWebRequest)WebRequest.Create(url);r.UserAgent="ToyStory2-remaster-updater";r.Timeout=15000;r.ReadWriteTimeout=30000;return r;}
    public static string Fetch(string url){using(var response=Request(url).GetResponse())using(var reader=new StreamReader(response.GetResponseStream()))return reader.ReadToEnd();}
    public static void CheckUrl(string url){var u=new Uri(url);if(u.Scheme!="https"||u.Host!="github.com"||!u.AbsolutePath.StartsWith("/gamezone047/ToyStory2-remaster-files/releases/download/",StringComparison.Ordinal))throw new InvalidDataException("Unexpected release URL.");}
    public static void Download(string url,string path,Action<string> say){using(var response=Request(url).GetResponse())using(var input=response.GetResponseStream())using(var output=File.Create(path)){byte[] buffer=new byte[131072];long total=0,last=0;int n;while((n=input.Read(buffer,0,buffer.Length))>0){total+=n;if(total>2147483648L)throw new InvalidDataException("Package too large.");output.Write(buffer,0,n);if(total-last>4*1024*1024){last=total;say("Downloading update… "+total/1048576+" MB"+(response.ContentLength>0?" / "+response.ContentLength/1048576+" MB":""));}}}}
    public static string Hash(string path){using(var s=File.OpenRead(path))using(var h=SHA256.Create())return BitConverter.ToString(h.ComputeHash(s)).Replace("-","");}
    static readonly HashSet<string> RootFiles=new HashSet<string>(StringComparer.OrdinalIgnoreCase){"toy2.exe","toy2.cfg","toy2.ico","toy2.hlp","DDraw.dll","D3DImm.dll","dsound.dll","dsound.ini","dgVoodoo.conf","dgVoodooCpl.exe","Toy Story 2 Launcher.exe","Toy Story 2 Co-op.exe","Toy Story 2 Updater.exe","Coop-build.sha256","runtime-files.sha256","version.txt","Multiplayer.txt","Modern-controls.txt","Escape-menu-fix.txt","Register-Game.ps1","NetworkSetup.ps1"};
    public static bool Allowed(string path){
        if(String.IsNullOrEmpty(path)||path.Contains(":")||path.StartsWith("/")||path.StartsWith("\\"))return false;
        string p=path.Replace('\\','/');foreach(string part in p.Split('/'))if(part==".."||part=="."||part.Length==0||part.EndsWith(".")||part.EndsWith(" "))return false;
        if(Path.GetExtension(p).Equals(".sav",StringComparison.OrdinalIgnoreCase))return false;
        return RootFiles.Contains(p)||p.StartsWith("data/",StringComparison.OrdinalIgnoreCase)||p.StartsWith("DiscData/",StringComparison.OrdinalIgnoreCase)||p.Equals("scripts/ModernControls.asi",StringComparison.OrdinalIgnoreCase)||p.Equals("scripts/CoopBridge.asi",StringComparison.OrdinalIgnoreCase)||p.Equals("scripts/ToyStory2Fix.asi",StringComparison.OrdinalIgnoreCase)||p.Equals("scripts/ModernControls.ini",StringComparison.OrdinalIgnoreCase)||p.Equals("scripts/ToyStory2Fix.ini",StringComparison.OrdinalIgnoreCase);
    }
    public static void Extract(string zip,string destination){
        Directory.CreateDirectory(destination);var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase);long total=0;
        using(var archive=ZipFile.OpenRead(zip))foreach(var entry in archive.Entries){
            string relative=entry.FullName.Replace('/','\\');
            if(!Allowed(relative)||!names.Add(relative))throw new InvalidDataException("Unsafe or duplicate package path: "+relative);
            total+=entry.Length;if(total>2147483648L||entry.Length>1073741824L)throw new InvalidDataException("Expanded package too large.");
            string path=Path.Combine(destination,relative);Directory.CreateDirectory(Path.GetDirectoryName(path));entry.ExtractToFile(path);
        }
        foreach(string required in new[]{"version.txt","toy2.exe","Toy Story 2 Launcher.exe","Toy Story 2 Co-op.exe","Toy Story 2 Updater.exe","scripts\\CoopBridge.asi","Coop-build.sha256"})if(!File.Exists(Path.Combine(destination,required)))throw new InvalidDataException("Incomplete package: "+required);
    }
    public static bool Preserve(string path){string ext=Path.GetExtension(path);return ext.Equals(".ini",StringComparison.OrdinalIgnoreCase)||ext.Equals(".conf",StringComparison.OrdinalIgnoreCase)||ext.Equals(".cfg",StringComparison.OrdinalIgnoreCase);}
    public static void Install(string source,string root,int failAfter){
        string backup=Path.Combine(Path.GetTempPath(),"Toy2-rollback-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(backup);
        var changed=new List<string>();bool recovered=true;
        try {
            var files=new List<string>(Directory.GetFiles(source,"*",SearchOption.AllDirectories));
            files.Sort(delegate(string a,string b){bool av=Path.GetFileName(a)=="version.txt",bv=Path.GetFileName(b)=="version.txt";return av==bv?String.CompareOrdinal(a,b):av?1:-1;});
            foreach(string file in files){string rel=file.Substring(source.TrimEnd('\\').Length+1);if(!Allowed(rel))throw new InvalidDataException("Unexpected file.");string dest=Path.Combine(root,rel);
                // Never follow a junction/symlink within an existing installation.
                for(string check=dest;check!=null&&check.Length>=root.TrimEnd('\\').Length;check=Path.GetDirectoryName(check))if((File.Exists(check)||Directory.Exists(check))&&(File.GetAttributes(check)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked install paths are unsupported: "+check);
                if(File.Exists(dest)&&Preserve(rel))continue;
                string old=Path.Combine(backup,rel);if(File.Exists(dest)){Directory.CreateDirectory(Path.GetDirectoryName(old));File.Copy(dest,old);}
                changed.Add(rel);Directory.CreateDirectory(Path.GetDirectoryName(dest));File.Copy(file,dest,true);
                if(failAfter>=0&&changed.Count>=failAfter)throw new IOException("Injected test failure.");
            }
        } catch {
            for(int i=changed.Count-1;i>=0;i--)try{string rel=changed[i],old=Path.Combine(backup,rel),dest=Path.Combine(root,rel);if(File.Exists(old))File.Copy(old,dest,true);else File.Delete(dest);}catch{recovered=false;}
            if(!recovered)throw new IOException("Rollback needs attention. Previous files retained at "+backup);
            throw;
        } finally {if(recovered)try{Directory.Delete(backup,true);}catch{}}
    }
    [STAThread] static int Main(string[] args){
        if(args.Length==1&&args[0]=="--self-test"){try{SelfTest();return 0;}catch(Exception e){Console.Error.WriteLine(e);return 1;}}
        if(args.Length!=3)return 2;int pid;if(!Int32.TryParse(args[2],out pid))return 2;
        if(args[1]!="Toy Story 2 Launcher.exe"&&args[1]!="Toy Story 2 Co-op.exe")return 2;
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new UpdateWindow(Path.GetFullPath(args[0]),args[1],pid));return 0;
    }
    static void SelfTest(){
        foreach(string bad in new[]{"../evil.exe","data/../../evil.exe","C:/evil.exe","data/file:stream","Toy200.sav","scripts/evil.asi","release/nested.zip","data//x"})if(Allowed(bad))throw new Exception("Path accepted: "+bad);
        string temp=Path.Combine(Path.GetTempPath(),"Toy2-tests-"+Guid.NewGuid().ToString("N"));string root=Path.Combine(temp,"game"),src=Path.Combine(temp,"package");Directory.CreateDirectory(root);Directory.CreateDirectory(src);
        try{File.WriteAllText(Path.Combine(root,"toy2.exe"),"old");File.WriteAllText(Path.Combine(root,"toy2.cfg"),"settings");File.WriteAllText(Path.Combine(root,"Toy200.sav"),"save");File.WriteAllText(Path.Combine(src,"toy2.exe"),"new");File.WriteAllText(Path.Combine(src,"toy2.cfg"),"defaults");File.WriteAllText(Path.Combine(src,"version.txt"),"1.0.0.0");
            bool failed=false;try{Install(src,root,1);}catch(IOException){failed=true;}
            if(!failed||File.ReadAllText(Path.Combine(root,"toy2.exe"))!="old"||File.Exists(Path.Combine(root,"version.txt")))throw new Exception("Rollback failed");
            Install(src,root,-1);if(File.ReadAllText(Path.Combine(root,"toy2.exe"))!="new"||File.ReadAllText(Path.Combine(root,"toy2.cfg"))!="settings"||File.ReadAllText(Path.Combine(root,"Toy200.sav"))!="save")throw new Exception("Install/preservation failed");
            string zip=Path.Combine(temp,"unsafe.zip");using(var z=ZipFile.Open(zip,ZipArchiveMode.Create)){using(var w=new StreamWriter(z.CreateEntry("../escape.exe").Open()))w.Write("bad");}
            failed=false;try{Extract(zip,Path.Combine(temp,"extract"));}catch(InvalidDataException){failed=true;}if(!failed)throw new Exception("ZIP traversal accepted");
        }finally{Directory.Delete(temp,true);}
    }
}


