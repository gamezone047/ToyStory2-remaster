"""Call-graph audit of the campaign level scripts. Evidence index, not synchronization proof.
Run from the project root: py controls-src/coop/audit-callgraph.py [game folder with toy2.exe]
For every level: tick (table 4a1b68), level start code (table 4a1ab8), actor callbacks installed by
the spawn switch (actor +0x94) and every level-local function they call. Lists, per function, the
level globals it writes, local-player conditions and mechanism calls, then the written globals that
no adapter header (controls-src/coop/*.h) mentions. Each of those must be classified
by hand (CAMPAIGN-COVERAGE.txt).
"""
from pathlib import Path
import re,struct,sys
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/python-libs'))
import pefile,capstone
from capstone.x86_const import X86_OP_IMM,X86_OP_MEM
game=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root
raw=(game/'toy2.exe').read_bytes();pe=pefile.PE(data=raw);base=0x400000
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);md.detail=True
rd=lambda va,n:pe.get_data(va-base,n)
u32=lambda va:struct.unpack('<I',rd(va,4))[0]
def thunk(a):
    ins=next(md.disasm(rd(a,5),a));return ins.operands[0].imm if ins.mnemonic=='jmp' else a
ticks={l:thunk(u32(0x4a1b68+(l-1)*4)) for l in range(1,16)}
inits={l:thunk(u32(0x4a1ab8+(l-1)*4)) for l in range(1,16)}
LO,HI=0x417000,0x431000
def walk(entry):
    pending=[entry];seen={}
    while pending:
        a=pending.pop()
        while a not in seen:
            ins=next(md.disasm(rd(a,15),a),None)
            if ins is None:break
            seen[a]=ins
            if ins.mnemonic.startswith('ret'):break
            if ins.mnemonic.startswith('j'):
                op=ins.operands[0]
                if op.type==X86_OP_IMM:pending.append(op.imm)
                elif op.type==X86_OP_MEM and op.mem.index and op.mem.scale==4 and not op.mem.base:
                    t=op.mem.disp&0xffffffff
                    for k in range(64):
                        v=u32(t+4*k)
                        if not(entry-0x4000<=v<entry+0x4000):break
                        pending.append(v)
                if ins.mnemonic=='jmp':break
            a+=ins.size
    return seen
text=[s for s in pe.sections if s.Name.startswith(b'.text')][0];code=text.get_data();va0=base+text.VirtualAddress
callbacks=set()
for i in range(6,len(code)-3):
    if code[i-6:i-2]==b'\xc7\x80\x94\x00':
        v=struct.unpack_from('<I',code,i)[0]
        if LO<=v<HI:callbacks.add(v)
def level_of(a):
    later=[l for l,t in ticks.items() if t>a];return min(later,key=lambda l:ticks[l]) if later else 15
API={0x49ec00:'balance',0x4ccc70:'mesh-rot',0x4ccb20:'mesh-scale',0x4cce30:'mesh-pos',0x488510:'col-pos',0x487900:'col-move',
     0x4878a0:'col-on',0x4878d0:'col-off',0x4a0db0:'token',0x406cd0:'spawn',0x405d20:'remove',0x487970:'col-rot',0x488aa0:'col-turn',
     0x48acc0:'path',0x48b640:'target',0x488580:'contact*',0x49f400:'near*',0x49f3c0:'near*',0x4071e0:'knockback',0x4020f0:'cutscene'}
LOCAL={0x52f300:'pos*',0x52f304:'pos*',0x52f308:'pos*',0x559e80:'standing*',0x52b81c:'surface*',0x53c64c:'landing*'}
WRITES=('mov','inc','dec','add','sub','or','and','xor','neg','not','shl','sar','shr')
src=''.join(p.read_text(encoding='utf-8') for p in sorted((root/'controls-src/coop').glob('*.h')))
covered=set(int(x,16) for x in re.findall(r'0x(5[23][0-9a-f]{4})',src))
for level in range(1,16):
    roots=[('tick',ticks[level]),('start',inits[level])]+[('callback',c) for c in sorted(callbacks) if level_of(c)==level]
    funcs={};queue=list(roots)
    while queue:
        kind,f=queue.pop(0)
        if f in funcs:continue
        seen=walk(f);w=set();local=set();api=set()
        for ins in seen.values():
            for k,op in enumerate(ins.operands):
                if op.type==X86_OP_MEM and not op.mem.base and not op.mem.index:
                    d=op.mem.disp&0xffffffff
                    if k==0 and ins.mnemonic in WRITES and 0x52f580<=d<0x530100:w.add(d)
                    if d in LOCAL:local.add(LOCAL[d])
            if ins.mnemonic=='call' and ins.operands[0].type==X86_OP_IMM:
                c=ins.operands[0].imm
                if c in API:api.add(API[c])
                if LO<=c<HI:queue.append(('helper of %x'%f,c))
        funcs[f]=(kind,w,local,api)
    print('== level %d  tick %x  start %x'%(level,ticks[level],inits[level]))
    pending=set()
    for f,(kind,w,local,api) in funcs.items():
        print('  %x %-16s writes %s | local %s | calls %s'%(f,kind,' '.join('%x'%x for x in sorted(w)) or '-',','.join(sorted(local)) or '-',','.join(sorted(api)) or '-'))
        if kind!='start':pending|={x for x in w if x not in covered}
    print('  not named by an adapter (classify by hand):',' '.join('%x'%x for x in sorted(pending)) or '-')
