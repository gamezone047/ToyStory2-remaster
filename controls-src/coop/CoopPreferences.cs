using System;
using System.IO;
using System.Text;

static class CoopPreferences {
    internal static readonly string FilePath=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"ToyStory2 Remaster","multiplayer.txt");
    internal static string[] Load(string path) {
        try {
            var lines=File.ReadAllLines(path);
            if(lines.Length==2){
                string name=Encoding.UTF8.GetString(Convert.FromBase64String(lines[0]));
                string address=Encoding.UTF8.GetString(Convert.FromBase64String(lines[1]));
                if(name.Length<=24&&address.Length<=255&&!name.Contains("\n")&&!address.Contains("\n"))return new[]{name,address};
            }
        }catch(IOException){}catch(UnauthorizedAccessException){}catch(FormatException){}
        return new[]{"Buzz","127.0.0.1"};
    }
    internal static void Save(string path,string name,string address) {
        Directory.CreateDirectory(Path.GetDirectoryName(path));
        File.WriteAllLines(path,new[]{Convert.ToBase64String(Encoding.UTF8.GetBytes(name)),Convert.ToBase64String(Encoding.UTF8.GetBytes(address))});
    }
}
