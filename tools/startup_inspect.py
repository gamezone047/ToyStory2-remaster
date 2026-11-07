exec(open('tools/inspect_keys.py').read().split('for s in')[0])
md.skipdata=True
for ent in p.DIRECTORY_ENTRY_IMPORT:
 for imp in ent.imports:
  if imp.name and any(x in imp.name.lower() for x in [b'dialogbox',b'enddialog']):print(imp.name,hex(imp.address))
for word in [b'Screen Mode',b'tt.dll',b'acti.dll',b'dlogo.dll',b'1st trailer',b'ESRB',b'copyright']:
 off=b.lower().find(word.lower());print(word,hex(base+p.get_rva_from_offset(off)) if off>=0 else 'missing')
 if off<0:continue
 needle=struct.pack('<I',base+p.get_rva_from_offset(off));at=0
 while True:
  at=b.find(needle,at)
  if at<0:break
  va=base+p.get_rva_from_offset(at)
  if va<0x4dc000:
   print('xref',hex(va))
   for i in md.disasm(b[at-12:at+65],va-12):print(hex(i.address),i.mnemonic,i.op_str)
  at+=4
