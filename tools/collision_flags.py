exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True;ins=list(md.disasm(p.sections[0].get_data(),0x401000))
for n,i in enumerate(ins):
 if i.mnemonic=='or' and (i.op_str.endswith(', 0x200') or i.op_str.endswith('ah, 2') or i.op_str.endswith('ch, 2') or i.op_str.endswith('dh, 2')):
  print('REF')
  for j in ins[max(0,n-5):n+7]:print(hex(j.address),j.mnemonic,j.op_str)
