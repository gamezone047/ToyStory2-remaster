"""Read-only, reproducible evidence for the native co-op integration investigation.

Run from any directory with py -3.9. Writes only native-audit.json beside this file.
This is a linear-disassembly index, not a complete call graph or safe patch plan.
Indirect references, dynamically generated code and pointer aliases are not resolved.
"""
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(ROOT / 'tools' / 'python-libs'))
import pefile
import capstone
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM

binary = (ROOT / 'toy2.exe').read_bytes()
expected = '4d1abb8f801120a4cd69ea669066974be56d02d6c23bd8ab4db7b32d6125022e'
digest = hashlib.sha256(binary).hexdigest()
if digest != expected:
    raise SystemExit('Unrecognized executable: audit addresses must be re-established.')
pe = pefile.PE(data=binary)
base = pe.OPTIONAL_HEADER.ImageBase
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True
md.skipdata = True
regions = {
    'movement_region': (0x436220, 0x436d40),
    'player_enemy_contact': (0x407440, 0x4076ed),
    'render_room_selection_excerpt': (0x4402b0, 0x440450),
    'active_game_loop': (0x49dfe0, 0x49e328),
}
report = {'executable_sha256': digest, 'limitations': __doc__, 'regions': {},
          'player_references': [], 'render_camera_references': []}
for name, bounds in regions.items():
    report['regions'][name] = {'start': hex(bounds[0]), 'end_exclusive': hex(bounds[1]),
                               'direct_calls': [], 'static_memory_operands': []}
for section in pe.sections:
    if not section.Characteristics & 0x20000000:
        continue
    for ins in md.disasm(section.get_data(), base + section.VirtualAddress):
        if ins.id == 0:
            continue
        site = {'address': hex(ins.address), 'bytes': ins.bytes.hex(),
                'instruction': ins.mnemonic + ' ' + ins.op_str}
        for op in ins.operands:
            address = None
            if op.type == X86_OP_IMM:
                address = op.imm & 0xffffffff
            elif op.type == X86_OP_MEM:
                address = op.mem.disp & 0xffffffff
            if address is not None:
                if 0x52f300 <= address < 0x52f39c:
                    report['player_references'].append(dict(site, target=hex(address)))
                if 0x52adc0 <= address < 0x52add4:
                    report['render_camera_references'].append(dict(site, target=hex(address)))
            for name, (start, end) in regions.items():
                if not start <= ins.address < end:
                    continue
                entry = report['regions'][name]
                if ins.mnemonic == 'call' and op.type == X86_OP_IMM:
                    entry['direct_calls'].append(dict(site, target=hex(address)))
                if op.type == X86_OP_MEM and not op.mem.base and address is not None:
                    access = ('r' if op.access & capstone.CS_AC_READ else '')
                    access += ('w' if op.access & capstone.CS_AC_WRITE else '')
                    entry['static_memory_operands'].append(dict(
                        site, target=hex(address), size=op.size, access=access or 'unknown',
                        indexed=bool(op.mem.index)))
output = HERE / 'native-audit.json'
output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print('Saved', output)
print('Player references:', len(report['player_references']))
print('Render camera references:', len(report['render_camera_references']))
print('Static evidence only; no executable/plugin modifications and no game launch.')
