using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Reflection;
using System.Windows.Forms;

static class LauncherSkin {
    public static readonly Color Panel=Color.FromArgb(17,39,83);
    static readonly Image Title=Image.FromStream(Assembly.GetExecutingAssembly().GetManifestResourceStream("Launcher.Title"));
    static readonly Image FontAtlas=Image.FromStream(Assembly.GetExecutingAssembly().GetManifestResourceStream("Launcher.Font"));
    const string Glyphs="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,;:\\/'!?()\"";
    public static void Text(Graphics g,string text,float x,float y,float size) {
        using(var attributes=new ImageAttributes()) {
            attributes.SetColorKey(Color.Lime,Color.Lime);
            var old=g.InterpolationMode;g.InterpolationMode=InterpolationMode.NearestNeighbor;
            foreach(char ch in text.ToUpperInvariant()) {
                int index=Glyphs.IndexOf(ch);
                if(index>=0)g.DrawImage(FontAtlas,new Rectangle((int)x,(int)y,(int)size,(int)size),index%8*32,index/8*32,32,32,GraphicsUnit.Pixel,attributes);
                x+=size*.86f;
            }
            g.InterpolationMode=old;
        }
    }
    public static void Background(Graphics g,Size size) {
        using(var gradient=new LinearGradientBrush(new Rectangle(Point.Empty,size),Color.FromArgb(7,21,65),Color.FromArgb(14,113,158),90f))g.FillRectangle(gradient,new Rectangle(Point.Empty,size));
        var random=new Random(22);
        using(var stars=new SolidBrush(Color.FromArgb(100,190,225,255)))for(int i=0;i<90;i++){int x=random.Next(size.Width),y=random.Next(size.Height);g.FillEllipse(stars,x,y,2,2);}
        g.DrawImage(Title,new Rectangle(22,90,340,255));
        Text(g,"REMASTERED",73,365,25);
        Text(g,"TO INFINITY",55,440,25);Text(g,"AND BEYOND!",55,474,25);
        using(var fill=new SolidBrush(Panel))g.FillRectangle(fill,380,20,560,size.Height-40);
        using(var pen=new Pen(Color.FromArgb(62,147,219),2))g.DrawRectangle(pen,380,20,560,size.Height-40);
        Text(g,"GAME OPTIONS",420,42,24);
    }
}
sealed class GameButton : Button {
    bool hover;
    public GameButton(){SetStyle(ControlStyles.UserPaint|ControlStyles.AllPaintingInWmPaint|ControlStyles.OptimizedDoubleBuffer,true);}
    protected override void OnMouseEnter(EventArgs e){hover=true;Invalidate();base.OnMouseEnter(e);}
    protected override void OnMouseLeave(EventArgs e){hover=false;Invalidate();base.OnMouseLeave(e);}
    protected override void OnPaint(PaintEventArgs e) {
        var bounds=new Rectangle(1,1,Width-3,Height-3);
        using(var brush=new LinearGradientBrush(bounds,hover?Color.FromArgb(52,124,206):Color.FromArgb(35,83,151),Color.FromArgb(13,34,83),90f))e.Graphics.FillRectangle(brush,bounds);
        using(var pen=new Pen(Enabled?Color.FromArgb(244,203,37):Color.Gray,2))e.Graphics.DrawRectangle(pen,bounds);
        string label=Text.Replace("&&","&").Replace("&","");float size=Math.Min(18,(Width-16)/(Math.Max(1,label.Length)*.86f));
        LauncherSkin.Text(e.Graphics,label,(Width-label.Length*size*.86f)/2,(Height-size)/2,size);
        if(Focused)ControlPaint.DrawFocusRectangle(e.Graphics,new Rectangle(5,5,Width-10,Height-10),Color.White,BackColor);
    }
}
