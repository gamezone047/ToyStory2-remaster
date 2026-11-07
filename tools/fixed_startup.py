from pathlib import Path
import struct
exec(open('tools/inspect_keys.py').read().split('for s in')[0])
path=Path('toy2.exe');data=bytearray(path.read_bytes())
patches=[(0x49d9bf, bytes.fromhex('e8 2c a8 f9 ff'),bytes.fromhex('b8 01 00 00 00')),(0x49d9cc,bytes.fromhex('e8 bf d1 ff ff'),bytes.fromhex('b8 01 00 00 00'))]
for va,old,new in patches:
 off=p.get_offset_from_rva(va-base)
 assert data[off:off+len(old)]==old,(hex(va),data[off:off+len(old)].hex())
 data[off:off+len(new)]=new
p.close()
backup=Path('controls-src/toy2-before-startup.exe')
assert not backup.exists() or backup.read_bytes()==path.read_bytes()
backup.write_bytes(path.read_bytes());path.write_bytes(data)
conf=Path('dgVoodoo.conf');s=conf.read_text();Path('controls-src/dgVoodoo-before-startup.conf').write_text(s)
s=s.replace('ExtraEnumeratedResolutions = 1280x720, 1920x1080, 2560x1440','ExtraEnumeratedResolutions = 2560x1440');conf.write_text(s)
cfg=Path('toy2.cfg');v=bytearray(cfg.read_bytes());Path('controls-src/toy2-before-startup.cfg').write_bytes(v);struct.pack_into('<I',v,20,0);cfg.write_bytes(v)
print('Patched startup splash/movie calls; fixed sole display mode at 2560x1440.')


