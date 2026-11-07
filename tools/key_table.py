exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for j in range(150):
 d=p.get_data(0x4efce0-base+j*6,6); scan=struct.unpack('<h',d[:2])[0]
 if scan==-1:break
 print(scan,repr(d))
for token in [0x1e,0x2e]:
 target=struct.unpack('<I',p.get_data(0x43165c-base+(token-0x19)*4,4))[0];print('token',hex(token),'target',hex(target))
