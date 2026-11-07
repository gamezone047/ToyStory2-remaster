exec(open('tools/inspect_keys.py').read().split('for s in')[0])
import re
for m in re.finditer(rb'[ -~]{4,}',b):
 if any(t in m.group().lower() for t in [b'volume',b'music',b'sfx',b'sound']):
  if base+p.get_rva_from_offset(m.start())<0x50a000:print(hex(base+p.get_rva_from_offset(m.start())),m.group()[:100])
