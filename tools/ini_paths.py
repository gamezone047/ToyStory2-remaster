exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for a in [0x4f52e4,0x504d54,0x4f5314]:print(hex(a),p.get_string_at_rva(a-base))
