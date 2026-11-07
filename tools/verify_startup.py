exec(open('tools/inspect_keys.py').read().split('for s in')[0])
print('Startup splash handler for screen 10:',hex(struct.unpack('<I',p.get_data(0x4384f4-base+9*4,4))[0]))
for va in [0x49d9bf,0x49d9cc]:
 print(hex(va),p.get_data(va-base,5).hex());assert p.get_data(va-base,5)==bytes.fromhex('b8 01 00 00 00')
print('Display config:',struct.unpack('<6I',open('toy2.cfg','rb').read()))
