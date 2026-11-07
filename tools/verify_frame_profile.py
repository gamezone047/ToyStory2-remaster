exec(open('tools/inspect_keys.py').read().split('for s in')[0])
sites={0x4411b6:0x4909e0,0x441236:0x4b2d50,0x44127b:0x4cddd0,
0x441285:0x4cdc20,0x4418e9:0x4b6a50,0x4418f5:0x4b8460,
0x4418fa:0x4907e0,0x441901:0x4b2de0,
0x4cdd3b:0x4bc410,0x4cde6d:0x4bc410,0x4cdf4a:0x4bc410}
for va,target in sites.items():
 assert p.get_data(va-base,5)==b'\xe8'+struct.pack('<i',target-va-5),hex(va)
assert p.get_data(0x4bc410-base,14)==bytes.fromhex('d9442404d84c2404d91db0885000')
print('PASS: render/audio/presentation timing boundaries and native geometry-distance calls verified')
