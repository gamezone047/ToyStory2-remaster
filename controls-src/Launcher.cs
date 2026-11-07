using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Threading.Tasks;

sealed class Settings {
    public string Resolution = "2560x1440";
    public int Music = 100, Sfx = 100;
    public int Sensitivity = 2, AimSensitivity = 2;
    public bool InvertY;
}

static class Configuration {
    public static readonly string[] Resolutions = { "1280x720", "1600x900", "1920x1080", "2560x1440", "3840x2160" };
    public static string Get(string text, string section, string key, string fallback) {
        string current = "";
        foreach (string line in text.Replace("\r", "").Split('\n')) {
            string t = line.Trim();
            if (t.StartsWith("[") && t.EndsWith("]")) current = t.Substring(1, t.Length - 2);
            else if (String.Equals(current, section, StringComparison.OrdinalIgnoreCase)) {
                int equal = t.IndexOf('=');
                if (equal > 0 && String.Equals(t.Substring(0, equal).Trim(), key, StringComparison.OrdinalIgnoreCase))
                    return t.Substring(equal + 1).Trim();
            }
        }
        return fallback;
    }
    public static string Set(string text, string section, string key, string value) {
        var lines = new List<string>(text.Replace("\r", "").Split('\n'));
        int start = -1, end = lines.Count;
        for (int i = 0; i < lines.Count; i++) {
            string t = lines[i].Trim();
            if (t.StartsWith("[") && t.EndsWith("]")) {
                if (start >= 0) { end = i; break; }
                if (String.Equals(t, "[" + section + "]", StringComparison.OrdinalIgnoreCase)) start = i;
            }
        }
        if (start < 0) { lines.Add("[" + section + "]"); lines.Add(key + "=" + value); }
        else {
            for (int i = start + 1; i < end; i++) {
                int equal = lines[i].IndexOf('=');
                if (equal > 0 && String.Equals(lines[i].Substring(0, equal).Trim(), key, StringComparison.OrdinalIgnoreCase)) {
                    lines[i] = key + "=" + value;
                    return String.Join("\r\n", lines);
                }
            }
            lines.Insert(end, key + "=" + value);
        }
        return String.Join("\r\n", lines);
    }
    static int Percent(string text) { int v; return Int32.TryParse(text, out v) ? Math.Max(0, Math.Min(100, v)) : 100; }
    static int MouseGain(string text) { int v; return Int32.TryParse(text, out v) ? Math.Max(1, Math.Min(10, v)) : 2; }
    public static Settings Load(string root) {
        string ini = File.ReadAllText(Path.Combine(root, "scripts", "ModernControls.ini"));
        var s = new Settings();
        s.Resolution = Get(ini, "Launcher", "Resolution", s.Resolution);
        if (Array.IndexOf(Resolutions, s.Resolution) < 0) s.Resolution = "2560x1440";
        s.Music = Percent(Get(ini, "Audio", "MusicVolume", "100"));
        s.Sfx = Percent(Get(ini, "Audio", "SfxVolume", "100"));
        s.InvertY = Get(ini, "Mouse", "InvertY", "0") != "0";
        s.Sensitivity = MouseGain(Get(ini, "Mouse", "Sensitivity", "2"));
        s.AimSensitivity = MouseGain(Get(ini, "Mouse", "AimSensitivity", "2"));
        return s;
    }
    static string ReplaceSetting(string text, string key, string value) {
        var regex = new Regex(@"(?m)^" + Regex.Escape(key) + @"[ \t]*=[^\r\n]*");
        if (!regex.IsMatch(text)) throw new InvalidDataException("Missing display setting: " + key);
        return regex.Replace(text, key + " = " + value);
    }
    static void AtomicWrite(string path, byte[] bytes) {
        string temporary = path + ".launcher-tmp";
        File.WriteAllBytes(temporary, bytes);
        try { File.Replace(temporary, path, null); }
        finally { if (File.Exists(temporary)) File.Delete(temporary); }
    }
    public static void Save(string root, Settings s) {
        if (Array.IndexOf(Resolutions, s.Resolution) < 0 || s.Music < 0 || s.Music > 100 || s.Sfx < 0 || s.Sfx > 100 || s.Sensitivity < 1 || s.Sensitivity > 10 || s.AimSensitivity < 1 || s.AimSensitivity > 10)
            throw new InvalidDataException("Invalid launcher settings.");
        string iniPath = Path.Combine(root, "scripts", "ModernControls.ini");
        string displayPath = Path.Combine(root, "dgVoodoo.conf");
        string cfgPath = Path.Combine(root, "toy2.cfg");
        string ini = File.ReadAllText(iniPath), display = File.ReadAllText(displayPath);
        byte[] cfg = File.ReadAllBytes(cfgPath);
        if (cfg.Length != 24) throw new InvalidDataException("Unexpected toy2.cfg format. No settings were changed.");
        ini = Set(ini, "Launcher", "Resolution", s.Resolution);
        ini = Set(ini, "Audio", "MusicVolume", s.Music.ToString());
        ini = Set(ini, "Audio", "SfxVolume", s.Sfx.ToString());
        ini = Set(ini, "Mouse", "InvertY", s.InvertY ? "1" : "0");
        ini = Set(ini, "Mouse", "Sensitivity", s.Sensitivity.ToString());
        ini = Set(ini, "Mouse", "AimSensitivity", s.AimSensitivity.ToString());
        display = ReplaceSetting(display, "Resolution", s.Resolution);
        display = ReplaceSetting(display, "ExtraEnumeratedResolutions", s.Resolution);
        display = ReplaceSetting(display, "DefaultEnumeratedResolutions", "none");
        display = ReplaceSetting(display, "EnumeratedResolutionBitdepths", "32");
        display = ReplaceSetting(display, "FullScreenMode", "false");
        display = ReplaceSetting(display, "PresentationModel", "discard");
        display = Set(display, "General", "CenterAppWindow", "false");
        display = Set(display, "General", "FullScreenOutput", "1");
        display = ReplaceSetting(display, "WindowedAttributes", "borderless, fullscreensize");
        Array.Copy(BitConverter.GetBytes(0), 0, cfg, 20, 4);
        var updates = new Dictionary<string, byte[]> {
            { iniPath, new UTF8Encoding(false).GetBytes(ini) },
            { displayPath, new UTF8Encoding(false).GetBytes(display) },
            { cfgPath, cfg }
        };
        var originals = new Dictionary<string, byte[]>();
        var changed = new List<string>();
        foreach (string path in updates.Keys) {
            originals[path] = File.ReadAllBytes(path);
            if (!File.Exists(path + ".launcher-backup")) File.WriteAllBytes(path + ".launcher-backup", originals[path]);
        }
        try {
            foreach (var item in updates) { AtomicWrite(item.Key, item.Value); changed.Add(item.Key); }
        } catch {
            foreach (string path in changed) AtomicWrite(path, originals[path]);
            throw;
        }
    }
}

sealed class Launcher : Form {
    string root;
    bool busy;
    TextBox installPath;
    ComboBox resolution;
    TrackBar music, sfx, sensitivity, aimSensitivity;
    CheckBox invert;
    Label status;
    public Launcher(string directory) {
        root = directory;
        Text = "Toy Story 2 Launcher";
        ClientSize = new Size(560, 825);
        MinimumSize = MaximumSize = Size;
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        AutoScaleMode = AutoScaleMode.Dpi;
        Font = new Font("Segoe UI", 10);
        BackColor = Color.FromArgb(22, 32, 53);
        ForeColor = Color.White;
        string iconPath = Path.Combine(root, "toy2.ico");
        if (File.Exists(iconPath)) Icon = new Icon(iconPath);
        var title = AddLabel("TOY STORY 2", 28, 22, 500, 42);
        title.Font = new Font("Segoe UI", 24, FontStyle.Bold);
        AddLabel("Ready for your next adventure", 30, 68, 500, 28).ForeColor = Color.FromArgb(163, 191, 229);
        AddLabel("Resolution  ·  16:9 borderless", 30, 114, 490, 27);
        resolution = new ComboBox { Left = 30, Top = 147, Width = 500, DropDownStyle = ComboBoxStyle.DropDownList };
        foreach (string mode in Configuration.Resolutions) resolution.Items.Add(mode.Replace("x", " × ") + (mode == "2560x1440" ? "  (1440p / 2K)" : ""));
        Controls.Add(resolution);
        sfx = AddVolume("SFX volume", 203);
        music = AddVolume("Music volume", 291);
        sensitivity = AddSensitivity("General mouse sensitivity", 379);
        aimSensitivity = AddSensitivity("Aiming mouse sensitivity", 467);
        invert = new CheckBox { Text = "Invert vertical mouse look", Left = 32, Top = 552, Width = 470, Height = 28, AutoSize = false };
        Controls.Add(invert);
        AddLabel("WASD: move  ·  RMB: aim  ·  LMB: fire  ·  E: interact\nLeft Ctrl: spin / airborne hard landing  ·  Esc: menu", 30, 590, 500, 42).Font = new Font("Segoe UI", 9);
        status = AddLabel("Settings apply on the next game start. Intros stay skipped.", 30, 638, 500, 29);
        status.Font = new Font("Segoe UI", 9);
        status.ForeColor = Color.FromArgb(163, 191, 229);
        var save = AddButton("Save settings", 30, 685, 150);
        save.Click += delegate { Save(false); };
        var play = AddButton("Save && Play", 190, 685, 160);
        play.BackColor = Color.FromArgb(59, 119, 212);
        play.Click += delegate { Save(true); };
        var multiplayer = AddButton("Multiplayer", 360, 685, 170);
        multiplayer.Click += delegate { Save(false, true); };
        // Keep the familiar menu, with a destination selector above its settings.
        foreach(Control c in Controls)if(c.Top>=114)c.Top+=80;
        AddLabel("Game installation folder",30,106,500,25);
        installPath=new TextBox {Left=30,Top=135,Width=390,ReadOnly=true,Text=root};Controls.Add(installPath);
        var browse=AddButton("Browse…",430,130,100);
        browse.Click+=delegate {
            using(var picker=new FolderBrowserDialog {Description="Choose where Toy Story 2 should be installed",SelectedPath=root,ShowNewFolderButton=true}) {
                if(picker.ShowDialog(this)==DialogResult.OK){root=picker.SelectedPath;installPath.Text=root;PortableInstall.Remember(root);}
            }
        };
        FormClosing+=delegate(object sender,FormClosingEventArgs e){if(busy)e.Cancel=true;};
        var settings = File.Exists(Path.Combine(root,"scripts","ModernControls.ini")) ? Configuration.Load(root) : PortableInstall.LoadPreferences();
        resolution.SelectedIndex = Array.IndexOf(Configuration.Resolutions, settings.Resolution);
        music.Value = settings.Music; sfx.Value = settings.Sfx; invert.Checked = settings.InvertY;
        sensitivity.Value = settings.Sensitivity; aimSensitivity.Value = settings.AimSensitivity;
        AcceptButton = play;
        title.Visible=false;
        Controls[1].Visible=false;
        foreach(Control c in Controls) {
            c.Left+=360;c.Top=(int)(c.Top*.84)+18;
            if(c is Label||c is CheckBox)c.BackColor=Color.Transparent;
            else if(c is TrackBar)c.BackColor=LauncherSkin.Panel;
        }
        // Put the control reference on the illustrated side of the launcher.
        foreach(Control c in Controls)if(c.Text.StartsWith("WASD:")){c.SetBounds(28,565,330,95);c.Text="WASD   Move\nMouse   Look    •    RMB   Aim\nLMB   Fire    •    E   Talk\nLeft Ctrl   Spin / hard landing\nEsc   Pause";}
        play.Text="PLAY";save.Text="SAVE";
        MinimumSize=Size.Empty;MaximumSize=Size.Empty;ClientSize=new Size(960,760);
        MinimumSize=MaximumSize=Size;DoubleBuffered=true;
    }
    protected override void OnPaintBackground(PaintEventArgs e){LauncherSkin.Background(e.Graphics,ClientSize);}
    Label AddLabel(string text, int x, int y, int width, int height) {
        var label = new Label { Text = text, Left = x, Top = y, Width = width, Height = height };
        Controls.Add(label); return label;
    }
    TrackBar AddVolume(string title, int y) {
        AddLabel(title, 30, y, 400, 27);
        var value = AddLabel("100%", 460, y, 70, 27); value.TextAlign = ContentAlignment.TopRight;
        var slider = new TrackBar { Left = 25, Top = y + 29, Width = 510, Height = 45, Minimum = 0, Maximum = 100, TickFrequency = 10, SmallChange = 5, LargeChange = 10, Value = 100 };
        slider.ValueChanged += delegate { value.Text = slider.Value + "%"; };
        Controls.Add(slider); return slider;
    }
    Button AddButton(string text, int x, int y, int width) {
        var button = new GameButton { Text = text, Left = x, Top = y, Width = width, Height = 40, FlatStyle = FlatStyle.Flat, ForeColor = Color.White, BackColor = Color.FromArgb(48, 65, 91) };
        Controls.Add(button); return button;
    }
    TrackBar AddSensitivity(string title, int y) {
        AddLabel(title, 30, y, 400, 27);
        var value = AddLabel("2 / 10", 460, y, 70, 27); value.TextAlign = ContentAlignment.TopRight;
        var slider = new TrackBar { Left = 25, Top = y + 29, Width = 510, Height = 45, Minimum = 1, Maximum = 10, TickFrequency = 1, SmallChange = 1, LargeChange = 1, Value = 2 };
        slider.ValueChanged += delegate { value.Text = slider.Value + " / 10"; };
        Controls.Add(slider); return slider;
    }
    async void Save(bool play, bool multiplayer = false) {
        if(busy)return;
        try {
            if (Process.GetProcessesByName("toy2").Length > 0) {
                MessageBox.Show(this, "Close Toy Story 2 before changing settings or starting another copy.", Text, MessageBoxButtons.OK, MessageBoxIcon.Information); return;
            }
            if (resolution.SelectedIndex < 0) throw new InvalidDataException("Select a resolution.");
            var selected=new Settings { Resolution = Configuration.Resolutions[resolution.SelectedIndex], Music = music.Value, Sfx = sfx.Value, InvertY = invert.Checked, Sensitivity = sensitivity.Value, AimSensitivity = aimSensitivity.Value };
            PortableInstall.ValidateFolder(root);PortableInstall.Remember(root);PortableInstall.SavePreferences(selected);
            if(play||multiplayer) {
                status.Text=multiplayer?"Checking for updates before opening Multiplayer�":"Checking for updates before Play�";
                busy=true;foreach(Control c in Controls)c.Enabled=false;status.Enabled=true;UseWaitCursor=true;
                await Task.Run(delegate { PortableInstall.Ensure(root,delegate(string message){BeginInvoke((Action)delegate{status.Text=message;});}); });
            }
            if(File.Exists(Path.Combine(root,"scripts","ModernControls.ini"))&&File.Exists(Path.Combine(root,"dgVoodoo.conf"))&&File.Exists(Path.Combine(root,"toy2.cfg")))Configuration.Save(root,selected);
            status.Text = "Saved. Your settings will be used on the next game start.";
            if (multiplayer) {
                Process.Start(new ProcessStartInfo(Path.Combine(root, "Toy Story 2 Co-op.exe"),"--update-checked") { WorkingDirectory = root, UseShellExecute = false });
                busy=false;
                Close();
            }
            if (play) {
                // The plugin supplies portable asset paths inside the game process.
                // No machine-wide registry registration or elevation is needed.
                Process.Start(GameLaunch.Create(root));
                busy=false;
                Close();
            }
        } catch (Win32Exception e) {
            if (e.NativeErrorCode == 1223) status.Text = "Launch cancelled. Your settings are saved.";
            else MessageBox.Show(this, e.Message, "Unable to start game", MessageBoxButtons.OK, MessageBoxIcon.Error);
        } catch (Exception e) {
            MessageBox.Show(this, e.Message, "Unable to prepare game", MessageBoxButtons.OK, MessageBoxIcon.Error);
        } finally {busy=false;if(!IsDisposed){UseWaitCursor=false;foreach(Control c in Controls)c.Enabled=true;}}
    }
    [STAThread] static int Main(string[] args) {
        try {
            if (args.Length > 0 && args[0] == "--self-test") { LauncherTests.Run(); return 0; }
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            if(args.Length==3&&args[0]=="--render-preview") {
                using(var preview=new Launcher(args[2]))using(var bitmap=new Bitmap(preview.Width,preview.Height)) {
                    preview.StartPosition=FormStartPosition.Manual;preview.Location=new Point(-3000,-3000);preview.Show();Application.DoEvents();preview.DrawToBitmap(bitmap,new Rectangle(0,0,bitmap.Width,bitmap.Height));bitmap.Save(args[1]);
                }
                return 0;
            }
            if (PortableInstall.Bootstrap(args)) return 0;
            var launcher=new Launcher(PortableInstall.DefaultRoot(PortableInstall.Origin));
            if(Array.IndexOf(args,"--multiplayer")>=0)launcher.Shown+=delegate{launcher.Save(false,true);};
            Application.Run(launcher); return 0;
        } catch (Exception e) {
            if (args.Length > 0) { Console.Error.WriteLine(e); return 1; }
            MessageBox.Show(e.Message, "Toy Story 2 Launcher", MessageBoxButtons.OK, MessageBoxIcon.Error); return 1;
        }
    }
}

static class LauncherTests {
    static string GetDisplaySetting(string text,string key) { var m=Regex.Match(text,@"(?m)^"+Regex.Escape(key)+@"[ \t]*=[ \t]*([^\r\n]*)");return m.Success?m.Groups[1].Value.Trim():""; }
    static void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
    public static void Run() {
        string previousLayer=Environment.GetEnvironmentVariable("__COMPAT_LAYER");
        try {
            Environment.SetEnvironmentVariable("__COMPAT_LAYER","DWM8And16BitMitigation RunAsAdmin RunAsHighest");
            var launch=GameLaunch.Create(AppDomain.CurrentDomain.BaseDirectory);
            Check(!launch.UseShellExecute&&String.IsNullOrEmpty(launch.Verb),"No shell elevation request");
            Check(launch.EnvironmentVariables["__COMPAT_LAYER"]=="DWM8And16BitMitigation RunAsInvoker","Child-only elevation override preserves display compatibility");
            Check(Environment.GetEnvironmentVariable("__COMPAT_LAYER").Contains("RunAsAdmin"),"Parent environment unchanged");
            launch.EnvironmentVariables["TOY2_COOP_BRIDGE"]="test-bridge";
            Check(launch.EnvironmentVariables["TOY2_COOP_BRIDGE"]=="test-bridge"&&launch.EnvironmentVariables["__COMPAT_LAYER"].Contains("RunAsInvoker"),"Co-op bridge and compatibility coexist");
            Environment.SetEnvironmentVariable("__COMPAT_LAYER",null);
            Check(GameLaunch.Create(AppDomain.CurrentDomain.BaseDirectory).EnvironmentVariables["__COMPAT_LAYER"]=="RunAsInvoker","Clean-environment launch override");
        } finally { Environment.SetEnvironmentVariable("__COMPAT_LAYER",previousLayer); }
        string dir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "controls-src", "launcher-test");
        Directory.CreateDirectory(Path.Combine(dir, "scripts"));
        string ini = "[Mouse]\r\nSensitivity=2\r\nInvertY=0\r\n[Other]\r\nKeep=unchanged\r\n";
        string display = "[General]\r\nPresentationModel = flip_discard\r\nFullScreenMode = false\r\nWindowedAttributes = borderless, fullscreensize\r\n[Glide]\r\nResolution = 2560x1440\r\n[DirectX]\r\nResolution = 2560x1440\r\n[DirectXExt]\r\nDefaultEnumeratedResolutions = none\r\nExtraEnumeratedResolutions = 2560x1440\r\nEnumeratedResolutionBitdepths = 32\r\n";
        File.WriteAllText(Path.Combine(dir, "scripts", "ModernControls.ini"), ini);
        File.WriteAllText(Path.Combine(dir, "dgVoodoo.conf"), display);
        byte[] cfg = new byte[24]; cfg[0] = 7; cfg[20] = 2; File.WriteAllBytes(Path.Combine(dir, "toy2.cfg"), cfg);
        foreach (string mode in Configuration.Resolutions) {
            Configuration.Save(dir, new Settings { Resolution = mode, Music = 0, Sfx = 65, InvertY = true, Sensitivity = 9, AimSensitivity = 3 });
            Settings loaded = Configuration.Load(dir);
            Check(loaded.Resolution == mode && loaded.Music == 0 && loaded.Sfx == 65 && loaded.InvertY, "Settings round trip");
            Check(loaded.Sensitivity == 9 && loaded.AimSensitivity == 3, "Independent mouse sensitivities round trip");
            string updated = File.ReadAllText(Path.Combine(dir, "dgVoodoo.conf"));
            Check(Regex.Matches(updated, "Resolution = " + mode).Count == 2, "Both renderer resolutions");
            Check(updated.Contains("ExtraEnumeratedResolutions = " + mode), "Native mode enumeration");
            Check(GetDisplaySetting(updated,"PresentationModel") == "discard", "Legacy presentation survives restore");
            Check(Configuration.Get(updated,"General","CenterAppWindow","") == "false", "Avoid monitor recentering");
            string controls = File.ReadAllText(Path.Combine(dir, "scripts", "ModernControls.ini"));
            Check(controls.Contains("Keep=unchanged"), "Preserve unrelated settings");
            byte[] saved = File.ReadAllBytes(Path.Combine(dir, "toy2.cfg"));
            Check(saved.Length == 24 && saved[0] == 7 && saved[20] == 0, "Native mode index");
        }
        string previous = File.ReadAllText(Path.Combine(dir, "scripts", "ModernControls.ini"));
        File.WriteAllBytes(Path.Combine(dir, "toy2.cfg"), new byte[1]);
        bool rejected = false; try { Configuration.Save(dir, new Settings()); } catch (InvalidDataException) { rejected = true; }
        Check(rejected && previous == File.ReadAllText(Path.Combine(dir, "scripts", "ModernControls.ini")), "Reject malformed config without partial changes");
        File.WriteAllText(Path.Combine(dir, "PASS.txt"), "PASS: all resolutions, independent audio, inversion, preserved settings, native mode index, malformed-config protection. No game launched.");
    }
}


