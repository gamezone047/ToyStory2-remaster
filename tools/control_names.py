exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for a in range(0x500e58,0x500ec8,8):
 mask,ptr=struct.unpack('<II',p.get_data(a-base,8))
 print(hex(a),p.get_string_at_rva(ptr-base) if ptr>=base else hex(ptr),hex(mask))

