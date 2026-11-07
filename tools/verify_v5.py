exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for va,target in [(0x405976,0x4038e0),(0x41327a,0x49ae40),(0x4a3ed5,0x49ae40),(0x4a3edf,0x49ae20)]:
 data=p.get_data(va-base,5);assert data[0]==0xe8 and va+5+struct.unpack_from('<i',data,1)[0]==target,hex(va)
assert p.get_data(0x436a36-base,5)==bytes.fromhex('8b168b4604')
assert p.get_data(0x432020-base,6)==bytes.fromhex('81eca8020000')
print('PASS: new hook sites match installed executable; startup skip patches remain installed.')
