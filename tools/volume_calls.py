exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for target in [0x49ae40,0x49ae20]:
 print('TARGET',hex(target));data=p.sections[0].get_data();va=0x401000
 for j in range(len(data)-5):
  if data[j]==0xe8 and va+j+5+struct.unpack_from('<i',data,j+1)[0]==target:print(hex(va+j))
