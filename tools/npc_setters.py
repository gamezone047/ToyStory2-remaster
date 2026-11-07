exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True;ins=list(md.disasm(p.sections[0].get_data(),0x401000))
for n,i in enumerate(ins):
 if i.mnemonic=='or' and (('+ 0x41]' in i.op_str and i.op_str.endswith(', 2')) or ('+ 0x40]' in i.op_str and '0x200' in i.op_str)):
  for j in ins[max(0,n-10):n+10]:print(hex(j.address),j.mnemonic,j.op_str)
