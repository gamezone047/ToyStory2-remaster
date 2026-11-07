using System;
using System.IO;
static class UpdaterPackageTest {
    static int Main(string[] args) {
        string stage=Path.Combine(Path.GetTempPath(),"Toy2-package-check-"+Guid.NewGuid().ToString("N"));
        try {
            UpdateWindow.Extract(Path.GetFullPath(args[0]),stage);int count=0;
            foreach(string file in Directory.GetFiles(stage,"*",SearchOption.AllDirectories)) {
                string relative=file.Substring(stage.Length+1);
                if(UpdateWindow.Hash(file)!=UpdateWindow.Hash(Path.Combine(args[1],relative)))throw new Exception("Extracted file mismatch: "+relative);
                if(Path.GetExtension(file)==".sav")throw new Exception("Save in release");count++;
            }
            string manifest=Path.Combine(stage,"data","runtime-files.sha256");
            int manifestCount=0;
            foreach(string line in File.ReadAllLines(manifest)) {
                if(line.Length<67||line.Substring(64,2)!="  ")throw new Exception("Malformed runtime manifest");
                string relative=line.Substring(66);
                if(!UpdateWindow.Allowed(relative)||UpdateWindow.Hash(Path.Combine(stage,relative))!=line.Substring(0,64))throw new Exception("Runtime manifest mismatch: "+relative);
                manifestCount++;
            }
            if(manifestCount!=count-1)throw new Exception("Runtime manifest does not cover all packaged files");
            Console.WriteLine("PASS: actual updater extracted and hash-verified all "+count+" packaged files; no saves included.");return 0;
        } catch(Exception e){Console.Error.WriteLine(e);return 1;}
        finally{if(Directory.Exists(stage))Directory.Delete(stage,true);}
    }
}
