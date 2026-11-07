using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;

static class UpdateGate {
    public static bool HandOff(string[] args) {
        if (Array.IndexOf(args,"--update-checked") >= 0) return false;
        string root=AppDomain.CurrentDomain.BaseDirectory;
        if(Path.GetFileName(Application.ExecutablePath).Equals("Toy Story 2 Co-op.exe",StringComparison.OrdinalIgnoreCase)){
            string launcher=Path.Combine(root,"Toy Story 2 Launcher.exe");
            if(!File.Exists(launcher)){MessageBox.Show("Download the standalone launcher to repair this installation.");return true;}
            Process.Start(new ProcessStartInfo(launcher,"--multiplayer"){UseShellExecute=false,WorkingDirectory=root});return true;
        }
        string updater=Path.Combine(root,"Toy Story 2 Updater.exe");
        if (!File.Exists(updater)) { MessageBox.Show("Updater is missing. Extract a complete release ZIP again."); return true; }
        // Run a disposable copy so the installed updater and launcher can both be replaced.
        string temp=Path.Combine(Path.GetTempPath(),"Toy2-update-"+Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(temp);string copy=Path.Combine(temp,"Updater.exe");File.Copy(updater,copy);
        string target=Path.GetFileName(Application.ExecutablePath);
        Process.Start(new ProcessStartInfo(copy,"\""+root.TrimEnd('\\')+"\" \""+target+"\" "+Process.GetCurrentProcess().Id) {UseShellExecute=false,WorkingDirectory=root});
        return true;
    }
}
