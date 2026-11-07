exec(open('tools/inspect_keys.py').read().split('for s in')[0])
assert p.get_data(0x49f901-base,9)==bytes.fromhex('663bd30f8504feffff')
assert p.get_data(0x5026f0-base,11)==b'exit level\0'
assert p.get_data(0x5027b4-base,10)==b'exit game\0'
assert p.get_data(0x409378-base,6)==bytes.fromhex('ff159cf5e400')
print('PASS: pause confirmation hook, both labels and normal window shutdown verified')
