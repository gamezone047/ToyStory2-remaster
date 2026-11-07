exec(open('tools/inspect_keys.py').read().split('for s in')[0])
start=int(sys.argv[1],16); end=int(sys.argv[2],16)
for i in md.disasm(p.get_data(start-base,end-start),start):print(hex(i.address),i.mnemonic,i.op_str)
