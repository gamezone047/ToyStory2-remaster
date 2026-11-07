exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for a in [0x5028a8,0x5028c8]:print(hex(a),struct.unpack('<11h',p.get_data(a-base,22)))
