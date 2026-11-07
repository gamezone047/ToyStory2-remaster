"""Verify hook signatures against disk, without starting the game."""
import re, sys
sys.path.insert(0, 'tools/python-libs')
import pefile
p = pefile.PE('toy2.exe')
sites = {
    0x447c3d: '0f8cde000000',
    0x447c4e: '0f84cd000000',
    0x447c8e: '7d6d',
    0x404704: '0f8e11050000',
    0x404712: '0f8503050000',
    0x4087b3: '7d10',
    0x408831: '0f8d86000000',
    0x4087b3: '7d10',
    0x408831: '0f8d86000000',
    0x403990: '890dfca45000',
    0x414af0: 'a13c9d5200',
    0x404aaa: '0fbf562e8b1d28a15000',
    0x404ea1: '0fbf4e2e66816628ff0f',
    0x4053b1: '0fbf562e0fbf7e26',
    0x4052be: 'f6c10c0f8590000000',
}
for va, reg in re.findall(r'\{(0x[0-9a-f]+),(\d),', open('controls-src/NpcSites.h').read()):
    sites[int(va, 16)] = ['2500020000', '81e100020000', '81e200020000'][int(reg)]
for va, hexbytes in sites.items():
    expected = bytes.fromhex(hexbytes)
    assert p.get_data(va - p.OPTIONAL_HEADER.ImageBase, len(expected)) == expected, hex(va)
import struct
for va in [0x49e30c, 0x49e5eb]:
    assert p.get_data(va-p.OPTIONAL_HEADER.ImageBase,5)==b'\xe8'+struct.pack('<i',0x49fd40-va-5)
for va,target in [(0x434cb1,0x4a5d30),(0x4a60d5,0x48c860),(0x4a60ea,0x4a5870)]:
    assert p.get_data(va-p.OPTIONAL_HEADER.ImageBase,5)==b'\xe8'+struct.pack('<i',target-va-5)
for va,target in [(0x434cb1,0x4a5d30),(0x4a60d5,0x48c860),(0x4a60ea,0x4a5870)]:
    assert p.get_data(va-p.OPTIONAL_HEADER.ImageBase,5)==b'\xe8'+struct.pack('<i',target-va-5)
p.close()
print('PASS: camera paths, aim transition, keyboard entry and all NPC hook signatures match installed executable.')
