using System;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;
using System.Net;
using System.Web.Script.Serialization;
using System.Reflection;
using System.Windows.Forms;

[assembly: AssemblyVersion("2026.11.7.5")]
static class PortableInstall {
    static readonly string Preferences=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"ToyStory2 Remaster");
    public static string Origin;
    public static Settings LoadPreferences() {
        string path=Path.Combine(Preferences,"settings.json");
        if(File.Exists(path))try{
            var s=new JavaScriptSerializer().Deserialize<Settings>(File.ReadAllText(path));
            if(s!=null&&Array.IndexOf(Configuration.Resolutions,s.Resolution)>=0&&s.Music>=0&&s.Music<=100&&s.Sfx>=0&&s.Sfx<=100&&s.Sensitivity>=1&&s.Sensitivity<=10&&s.AimSensitivity>=1&&s.AimSensitivity<=10)return s;
        }catch{}
        return new Settings();
    }
    public static void SavePreferences(Settings settings) {
        Directory.CreateDirectory(Preferences);
        File.WriteAllText(Path.Combine(Preferences,"settings.json"),new JavaScriptSerializer().Serialize(settings));
    }
    public static string DefaultRoot(string origin) {
        string saved=Path.Combine(Preferences,"install-path.txt");
        if(File.Exists(saved))return Path.GetFullPath(File.ReadAllText(saved).Trim());
        string beside=Path.GetDirectoryName(origin);
        if(File.Exists(Path.Combine(beside,"version.txt"))&&File.Exists(Path.Combine(beside,"toy2.exe")))return beside;
        return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Games","Toy Story 2");
    }
    public static void Remember(string root) {
        Directory.CreateDirectory(Preferences);
        File.WriteAllText(Path.Combine(Preferences,"install-path.txt"),Path.GetFullPath(root));
    }
    public static bool Bootstrap(string[] args) {
        int marker=Array.IndexOf(args,"--portable-origin");
        if(marker>=0&&marker+1<args.Length){Origin=Path.GetFullPath(args[marker+1]);return false;}
        Origin=Application.ExecutablePath;
        string source=Origin,installed=Path.Combine(DefaultRoot(Origin),"Toy Story 2 Launcher.exe");
        if(File.Exists(installed)&&AssemblyName.GetAssemblyName(installed).Version>Assembly.GetExecutingAssembly().GetName().Version)source=installed;
        // Run outside the install directory so updates can replace its launcher.
        // Keep a reusable, hash-named cache; remove only obsolete unlocked copies.
        string cache=Path.Combine(Preferences,"launcher-cache");Directory.CreateDirectory(cache);
        string copy=Path.Combine(cache,UpdateWindow.Hash(source)+".exe");
        if(!File.Exists(copy))File.Copy(source,copy);
        foreach(string old in Directory.GetFiles(cache,"*.exe"))if(old!=copy)try{File.Delete(old);}catch(IOException){}catch(UnauthorizedAccessException){}
        Process.Start(new ProcessStartInfo(copy,"--portable-origin \""+Origin+"\""+(Array.IndexOf(args,"--multiplayer")>=0?" --multiplayer":"")){UseShellExecute=false});
        return true;
    }
    public static bool Healthy(string root) {
        string manifest=Path.Combine(root,"data","runtime-files.sha256");
        if(!File.Exists(manifest))return false;
        var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach(string line in File.ReadAllLines(manifest)) {
            int split=line.IndexOf("  ",StringComparison.Ordinal);
            if(split!=64)return false;
            string relative=line.Substring(split+2);
            if(!UpdateWindow.Allowed(relative)||!seen.Add(relative))return false;
            string file=Path.Combine(root,relative);
            if(!File.Exists(file))return false;
            if(!UpdateWindow.Preserve(relative)&&!String.Equals(UpdateWindow.Hash(file),line.Substring(0,64),StringComparison.OrdinalIgnoreCase))return false;
        }
        return seen.Contains("toy2.exe")&&seen.Contains("Toy Story 2 Launcher.exe")&&seen.Contains("scripts\\ModernControls.asi")&&seen.Contains("version.txt");
    }
    public static void ValidateFolder(string root) {
        root=Path.GetFullPath(root);
        if(root.TrimEnd('\\')==Path.GetPathRoot(root).TrimEnd('\\'))throw new IOException("Choose a game folder, not the root of a drive.");
        for(string p=root;p!=null;p=Path.GetDirectoryName(p))
            if(Directory.Exists(p)&&(File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException("Choose a folder without symbolic links or junctions.");
    }
    public static void Ensure(string root,Action<string> say) {
        ValidateFolder(root);Directory.CreateDirectory(root);
        string stage=null;
        using(var gate=new FileStream(Path.Combine(root,".update-lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None,1,FileOptions.DeleteOnClose))try {
            if(Process.GetProcessesByName("toy2").Length>0||Process.GetProcessesByName("Toy Story 2 Co-op").Length>0)throw new IOException("Close the game and co-op lobby before checking or changing game files.");
            ServicePointManager.SecurityProtocol=(SecurityProtocolType)3072;
            say("Checking for updates…");
            var release=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(UpdateWindow.Fetch("https://api.github.com/repos/gamezone047/ToyStory2-remaster/releases/latest"));
            Version latest;if(!Version.TryParse(((string)release["tag_name"]).TrimStart('v'),out latest))throw new InvalidDataException("Invalid release version.");
            Version current=null;string version=Path.Combine(root,"version.txt");if(File.Exists(version))Version.TryParse(File.ReadAllText(version).Trim(),out current);
            say("Checking game files…");
            if(current!=null&&current>=latest&&Healthy(root))return;
            if(current!=null&&current>latest)throw new IOException("This installation is newer than the published release. Publish its matching build before repairing it.");
            release=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(UpdateWindow.Fetch("https://api.github.com/repos/gamezone047/ToyStory2-remaster-files/releases/tags/v"+latest));
            string filename="ToyStory2-remaster-"+latest+".zip",zipUrl=null,hashUrl=null;
            foreach(object item in (System.Collections.IEnumerable)release["assets"]){var a=(Dictionary<string,object>)item;string n=(string)a["name"];if(n==filename)zipUrl=(string)a["browser_download_url"];if(n==filename+".sha256")hashUrl=(string)a["browser_download_url"];}
            if(zipUrl==null||hashUrl==null)throw new InvalidDataException("The published build is missing its download or checksum.");
            UpdateWindow.CheckUrl(zipUrl);UpdateWindow.CheckUrl(hashUrl);
            string expected=UpdateWindow.Fetch(hashUrl).Trim().Split(new[]{' ','\t','\r','\n'},StringSplitOptions.RemoveEmptyEntries)[0];
            if(!System.Text.RegularExpressions.Regex.IsMatch(expected,"\\A[0-9a-fA-F]{64}\\z"))throw new InvalidDataException("Invalid download checksum.");
            stage=Path.Combine(Path.GetTempPath(),"Toy2-install-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(stage);
            string zip=Path.Combine(stage,"game.zip");say("Downloading version "+latest+"…");UpdateWindow.Download(zipUrl,zip,say);
            if(!String.Equals(expected,UpdateWindow.Hash(zip),StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("Download verification failed. Please retry.");
            string unpack=Path.Combine(stage,"unpacked");UpdateWindow.Extract(zip,unpack);
            if(File.ReadAllText(Path.Combine(unpack,"version.txt")).Trim()!=latest.ToString())throw new InvalidDataException("Package version mismatch.");
            if(!Healthy(unpack))throw new InvalidDataException("This release does not contain a complete file manifest. Publish the new launcher build first.");
            say("Installing game files… Saves and settings are preserved.");UpdateWindow.Install(unpack,root,-1);
            if(!Healthy(root))throw new IOException("Some game files could not be verified. Please retry.");
        } finally {if(stage!=null)try{Directory.Delete(stage,true);}catch(IOException){}catch(UnauthorizedAccessException){}}
    }
}


