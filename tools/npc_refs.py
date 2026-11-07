exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for target in [0x4f0a18,0x4f08c0]:
 needle=struct.pack('<I',target);at=0
 while True:
  at=b.find(needle,at)
  if at<0:break
  print(hex(target),'ref',hex(base+p.get_rva_from_offset(at)));at+=4
md.skipdata=True; ins=list(md.disasm(p.sections[0].get_data(),0x401000))
for n,i in enumerate(ins):
 if '0x2000' in i.op_str and 0x435800<i.address<0x435a00:
  for j in ins[max(0,n-4):n+12]:print(hex(j.address),j.mnemonic,j.op_str)
