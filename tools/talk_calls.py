exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True;ins=list(md.disasm(p.sections[0].get_data(),0x401000))
for n,i in enumerate(ins):
 if i.mnemonic=='call' and i.op_str=='0x4027f0':
  print('TALK',hex(i.address))
  for j in ins[max(0,n-23):n+3]:print(hex(j.address),j.mnemonic,j.op_str)
