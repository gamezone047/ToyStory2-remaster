exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for n in [0,512,1024,2048,3072]:print(n,struct.unpack('<h',p.get_data(0x4fe788-base+n*2,2))[0])
md.skipdata=True;ins=list(md.disasm(p.sections[0].get_data(),0x401000))
for n,i in enumerate(ins):
 if any(x in i.op_str for x in ['0x4f5b3c','0x4f5b4c','0x502660']):
  print('REF')
  for j in ins[max(0,n-10):n+16]:print(hex(j.address),j.mnemonic,j.op_str)
