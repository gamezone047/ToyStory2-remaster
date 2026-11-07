exec(open('tools/inspect_keys.py').read().split('for s in')[0])
va=0x441252
assert p.get_data(va-base,5)==b'\xe8'+struct.pack('<i',0x48f230-va-5)
sites={0x48f2bd:'d91dd41c7300',0x48f2f5:'a34c0c8300',
       0x48f33b:'d915500c8300',0x48f5c8:'e8f3960200',
       0x44124e:'f7d85051',0x48f2dd:'d8e9',0x48f335:'d805241f7300'}
for va,h in sites.items():
 assert p.get_data(va-base,len(bytes.fromhex(h)))==bytes.fromhex(h),hex(va)
print('PASS: sky update call, panorama width, yaw history and scroll accumulator verified')
