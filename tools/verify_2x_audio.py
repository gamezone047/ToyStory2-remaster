exec(open('tools/inspect_keys.py').read().split('for s in')[0])
sites={0x4087a8:'8bc70fafc7',0x447c8a:'03cf3bd97d6d',
0x4413a8:'3bc10f8d6c020000',0x4a3810:'8b4424108b1518535500',
0x4a3c80:'8b44241453',0x4a3b90:'8b4424188b4c2410',
0x408831:'0f8d86000000',0x447c3d:'0f8cde000000',0x447c4e:'0f84cd000000'}
for va,h in sites.items():assert p.get_data(va-base,len(bytes.fromhex(h)))==bytes.fromhex(h),hex(va)
from pathlib import Path
main=Path('controls-src/ModernControls.cpp').read_text()
pacing=Path('controls-src/FramePacing.h').read_text()
assert '#include "FrameProfile.h"' not in main and 'ProfileFinish(' not in pacing
assert 'InstallFrameProfile' not in main
print('PASS: 2x distance hooks, sound trampolines, preserved room/inactivity gates; profiler detached')
