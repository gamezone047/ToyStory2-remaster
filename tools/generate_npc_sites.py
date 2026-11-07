exec(open('tools/inspect_keys.py').read().split('for s in')[0])
import re
md.detail=True;md.skipdata=True; ins=list(md.disasm(p.sections[0].get_data(),0x401000));by={i.address:n for n,i in enumerate(ins)}
sites=sorted(set(int(x,16) for x in re.findall(r'(0x[0-9a-f]+) and e(?:ax|cx|dx), 0x200',open('tools/talk_calls.txt').read())))
lines=[]
for va in sites:
 n=by[va]; i=ins[n];reg=i.op_str.split(',')[0]; actor=0
 if va!=0x4a250a:
  for prev in reversed(ins[n-8:n]):
   if prev.mnemonic=='mov' and prev.op_str.startswith(reg+', dword ptr [0x52'):
    actor=int(re.search(r'\[(0x[0-9a-f]+)\]',prev.op_str)[1],16);break
  assert actor,hex(va)
 lines.append(' {0x%x,%d,0x%x}, // %s'%(va,{'eax':0,'ecx':1,'edx':2}[reg],actor,i.bytes.hex()))
open('controls-src/NpcSites.h','w').write('// Verified NPC contact-flag consumers that lead to dialogue.\nstatic const NpcSite npcSites[]={\n'+'\n'.join(lines)+'\n};\n')
print('\n'.join(lines))
