import sys,struct
sys.path.insert(0,'tools/python-libs')
import pefile,capstone
p=pefile.PE('toy2.exe'); b=open('toy2.exe','rb').read(); base=p.OPTIONAL_HEADER.ImageBase
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
for s in [b'DEFKEY\0',b'PAUSE\0',b'PAD_START\0']:
 off=b.find(s); va=base+p.get_rva_from_offset(off); print(s,hex(va))
 needle=struct.pack('<I',va)
 at=0
 while True:
  at=b.find(needle,at)
  if at<0:break
  print('xref',hex(base+p.get_rva_from_offset(at)))
  for i in md.disasm(b[at-16:at+130],base+p.get_rva_from_offset(at-16)):print(hex(i.address),i.mnemonic,i.op_str)
  at+=4
