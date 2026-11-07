exec(open('tools/inspect_keys.py').read().split('for s in')[0])
import struct
sites={0x4a121b:'c1e705897c2438',0x4a1873:'a1ac0c8300',0x439319:'668b159c278800',0x4a18a9:'c705c40c830009000000'}
for va,h in sites.items(): assert p.get_data(va-base,len(bytes.fromhex(h)))==bytes.fromhex(h),hex(va)
for va,target in [(0x49de14,0x454020),(0x49de71,0x454020),(0x4397bd,0x49d750),(0x408bc7,0x405d20),(0x408942,0x405d20),(0x408a26,0x405d20),(0x49eb46,0x49ab90)]:
 assert p.get_data(va-base,5)==b'\xe8'+struct.pack('<i',target-va-5),hex(va)
print('PASS: all progression hook bytes match installed executable')
