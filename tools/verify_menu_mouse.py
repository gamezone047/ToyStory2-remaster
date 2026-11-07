exec(open('tools/inspect_keys.py').read().split('for s in')[0])
sites={0x4413a8:'3bc10f8d6c020000',0x49b580:'558b6c240c',0x401fb0:'8b44241833c9',0x49d750:'8b44240c8b4c2408',0x493f40:'668b44240c',0x452198:'66890d9c278800',0x49b2a0:'538b5c240c',0x494820:'0fbf44240c'}
for va,h in sites.items(): assert p.get_data(va-base,len(bytes.fromhex(h)))==bytes.fromhex(h),hex(va)
print('PASS: menu trampolines and coin distance hook match the installed executable')
