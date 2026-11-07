exec(open('tools/inspect_keys.py').read().split('for s in')[0])
for a,n in [(0x414b98,7),(0x404ea1,10),(0x404031,6)]:print(hex(a),p.get_data(a-base,n).hex())
