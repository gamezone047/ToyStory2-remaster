exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for a in range(0x502164,0x502188,4):print(hex(a),p.get_string_at_rva(a-base))
