exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for va,target in [(0x4381c5,0x437c40),(0x4382cc,0x437fb0),(0x49dee9,0x453cf0),(0x49dbc6,0x414720),(0x49dc74,0x49dfe0),(0x441260,0x48f410),(0x4cdd3b,0x4bc410),(0x4cde6d,0x4bc410),(0x4cdf4a,0x4bc410)]:
 assert p.get_data(va-base,5)==b'\xe8'+struct.pack('<i',target-va-5),hex(va)
for va,h in [(0x40581c,'5f5e5d81c480000000'),(0x47e182,'ff511085c0'),(0x47e249,'ff521085c0'),(0x4b6a3e,'ff1524875000')]:
 assert p.get_data(va-base,len(bytes.fromhex(h)))==bytes.fromhex(h),hex(va)
print('PASS: camera, geometry, sky, sound status, autosave and menu native hook signatures')
