using System;
using System.IO;
using System.Text.RegularExpressions;

// Temporary runtime copies: no writes to installed settings, saves or assets.
sealed class LocalTestFiles : IDisposable {
    public readonly string DirectoryPath;
    public readonly string[] Players=new string[2];
    public LocalTestFiles(string source) {
        DirectoryPath=Path.Combine(Path.GetTempPath(),"Toy2-local-coop-"+Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(DirectoryPath);
        try {
            string manifest=Path.Combine(source,"data","runtime-files.sha256");
            string[] lines=File.ReadAllLines(manifest);
            for(int i=0;i<2;i++) {
                Players[i]=Path.Combine(DirectoryPath,"Player"+(i+1));Directory.CreateDirectory(Players[i]);
                foreach(string line in lines) {
                    if(String.IsNullOrWhiteSpace(line))continue;
                    int separator=line.IndexOf("  ",StringComparison.Ordinal);
                    if(separator!=64)throw new IOException("Invalid runtime manifest; repair the installation first.");
                    string relative=line.Substring(separator+2);
                    string from=Contained(source,relative),to=Contained(Players[i],relative);
                    Directory.CreateDirectory(Path.GetDirectoryName(to));File.Copy(from,to,false);
                }
                string config=Path.Combine(Players[i],"dgVoodoo.conf");
                File.WriteAllText(config,Windowed(File.ReadAllText(config)));
            }
        }catch{Dispose();throw;}
    }
    internal static string Contained(string root,string relative) {
        string prefix=Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar;
        string full=Path.GetFullPath(Path.Combine(prefix,relative));
        if(!full.StartsWith(prefix,StringComparison.OrdinalIgnoreCase))throw new IOException("Manifest path escapes game directory.");
        return full;
    }
    internal static string Windowed(string text) {
        foreach(string key in new[]{"Resolution","ExtraEnumeratedResolutions"})text=Set(text,key,"960x540");
        text=Set(text,"FullScreenMode","false");text=Set(text,"WindowedAttributes","");
        text=Set(text,"CenterAppWindow","false");text=Set(text,"CaptureMouse","false");
        return text;
    }
    static string Set(string text,string key,string value) {
        return Regex.Replace(text,@"(?m)^([ \t]*"+Regex.Escape(key)+@"[ \t]*=)[^\r\n]*",m=>m.Groups[1].Value+" "+value);
    }
    static void CheckNoLinks(string path) {
        if((File.GetAttributes(path)&FileAttributes.ReparsePoint)!=0)throw new IOException("Refusing linked test path.");
        if(System.IO.Directory.Exists(path))foreach(string entry in System.IO.Directory.GetFileSystemEntries(path))CheckNoLinks(entry);
    }
    public void Dispose() {
        if(!System.IO.Directory.Exists(DirectoryPath))return;
        // This object only owns its randomly generated, direct temp child.
        string parent=Path.GetFullPath(Path.GetTempPath()).TrimEnd(Path.DirectorySeparatorChar);
        if(!String.Equals(Path.GetDirectoryName(DirectoryPath),parent,StringComparison.OrdinalIgnoreCase)||
           !Path.GetFileName(DirectoryPath).StartsWith("Toy2-local-coop-",StringComparison.Ordinal))throw new IOException("Invalid cleanup directory.");
        CheckNoLinks(DirectoryPath);
        System.IO.Directory.Delete(DirectoryPath,true);
    }
}
