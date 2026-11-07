"""Check static native assumptions against the shipped executable and Buzz asset."""
from pathlib import Path
import hashlib
import struct
import sys
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/python-libs'))
if len(sys.argv)>1: root=Path(sys.argv[1]).resolve()
import pefile
data=(root/'toy2.exe').read_bytes()
pe=pefile.PE(data=data)
assert pe.OPTIONAL_HEADER.ImageBase==0x400000
# Manifest/resource-only updates may change the whole-file hash. Verify every
# other section, its address and the overlay instead of weakening hook checks.
fingerprint=hashlib.sha256()
for section in pe.sections:
    if section.Name.rstrip(b'\0')!=b'.rsrc':
        fingerprint.update(struct.pack('<I',section.VirtualAddress)+section.get_data())
fingerprint.update(pe.get_overlay() or b'')
assert fingerprint.hexdigest()=='cb2117c6487f5c3aeb64f03a292104e3398b4bc7323019f7deeedeedd1b26342'
def read(va,n):
    return data[pe.get_offset_from_rva(va-0x400000):][:n]
for site,target in [(0x4aee4d,0x4af110),(0x49e311,0x4a28b0),(0x49e15d,0x4a0f80),(0x49e153,0x4086f0),(0x4411ae,0x4cd880),(0x441285,0x4cdc20),(0x405d65,0x40fae0),(0x436a78,0x433700),(0x437ccd,0x4a1be0)]:
    assert read(site,5)==b'\xe8'+struct.pack('<i',target-site-5),hex(site)
imports={i.address:i.name for entry in pe.DIRECTORY_ENTRY_IMPORT for i in entry.imports}
assert imports[0xe4f3b8]==b'RegOpenKeyExA' and imports[0xe4f3b0]==b'RegQueryValueExA' and imports[0xe4f3b4]==b'RegCloseKey'
# Non-model pickups dispatch through default type 0x10 (+1 jump-table index).
assert read(0x4a1279,13)==bytes.fromhex('c744241400000000b810000000')
assert struct.unpack('<I',read(0x4a1a08+17*4,4))[0]==0x4a162a
assert read(0x4a168d,7)==bytes.fromhex('c7460400000080')
# Last-object special handling and matrix-array per-object stride.
assert read(0x4cd8cf,3)==bytes.fromhex('8b4f04')
assert read(0x4cdb7c,6)==bytes.fromhex('81c300080000')
anm=(root/'data/chars/buzz.anm').read_bytes()
assert hashlib.sha256(anm).hexdigest()=='b4911ea64cfeb2f158e6c9b820086e693f3ddeda8d406ffd3fccf9fa4a5ea7a3'
count=struct.unpack_from('<H',anm,2)[0];assert count==35
frames=[struct.unpack_from('<H',anm,struct.unpack_from('<I',anm,8+i*4)[0]+4)[0]&32767 for i in range(count)]
assert frames==[24,24,16,16,17,18,14,24,25,24,42,8,18,30,12,12,38,15,19,24,24,12,17,13,24,24,19,17,23,58,48,32,24,24,19]
print('PASS: native hook signatures, registry imports, coin dispatch/removal, render list/matrix stride, Buzz animation bounds.')
print('Static validation only; no game launched.')

assert read(0x4101ea,6)==bytes.fromhex('66a19ef35200')
# Pushable state is a pointer-free 40-byte table. Render and collision refresh calls.
assert read(0x433a0a,3)==bytes.fromhex('83c628')
for va,target in [(0x433c8e,0x4cce30),(0x433cac,0x488510)]:
    assert read(va,5)==b'\xe8'+struct.pack('<i',target-va-5)
print('PASS: dynamic drop, pushable movement/model/collision and automatic menu-start signatures.')

assert read(0x4a11be,8)==bytes.fromhex('8b46043d00000080')
assert read(0x4a1257,6)==bytes.fromhex('8a460c83c410')
print('PASS: collectible dispatch replay and native award hook signatures.')

assert read(0x4057ee,6)==bytes.fromhex('81ff00710200')
assert read(0x49e181,5)==b'\xe8'+struct.pack('<i',0x4a1b00-0x49e181-5)
assert struct.unpack('<I',read(0x4a1a08,4))[0]==0x4a165b
assert read(0x4176a4,7)==bytes.fromhex('66393d bec85200'.replace(' ',''))
print('PASS: wall minimum-distance rule, level script hook, ear dispatch and crib target health checks.')

# Ten level-specific quest actors use the same collection counter and removal ABI.
for site in [0x416aa5,0x41867f,0x41bc12,0x41df61,0x420f64,0x422d0b,0x425a68,0x4286ea,0x42c1eb,0x42d66c]:
    assert read(site,5)==b'\xe8'+struct.pack('<i',0x405d20-site-5)
assert read(0x47f065,6)==bytes.fromhex('f73d446f7200')
# Book advances two nodes into its fall; accepting the final native node matters.
assert read(0x433953,5)==bytes.fromhex('6683460a02')
tin=(root/'data/chars/tinman.anm').read_bytes()
assert struct.unpack_from('<H',tin,2)[0]==8
assert [struct.unpack_from('<H',tin,struct.unpack_from('<I',tin,8+i*4)[0]+4)[0]&32767 for i in range(8)]==[24,24,24,24,24,6,8,16]
print('PASS: all ten quest removal sites, terminal book node, CPU divide guard and Tin Robot animation limits.')

# Visibility repair duplicates this camera-relative native radius calculation.
assert read(0x408724,4)==bytes.fromhex('0fbf4602')
assert read(0x40872c,3)==bytes.fromhex('8d0c80')
assert read(0x408747,5)==bytes.fromhex('d1e7c1ff09')
assert read(0x4087b5,7)==bytes.fromhex('892c bdd0f15200'.replace(' ',''))
print('PASS: native enemy visibility range and candidate-list signatures.')

assert read(0x4cdc5e,5)==bytes.fromhex('845940745a')
print('PASS: native actor draw-enable bit gate.')

assert read(0x4183ba,5)==b'\xe8'+struct.pack('<i',0x49ec00-0x4183ba-5)
assert read(0x49ee36,5)==b'\xe8'+struct.pack('<i',0x487970-0x49ee36-5)
assert read(0x49ee3d,5)==b'\xe8'+struct.pack('<i',0x488bd0-0x49ee3d-5)
assert read(0x487a4e,7)==bytes.fromhex('0fbf8030877200')
print('PASS: attic balance plank native call, collision angle field and mesh synchronization signatures.')

for site,target in [(0x49dc74,0x49dfe0),(0x49dc7b,0x49e330)]:
    assert read(site,5)==b'\xe8'+struct.pack('<i',target-site-5),hex(site)
rc=(root/'data/chars/rc.anm').read_bytes()
assert struct.unpack_from('<H',rc,2)[0]==5
assert [struct.unpack_from('<H',rc,struct.unpack_from('<I',rc,8+i*4)[0]+4)[0]&32767 for i in range(5)]==[12,16,12,12,24]
print('PASS: both native pause/gameplay loop callsites and RC animation bounds.')

# Model animation count/table and clip-frame limit used by the native renderer.
assert read(0x4ca068,4)==bytes.fromhex('837e2c64')
assert read(0x4cdad9,3)==bytes.fromhex('8b4328')
assert read(0x4cd8ed,4)==bytes.fromhex('0fbf4614')
assert read(0x4cd1da,4)==bytes.fromhex('668b4604')
assert read(0x4a0dc9,10)==bytes.fromhex('c786d00c830001000000')
print('PASS: token reveal state and model-local animation table/count/frame bounds.')

# Single enemy-think entry and post-frustum repair callsite.
for site,target in [(0x40891b,0x4076f0),(0x4087ec,0x447bd0)]:
    assert read(site,5)==b'\xe8'+struct.pack('<i',target-site-5)
for site,axis in [(0x407a89,0),(0x407a92,2),(0x407af5,0),(0x407afd,2),(0x407c0b,2),(0x407c16,0),(0x407c68,0),(0x407c75,1),(0x407c7d,2),(0x407c8e,2),(0x407c96,0),(0x407cfd,0),(0x407d0a,2),(0x407d9a,2),(0x407da5,0),(0x407e0e,0),(0x407e1e,2),(0x407ee7,1)]:
    assert read(site,4)==struct.pack('<I',0x52f300+4*axis)
print('PASS: native target coordinate operands and post-cull activation hook.')

assert read(0x40615f,11)==bytes.fromhex('8b480483c004423bce75f5')
print('PASS: native actor-removal unbounded membership search signature (guarded for remote removal).')

assert read(0x418394,12)==bytes.fromhex('a1a0de540083c40c83f80575')
print('PASS: native attic room-five gate preceding balance simulation.')

# RC uses native contact geometry and impulse, with a bounded local actor list.
for site,target in [(0x49e175,0x407440),(0x4076bb,0x408a60)]:
    assert read(site,5)==b'\xe8'+struct.pack('<i',target-site-5)
assert read(0x407637,4)==bytes.fromhex('f6474080')
# Backyard hole script operates on slot 4, and chooses its next path node here.
assert read(0x419676,6)==bytes.fromhex('8935b0ca5200')
assert read(0x4195b9,6)==bytes.fromhex('8b0d789c5500')
assert read(0x4198cc,4)==bytes.fromhex('895c8a08')
assert read(0x4198e0,3)==bytes.fromhex('83f806')
print('PASS: RC native collision/impulse hooks and backyard hole selection/completion signatures.')

# Backyard duck/pump mesh and rider-carrying collider calls; tree fall colliders.
for site,target in [(0x419bb8,0x4ccb20),(0x419d6c,0x4ccb20),(0x419c14,0x4cce30),(0x419d0e,0x4879c0),(0x419d40,0x487900),(0x41a31b,0x4878d0),(0x41a322,0x4878a0),(0x41a3b6,0x4ccc70),(0x41a3cc,0x4ccc70)]:
    assert read(site,5)==b'\xe8'+struct.pack('<i',target-site-5),hex(site)
assert read(0x49dbb9,7)==bytes.fromhex('8b04958c265000')
assert read(0x43590d,6)==bytes.fromhex('393d58c65300')
assert read(0x435922,7)==bytes.fromhex('c7466c00fcffff')
assert read(0x43598e,10)==bytes.fromhex('c70568c65300ffffffff')
print('PASS: backyard duck/tree mesh and collision calls, level selection table, rope cooldown and jump-off signatures.')

# Verify argument order, not just call targets: both tree pieces rotate on Z.
assert read(0x41a3b1,5)==bytes.fromhex('5153536a1e')
assert read(0x41a3c7,5)==bytes.fromhex('5253536a1f')
print('PASS: both tree plank native meshes use Z-axis rotation arguments.')

# Backyard RC shares actor 29 but uses a different path/counter bank from House.
assert read(0x419225,5)==bytes.fromhex('a12cda5200')
assert read(0x4192db,5)==bytes.fromhex('a168f65200')
assert read(0x419430,3)==bytes.fromhex('83f840')
assert read(0x41943d,5)==bytes.fromhex('a368f65200')
print('PASS: backyard RC actor, path state and inclusive 64-node boundary.')

for site in (0x4190de,0x419103):
    assert read(site,5)==b'\xe8'+struct.pack('<i',0x49ec00-site-5)
print('PASS: both backyard native weight-driven balance platform hook sites.')

# Balance is deferred: it queues delta, physics commits angle later, then the
# mesh conversion reads CURRENT quarter-angle units. Never restore owner angle
# from the snapshot captured before the commit.
assert read(0x48798c,7)==bytes.fromhex('80885287720008')
assert read(0x4879ab,7)==bytes.fromhex('66899044877200')
assert read(0x488bdd,7)==bytes.fromhex('0fbf9030877200')
assert read(0x488beb,3)==bytes.fromhex('c1fa02')
assert read(0x488d62,7)==bytes.fromhex('66018130877200')
print('PASS: deferred platform rotation queue, later collider angle commit, and mesh quarter-angle conversion.')
