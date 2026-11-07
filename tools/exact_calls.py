exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for target in [0x4ac3d0,0x49abb0,0x49ab60]:
 print('TARGET',hex(target))
 for s in p.sections[:1]:
  data=s.get_data();va=base+s.VirtualAddress
  for j in range(len(data)-5):
   if data[j]==0xe8 and va+j+5+struct.unpack_from('<i',data,j+1)[0]==target:print(hex(va+j))
