"""Inventory native level-tick control flow. Evidence index, not synchronization proof.
Run from project root. Reports direct native mechanism calls and player/global state
references; indirect calls and called actor scripts still require separate review.
"""
from pathlib import Path
import sys,struct,json,hashlib
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/python-libs'))
import pefile,capstone
from capstone.x86_const import X86_OP_IMM,X86_OP_MEM
raw=(root/'toy2.exe').read_bytes(); pe=pefile.PE(data=raw); base=pe.OPTIONAL_HEADER.ImageBase
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);md.detail=True
apis={0x49ec00:'balance platform',0x4ccc70:'mesh rotation',0x4ccb20:'mesh scale',0x4cce30:'mesh position',0x488510:'collider position',0x487900:'collider movement',0x4878a0:'enable collider',0x4878d0:'disable collider',0x4a0db0:'token reveal',0x406cd0:'actor removal',0x4027f0:'dialogue/event',0x488bd0:'collider-to-mesh transform'}
report={'executable_sha256':hashlib.sha256(raw).hexdigest(),'limitations':__doc__,'levels':[]}
for level in range(1,16):
 dispatch=struct.unpack('<I',pe.get_data(0x4a1b68-base+(level-1)*4,4))[0]
 jump=next(md.disasm(pe.get_data(dispatch-base,5),dispatch));entry=jump.operands[0].imm
 pending=[entry];seen=set();calls=[];state=set();player=[];unresolved=[]
 while pending:
  address=pending.pop()
  while address not in seen and 0x414000<=address<0x431000:
   seen.add(address);ins=next(md.disasm(pe.get_data(address-base,15),address),None)
   if ins is None:break
   for op in ins.operands:
    if op.type==X86_OP_MEM and not op.mem.base and not op.mem.index:
     a=op.mem.disp
     if 0x52f300<=a<0x52f3a0 or a in (0x54dea0,0x53c648,0x53c64c):player.append(hex(address))
     if 0x52f4f8<=a<0x53c000:state.add(hex(a))
   if ins.mnemonic=='call':
    if ins.operands[0].type==X86_OP_IMM:
     dest=ins.operands[0].imm
     if dest in apis:calls.append({'site':hex(address),'api':apis[dest],'target':hex(dest)})
    else:unresolved.append(hex(address))
   if ins.mnemonic.startswith('ret'):break
   if ins.mnemonic.startswith('j'):
    if ins.operands[0].type!=X86_OP_IMM:unresolved.append(hex(address));break
    pending.append(ins.operands[0].imm)
    if ins.mnemonic=='jmp':break
   address+=ins.size
 report['levels'].append({'level':level,'tick':hex(entry),'instructions':len(seen),'mechanism_calls':calls,'script_globals':sorted(state),'local_player_or_room_references':sorted(set(player)),'unresolved_control_flow':unresolved})
out=root/'controls-src/coop/campaign-audit.json';out.write_text(json.dumps(report,indent=2)+'\n')
for item in report['levels']:print('Level %2d: %4d instructions, %2d mechanism calls, %2d script globals, %2d player/room references, %d unresolved sites'%(item['level'],item['instructions'],len(item['mechanism_calls']),len(item['script_globals']),len(item['local_player_or_room_references']),len(item['unresolved_control_flow'])))
