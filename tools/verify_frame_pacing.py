exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for va in (0x49bbe2,0x44190b,0x4419f6):
 assert p.get_data(va-base,5)==b'\xe8'+struct.pack('<i',0x490860-va-5),hex(va)
fix=pefile.PE('scripts/ToyStory2Fix.asi')
# Verify the installed routine identity and old round-up/wait instructions.
for rva,raw in {0x9610:'535657',0x9697:'42',0x976c:'69c71b410000'}.items():
 assert fix.get_data(rva,len(bytes.fromhex(raw)))==bytes.fromhex(raw),hex(rva)
print('PASS: all timing call sites and installed fix routine match the pacing replacement')
