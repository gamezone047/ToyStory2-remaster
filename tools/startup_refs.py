exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for word in [b'Screen Mode',b'Screen Mode Select',b'copyright',b'esrb']:
 start=0
 while True:
  at=b.lower().find(word.lower(),start)
  if at<0:break
  print(word,hex(base+p.get_rva_from_offset(at)),repr(b[max(0,at-30):at+70]));start=at+len(word)
for target in [0x49ab90]:
 data=p.sections[0].get_data();va=base+p.sections[0].VirtualAddress
 for j in range(len(data)-5):
  if data[j]==0xe8 and va+j+5+struct.unpack_from('<i',data,j+1)[0]==target:
   print('call',hex(va+j))
   for i in md.disasm(data[j-12:j+16],va+j-12):print(hex(i.address),i.mnemonic,i.op_str)
