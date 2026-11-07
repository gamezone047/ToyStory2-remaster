"""Analyze a game-generated CSV: py -3.9 tools/analyze_frame_profile.py [path]."""
import csv,sys,pathlib,statistics
files=sorted(pathlib.Path('scripts').glob('FrameProfile-*.csv'),key=lambda p:p.stat().st_mtime)
if len(sys.argv)>1:path=pathlib.Path(sys.argv[1])
elif files:path=files[-1]
else:raise SystemExit('No gameplay capture yet. Start the game yourself and reproduce the stutter.')
with path.open() as f:rows=list(csv.DictReader(f))
rows=[r for r in rows if r.get('dropped') is not None and r['focused']=='1' and 1<=int(r['level'])<=15 and r['stage_mask']=='511']
print(path)
stages=['logic_and_prepare_ms','prepare_ms','background_ms','world_ms','actors_and_effects_ms','submit_ms','flush_ms','audio_ms','present_ms','limiter_ms']
def percentile(values,p):return sorted(values)[min(len(values)-1,int((len(values)-1)*p))]
for native in ('0','1','2','3','4'):
 group=[r for r in rows if r['native_distance']==native]
 if not group:continue
 print('\n'+{'0':'OLD UNLIMITED','1':'ORIGINAL','2':'BALANCED','3':'FAR ENEMIES ONLY','4':'FAR ITEMS ONLY'}[native]+' draw distance:',len(group),'complete focused gameplay frames')
 for field in ['frame_ms','work_ms']+stages:
  values=[float(r[field]) for r in group if float(r[field])>=0]
  if values:print(f'{field:24s} mean={statistics.mean(values):7.3f} p95={percentile(values,.95):7.3f} p99={percentile(values,.99):7.3f} max={max(values):7.3f}')
 print('Frames >25ms:',sum(float(r['frame_ms'])>25 for r in group),'simulation tick jumps:',sum(int(r['simulation_ticks'])>1 for r in group))
 print('Worst frames:')
 for r in sorted(group,key=lambda r:float(r['frame_ms']),reverse=True)[:8]:
  cost=sorted(stages,key=lambda s:float(r[s]),reverse=True)[:3]
  print(' frame',r['frame'],'level',r['level'],'mode',r['camera_mode'],'total',r['frame_ms'],'; '.join(s+'='+r[s] for s in cost))
print('\nDropped diagnostic rows:',max((int(r['dropped']) for r in rows),default=0))
print('CPU-side phase costs include driver waits; this is not a GPU hardware timing trace.')
