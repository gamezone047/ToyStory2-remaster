exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for s in p.sections:print(s.Name,hex(s.VirtualAddress),hex(s.Misc_VirtualSize),hex(s.SizeOfRawData))
for a in range(0x529b30,0x529b80,8):print(hex(a),p.get_data(a-base,8).hex(),hex(p.get_offset_from_rva(a-base)))
