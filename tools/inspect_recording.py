import sys, subprocess, pathlib, json
sys.path.insert(0,'tools/video-libs')
import imageio_ffmpeg
import numpy as np
from PIL import Image, ImageDraw
src=r'G:\recordings\Toy Story 2 Action Game\Toy Story 2 Action Game 2026.10.23 - 00.25.48.02.mp4'
raw=subprocess.check_output([imageio_ffmpeg.get_ffmpeg_exe(),'-v','error','-i',src,'-vf','scale=320:180','-pix_fmt','rgb24','-f','rawvideo','-'])
frames=np.frombuffer(raw,np.uint8).reshape(-1,180,320,3)
diff=np.abs(frames[1:].astype(np.int16)-frames[:-1].astype(np.int16)).mean(axis=(1,2,3))
pathlib.Path('tools/video-analysis').mkdir(exist_ok=True)
sheet=Image.new('RGB',(960,720))
for n,sec in enumerate(range(0,24,2)):
 im=Image.fromarray(frames[sec*60]);ImageDraw.Draw(im).text((6,6),str(sec)+' s',fill='yellow');sheet.paste(im,(n%3*320,n//3*180))
sheet.save('tools/video-analysis/overview.jpg')
np.save('tools/video-analysis/frames.npy',frames)
np.save('tools/video-analysis/differences.npy',diff)
for sec in range(25):
 d=diff[sec*60:(sec+1)*60]
 print(sec,'mean',round(float(d.mean()),2),'near duplicates',int((d<.12).sum()),'min/max',round(float(d.min()),2),round(float(d.max()),2))
print('lowest differences',[(round((int(n)+1)/60,3),round(float(diff[n]),3)) for n in np.argsort(diff)[:35]])
