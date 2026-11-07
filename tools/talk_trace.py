exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True;ins=list(md.disasm(p.sections[0].get_data(),0x401000))
for n,i in enumerate(ins):
 if any(t in i.op_str for t in ['0x4f0f','0x4f0e','0x52b7e4']) and (i.address<0x490000):
  print('REF')
  for j in ins[max(0,n-4):n+9]:print(hex(j.address),j.mnemonic,j.op_str)
