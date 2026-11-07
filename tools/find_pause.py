exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True; sec=p.sections[0]; ins=list(md.disasm(sec.get_data(),base+sec.VirtualAddress))
for n,i in enumerate(ins):
 if '0x529b78' in i.op_str:
  print('\nREF')
  for j in ins[max(0,n-8):n+15]:print(hex(j.address),j.mnemonic,j.op_str)


