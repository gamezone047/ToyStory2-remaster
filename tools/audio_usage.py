exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True;ins=list(md.disasm(p.sections[0].get_data(),0x401000))
for n,i in enumerate(ins):
 if any(x in i.op_str for x in ['0x52f0cd','0x52f0ce']) and i.address>0x437a00:
  print('REF')
  for j in ins[max(0,n-3):n+12]:print(hex(j.address),j.mnemonic,j.op_str)
