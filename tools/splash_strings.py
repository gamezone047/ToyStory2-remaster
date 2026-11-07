exec(open('tools/inspect_keys.py').read().split('for s in')[0])
import re
for m in re.finditer(rb'[ -~]{5,}',b):
 if any(t in m.group().lower() for t in [b'splash',b'esrb',b'legal',b'copyright',b'logo']):print(hex(base+p.get_rva_from_offset(m.start())),m.group()[:160])
