exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True; sec=p.sections[0]; ins=list(md.disasm(sec.get_data(),base+sec.VirtualAddress))
for n,i in enumerate(ins):
 if i.mnemonic=='call' and i.op_str in ['0x4aceb0','0x4ace30','0x49a930']:
  print('\nREF')
  for j in ins[max(0,n-8):n+15]:print(hex(j.address),j.mnemonic,j.op_str)



