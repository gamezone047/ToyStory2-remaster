using System;
using System.Diagnostics;
using System.IO;
using System.Collections.Generic;
using Microsoft.Win32;

static class GameLaunch {
    internal const string CompatibilityKey = @"Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers";
    internal static string CleanLayers(string value) {
        var kept = new List<string>();
        foreach (string layer in value.Split(new[]{' ','\t'},StringSplitOptions.RemoveEmptyEntries)) {
            if (!layer.Equals("RUNASADMIN",StringComparison.OrdinalIgnoreCase) &&
                !layer.Equals("RUNASHIGHEST",StringComparison.OrdinalIgnoreCase) &&
                !layer.Equals("WIN98",StringComparison.OrdinalIgnoreCase)) kept.Add(layer);
        }
        return String.Join(" ",kept);
    }
    static void RepairCompatibility(string executable) {
        // Repair only this game's per-user entry. Never change machine-wide
        // settings or request elevation. WIN98 itself can force error 740.
        using (var key = Registry.CurrentUser.OpenSubKey(CompatibilityKey,true)) {
            if (key == null) return;
            string old = key.GetValue(executable) as string;
            if (old == null) return;
            string repaired = CleanLayers(old);
            if (repaired == old) return;
            if (repaired.Trim(' ','~','$').Length == 0) key.DeleteValue(executable,false);
            else key.SetValue(executable,repaired,RegistryValueKind.String);
        }
    }
    public static ProcessStartInfo Create(string root) {
        string executable=Path.GetFullPath(Path.Combine(root,"toy2.exe"));
        RepairCompatibility(executable);
        var info=new ProcessStartInfo(executable) {WorkingDirectory=root,UseShellExecute=false};
        // Windows can apply an elevation compatibility fix to an old executable
        // even without an elevation manifest. Override it for this child only.
        // Preserve unrelated inherited compatibility fixes.
        var layers=new List<string>();
        string inherited=CleanLayers(info.EnvironmentVariables["__COMPAT_LAYER"]??"");
        foreach(string layer in inherited.Split(new[]{' ','\t'},StringSplitOptions.RemoveEmptyEntries)) {
            if(!layer.Equals("RunAsAdmin",StringComparison.OrdinalIgnoreCase)&&
               !layer.Equals("RunAsHighest",StringComparison.OrdinalIgnoreCase)&&
               !layer.Equals("RunAsInvoker",StringComparison.OrdinalIgnoreCase))layers.Add(layer);
        }
        layers.Add("RunAsInvoker");
        info.EnvironmentVariables["__COMPAT_LAYER"]=String.Join(" ",layers);
        return info;
    }
}
