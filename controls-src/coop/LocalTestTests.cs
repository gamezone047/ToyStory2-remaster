using System;
using System.IO;
static class LocalTestTests {
 static void Check(bool condition,string message){if(!condition)throw new Exception(message);}
 public static void Run(){
  string source=Path.Combine(Path.GetTempPath(),"Toy2-local-source-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(Path.Combine(source,"data"));
  string original="[General]\r\nFullScreenMode = true\r\nCaptureMouse = true\r\nCenterAppWindow = true\r\nWindowedAttributes = borderless, fullscreensize\r\n[DirectX]\r\nResolution = 2560x1440\r\nExtraEnumeratedResolutions = 2560x1440\r\n";
  try{
   File.WriteAllText(Path.Combine(source,"dgVoodoo.conf"),original);
   File.WriteAllText(Path.Combine(source,"data","asset.bin"),"runtime asset");
   File.WriteAllText(Path.Combine(source,"private.sav"),"must not copy");
   File.WriteAllLines(Path.Combine(source,"data","runtime-files.sha256"),new[]{new string('0',64)+"  dgVoodoo.conf",new string('0',64)+"  data/asset.bin"});
   string temporary;
   using(var files=new LocalTestFiles(source)){
    temporary=files.DirectoryPath;Check(files.Players[0]!=files.Players[1],"separate runtime roots");
    foreach(string player in files.Players){string cfg=File.ReadAllText(Path.Combine(player,"dgVoodoo.conf"));Check(cfg.Contains("FullScreenMode = false")&&cfg.Contains("Resolution = 960x540")&&!cfg.Contains("borderless"),"windowed configuration");Check(File.Exists(Path.Combine(player,"data","asset.bin"))&&!File.Exists(Path.Combine(player,"private.sav")),"only manifest runtime copied");}
    File.WriteAllText(Path.Combine(files.Players[0],"data","asset.bin"),"modified");Check(File.ReadAllText(Path.Combine(files.Players[1],"data","asset.bin"))=="runtime asset","independent files");
    Check(File.ReadAllText(Path.Combine(source,"dgVoodoo.conf"))==original,"source configuration preserved");
    bool denied=false;try{LocalTestFiles.Contained(source,"../escape.bin");}catch(IOException){denied=true;}Check(denied,"manifest traversal refused");
   }
   Check(!Directory.Exists(temporary),"temporary runtime cleanup");
  }finally{Directory.Delete(source,true);}
  Console.WriteLine("PASS local test isolation, window configuration, path containment, cleanup");
 }
}
