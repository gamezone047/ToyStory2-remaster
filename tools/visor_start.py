exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True; ins=list(md.disasm(p.sections[0].get_data(),0x401000))
for n,i in enumerate(ins):
 if any(x in i.op_str for x in ['0x4f5950','0x4f56bc','0x50a13c']):
  print('REF')
  for j in ins[max(0,n-4):n+8]:print(hex(j.address),j.mnemonic,j.op_str)
