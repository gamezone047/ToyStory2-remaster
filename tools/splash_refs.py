exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True;ins=list(md.disasm(p.sections[0].get_data(),0x401000))
for n,i in enumerate(ins):
 if '0x4fe100' in i.op_str:
  for j in ins[n-10:n+25]:print(hex(j.address),j.mnemonic,j.op_str)
