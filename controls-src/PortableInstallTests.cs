using System;
using System.IO;
using System.Web.Script.Serialization;
static class PortableInstallTests {
    static void Check(bool value,string message){if(!value)throw new Exception(message);}
    static void Manifest(string root) {
        var lines=new System.Collections.Generic.List<string>();
        foreach(string file in Directory.GetFiles(root,"*",SearchOption.AllDirectories)) {
            string relative=file.Substring(root.Length+1);
            if(relative=="data\\runtime-files.sha256"||relative.EndsWith(".sav"))continue;
            lines.Add(UpdateWindow.Hash(file)+"  "+relative);
        }
        Directory.CreateDirectory(Path.Combine(root,"data"));File.WriteAllLines(Path.Combine(root,"data","runtime-files.sha256"),lines);
    }
    static int Main() {
        string temp=Path.Combine(Path.GetTempPath(),"Toy2-portable-test-"+Guid.NewGuid().ToString("N"));
        string source=Path.Combine(temp,"package"),root=Path.Combine(temp,"chosen install folder");
        try {
            UpdateWindow.CheckUrl("https://github.com/gamezone047/ToyStory2-remaster-files/releases/download/v2026.10.23.8/game.zip");
            bool wrongUrl=false;try{UpdateWindow.CheckUrl("https://github.com/other/repo/releases/download/v1/game.zip");}catch(InvalidDataException){wrongUrl=true;}Check(wrongUrl,"Untrusted download repository rejected");
            Directory.CreateDirectory(Path.Combine(source,"scripts"));
            foreach(string file in new[]{"toy2.exe","Toy Story 2 Launcher.exe","scripts\\ModernControls.asi","toy2.cfg","scripts\\ModernControls.ini","version.txt"})File.WriteAllText(Path.Combine(source,file),"packaged "+file);
            Manifest(source);
            Check(!PortableInstall.Healthy(root),"Empty destination detected");
            PortableInstall.ValidateFolder(root);UpdateWindow.Install(source,root,-1);
            Check(PortableInstall.Healthy(root),"Clean installation verified");
            File.WriteAllText(Path.Combine(root,"Toy200.sav"),"player save");File.WriteAllText(Path.Combine(root,"toy2.cfg"),"player settings");
            Check(PortableInstall.Healthy(root),"Player settings allowed");
            File.Delete(Path.Combine(root,"scripts","ModernControls.asi"));Check(!PortableInstall.Healthy(root),"Missing plugin detected");
            UpdateWindow.Install(source,root,-1);Check(PortableInstall.Healthy(root),"Missing plugin repaired");
            File.WriteAllText(Path.Combine(root,"toy2.exe"),"damaged");Check(!PortableInstall.Healthy(root),"Damaged executable detected");
            UpdateWindow.Install(source,root,-1);Check(PortableInstall.Healthy(root),"Damaged executable repaired");
            Check(File.ReadAllText(Path.Combine(root,"Toy200.sav"))=="player save"&&File.ReadAllText(Path.Combine(root,"toy2.cfg"))=="player settings","Saves and settings preserved");
            File.Delete(Path.Combine(root,"toy2.cfg"));Check(!PortableInstall.Healthy(root),"Missing mutable config detected");UpdateWindow.Install(source,root,-1);Check(PortableInstall.Healthy(root),"Missing config restored");
            File.WriteAllText(Path.Combine(root,"data","runtime-files.sha256"),new string('0',64)+"  ..\\outside.exe");Check(!PortableInstall.Healthy(root),"Manifest traversal rejected");
            var settings=new Settings {Music=30,Sensitivity=7};var json=new JavaScriptSerializer();var loaded=json.Deserialize<Settings>(json.Serialize(settings));Check(loaded.Music==30&&loaded.Sensitivity==7,"Portable preferences round trip");
            bool rejected=false;try{PortableInstall.ValidateFolder(Path.GetPathRoot(root));}catch(IOException){rejected=true;}Check(rejected,"Drive root rejected");
            Console.WriteLine("PASS: clean installation, missing/corrupt files, missing config repair, save/settings preservation, unsafe manifest rejection, preference round trip, destination validation. No game launched.");return 0;
        }catch(Exception e){Console.Error.WriteLine(e);return 1;}
        finally{if(Directory.Exists(temp))Directory.Delete(temp,true);}
    }
}
