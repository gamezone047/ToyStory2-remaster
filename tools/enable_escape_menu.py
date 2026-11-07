from pathlib import Path
import sys,struct,hashlib
sys.path.insert(0,str(Path(__file__).parent/'python-libs'))
import pefile,capstone
root=Path(__file__).resolve().parent.parent
exe=root/'toy2.exe'; original=exe.read_bytes(); p=pefile.PE(data=original)
base=p.OPTIONAL_HEADER.ImageBase
hook=0x414b98; resume=0x414b9f; cave=0x4dbd50
hook_off=p.get_offset_from_rva(hook-base); cave_off=p.get_offset_from_rva(cave-base)
expected=bytes.fromhex('668b35009b5200')
assert original[hook_off:hook_off+7]==expected,'Unexpected executable: refusing patch'
# At each keyboard poll: if Escape is down, set Return down as well.
# Then replay the overwritten instruction and resume the original input loop.
code=bytes.fromhex('803d019a5200007407c6051c9a520001')+expected
code+=b'\xe9'+struct.pack('<i',resume-(cave+len(code)+5))
assert all(v==0 for v in original[cave_off:cave_off+len(code)]),'Code padding is not empty'
patched=bytearray(original)
patched[cave_off:cave_off+len(code)]=code
patched[hook_off:hook_off+7]=b'\xe9'+struct.pack('<i',cave-(hook+5))+b'\x90\x90'
text=p.sections[0]
size=max(text.Misc_VirtualSize,cave-base-text.VirtualAddress+len(code))
struct.pack_into('<I',patched,text.get_file_offset()+8,size)
backup=root/'InputBackup';backup.mkdir(exist_ok=True)
backup_exe=backup/'toy2.exe'
assert not backup_exe.exists(),'Backup already exists: inspect before reapplying'
backup_exe.write_bytes(original)
exe.write_bytes(patched)
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
print('Original SHA256:',hashlib.sha256(original).hexdigest())
print('Patched SHA256:',hashlib.sha256(patched).hexdigest())
for i in md.disasm(code,cave):print(hex(i.address),i.mnemonic,i.op_str)
assert bytes(patched[hook_off:hook_off+1])==b'\xe9'
assert code[7:9]==b'\x74\x07'
print('Verified: Escape scan code 1 maps to Return scan code 28; original input handling resumes.')
