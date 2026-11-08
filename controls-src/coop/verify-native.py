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
# MechanismSync.h: House trigger/vents and Backyard buttons, gate, board, platform, challenge.
for site,target in [(0x417a3f,0x4ccb20),(0x417a48,0x4ccff0),(0x417fee,0x417380),(0x4173d5,0x4878d0),(0x41741f,0x4878a0),
                    (0x419df7,0x4ccb20),(0x419e01,0x4ccb20),(0x419e4e,0x4ccb20),(0x419e58,0x4ccb20),(0x419ffa,0x4878a0),(0x41a001,0x4878d0),
                    (0x41a009,0x448080),(0x41a101,0x4ccb20),(0x41a10b,0x4ccb20),(0x41a253,0x4878d0),(0x41a25a,0x4878d0),(0x41a261,0x4878a0),
                    (0x41a2d0,0x4ccc70),(0x41a2df,0x4ccc70),(0x41a833,0x4cce30),(0x41a87d,0x4ccb20)]:
    assert read(site,5)==b'\xe8'+struct.pack('<i',target-site-5),hex(site)
for site,code in [(0x4179fe,'c705a0f5520008000000'),(0x418f66,'c705c8f6520000100000'),(0x418f70,'c705ccf6520000100000'),
                  (0x41a47e,'a1f8024e00'),(0x41a497,'a398d85200'),(0x41a83e,'b8ef22dd17'),(0x41a846,'8d8a6c820500'),
                  (0x41a785,'897e0c'),(0x41a7aa,'8b7e10'),(0x41a811,'894e04')]:
    assert read(site,len(code)//2)==bytes.fromhex(code),hex(site)
anm=(root/'data/chars/buzz.anm').read_bytes()
assert hashlib.sha256(anm).hexdigest()=='b4911ea64cfeb2f158e6c9b820086e693f3ddeda8d406ffd3fccf9fa4a5ea7a3'
count=struct.unpack_from('<H',anm,2)[0];assert count==35
frames=[struct.unpack_from('<H',anm,struct.unpack_from('<I',anm,8+i*4)[0]+4)[0]&32767 for i in range(count)]
assert frames==[24,24,16,16,17,18,14,24,25,24,42,8,18,30,12,12,38,15,19,24,24,12,17,13,24,24,19,17,23,58,48,32,24,24,19]
print('PASS: native hook signatures, level mechanism sites, registry imports, coin dispatch/removal, render list/matrix stride, Buzz animation bounds.')
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

def code(pairs):
    for site,hexbytes in pairs:
        assert read(site,len(hexbytes)//2)==bytes.fromhex(hexbytes),hex(site)
def calls(pairs):
    for site,target in pairs:
        assert read(site,5)==b'\xe8'+struct.pack('<i',target-site-5),hex(site)
# ScriptSync.h level 3 boss: hit = health differs from 52f730, +0x800 damage,
# type-0x3f rearm loop, 0x3000 reveal of mesh 0x32/pickup 0, phases 3e7/3e8/3e9.
code([(0x41ac6e,'0fbf15bec85200'),(0x41ac75,'a130f75200'),(0x41acc0,'0500080000'),(0x41acc5,'a3f0f65200'),
      (0x41accf,'8078093f'),(0x41acd8,'83c03c'),(0x41acdb,'3d7cad5200'),(0x41ace2,'813df0f6520000300000'),(0x41ad1a,'894108'),
      (0x41ab20,'bbe7030000'),(0x41abba,'891d54f75200'),(0x41abad,'8b0dfc024e00'),(0x41ad8e,'c70554f75200e8030000'),
      (0x41addd,'0520d1ffff'),(0x41ade8,'a3f4f65200'),(0x41b029,'c70554f75200e9030000'),(0x41a8d1,'a330f75200'),(0x41a9b8,'c7400800000080')])
calls([(0x41ad07,0x4ccb20),(0x41b043,0x405d20)])
assert read(0x41ad05,2)==bytes.fromhex('6a32')
print('PASS: level 3 boss hit/damage/reveal/rearm/phase signatures.')
# Level 6 boss: changed nonzero health vs 52f98c is a hit, <=10 defeat (phase 3),
# local position trigger x<0x31f9e, phase 2 after cutscene, removal + level complete.
code([(0x420085,'66a1bec85200'),(0x420094,'8b0d8cf95200'),(0x4200ab,'a38cf95200'),(0x4200b4,'66833dbec852000a'),
      (0x42011e,'c705a4f9520003000000'),(0x420128,'c680d7f0520080'),(0x420242,'813d00f352009e1f0300'),(0x42026e,'891da4f95200'),
      (0x42043c,'ba02000000'),(0x4207e9,'891dc40c8300'),(0x4207ef,'8915a4f95200'),(0x41ffd4,'a38cf95200')])
calls([(0x4207ce,0x405d20)])
print('PASS: level 6 boss hit/defeat/trigger/phase signatures.')
# Level 12 boss: local trigger x > -0x18b2b, phases 1/2/3/4, phase-2 hit vs 52fe00, defeat < 10.
code([(0x42b5be,'813d00f35200d574feff'),(0x42b5e7,'893524fe5200'),(0x42b72d,'893d24fe5200'),(0x42b7c1,'0fbf15bec85200'),
      (0x42b7c8,'a100fe5200'),(0x42b815,'891500fe5200'),(0x42b829,'66833dbec852000a'),(0x42b84f,'c70524fe520003000000'),
      (0x42b8f9,'c680d7f0520080'),(0x42bd8a,'c705c40c830001000000'),(0x42bd94,'c70524fe520004000000'),(0x42b317,'a300fe5200')])
print('PASS: level 12 boss trigger/hit/defeat/phase signatures.')
# Level 4 progress: three targets (colliders 0x16..0x18 -> 488aa0 + mesh 0x41..0x43),
# sequence rounds == 7 -> token 3, actor 24 stage command 4e030c+0x18 -> token 4.
code([(0x41dad0,'f60584f7520001'),(0x41dad9,'6a16'),(0x41db38,'6a17'),(0x41db93,'6a18'),(0x41daeb,'6880feffff'),(0x41db00,'6a41'),(0x41db5f,'6a42'),(0x41dbba,'6a43'),
      (0x41db0f,'83ce01'),(0x41db6e,'83ce02'),(0x41dbc9,'83ce04'),(0x41d4bb,'833d70f7520007'),(0x41d4c4,'391d000d8300'),
      (0x41d7e2,'c70504f8520001000000'),(0x41d810,'a10c034e00'),(0x41d815,'c70504f8520002000000'),(0x41d81f,'83c018'),
      (0x41d829,'a360d75200'),(0x41d82e,'c705440d8300b4000000'),(0x41d8a0,'c70504f85200c8000000'),(0x41d83f,'66391df4d65200')])
calls([(0x41daf2,0x488aa0),(0x41db51,0x488aa0),(0x41dbac,0x488aa0),(0x41db02,0x4ccc70),(0x41db61,0x4ccc70),(0x41dbbc,0x4ccc70),(0x41d4cf,0x4a0db0),(0x41d898,0x4a0db0)])
print('PASS: level 4 target, sequence-round and actor-24 stage signatures.')
# Rescued-friend reward stages (levels 5,7,8,10,11,13,14; level 4 above): stage 1 on
# dialogue, stage 2 starts the friend command [base]+0x1c, timer 0, descriptor+0xf 0.
code([(0x41f834,'8b1534034e00'),(0x41f83f,'83c21c'),(0x41f84c,'8935e8f85200'),(0x41f852,'891594ca5200'),(0x41f858,'66891d8cca5200'),(0x41f85f,'c6400f00'),
      (0x422586,'a124034e00'),(0x422591,'83c01c'),(0x42259e,'893d14fa5200'),(0x4225a4,'a3c0c85200'),(0x4225a9,'668935b8c85200'),(0x4225b0,'c6410f00'),
      (0x4242cc,'8b0d50034e00'),(0x4242d8,'83c11c'),(0x4242e5,'c705e0fa520002000000'),(0x4242ef,'890d20e15200'),(0x4242f5,'66893d18e15200'),(0x4242fc,'c6420f00'),
      (0x424305,'66c7052ae152009001'),(0x42430e,'c7059cfb520058020000'),
      (0x427dd3,'8b0d30034e00'),(0x427ddf,'83c11c'),(0x427dec,'c7054cfc520002000000'),(0x427df6,'890da0cd5200'),(0x427dfc,'66892d98cd5200'),(0x427e03,'c6420f00'),
      (0x42ac21,'8b1548034e00'),(0x42ac2c,'83c21c'),(0x42ac39,'c70540fd520002000000'),(0x42ac43,'891574cf5200'),(0x42ac49,'66c7056ccf52000000'),(0x42ac52,'c6400f00'),
      (0x42d1d7,'8b1568034e00'),(0x42d1e2,'83c21c'),(0x42d1ef,'c7051cff520002000000'),(0x42d1f9,'891540dc5200'),(0x42d1ff,'66891d38dc5200'),(0x42d206,'c6400f00'),
      (0x42ed86,'8b1560034e00'),(0x42ed91,'83c21c'),(0x42ed9e,'893dacff5200'),(0x42eda4,'8915c8e45200'),(0x42edaa,'66891dc0e45200'),(0x42edb1,'c6400f00'),
      (0x41f80c,'c705e8f8520001000000'),(0x4242a4,'c705e0fa520001000000'),(0x427daa,'c7054cfc520001000000'),(0x42d1b3,'c7051cff520001000000'),(0x42ed62,'c705acff520001000000')])
calls([(0x41f8ac,0x4a0db0),(0x4225da,0x4a0db0),(0x424344,0x4a0db0),(0x427f64,0x4a0db0),(0x42ac9b,0x4a0db0),(0x42d22f,0x4a0db0),(0x42eddb,0x4a0db0)])
print('PASS: rescued-friend reward stage signatures for levels 5,7,8,10,11,13,14.')
# Level 9 boss: hit count 52fbd4 (init 1, ++ when both minions have health 0),
# pair table 4f2f58 -> 52fbc8/52fbc4, expected health 52fbb8, release timer 52fbc0,
# phases 1/3/4/5, owner-cutscene spawn 406cd0 / opening 405cf0 / removal 405d20.
code([(0x4243e6,'c705d4fb520001000000'),(0x4248a3,'8b0dd4fb5200'),(0x4248d1,'8b148d5c2f4f00'),(0x4248e1,'8915c8fb5200'),(0x4248ee,'890dc4fb5200'),
      (0x4248fc,'8915b8fb5200'),(0x424a08,'8b0dc0fb5200'),(0x424a62,'c70504fc520003000000'),(0x424ae7,'813d08f35200a844feff'),(0x424b14,'893d04fc5200'),
      (0x425022,'66393495bec85200'),(0x42505e,'a3d4fb5200'),(0x4249de,'c70504fc520004000000'),(0x42512e,'893d04fc5200'),(0x42514d,'c705c40c830001000000'),
      (0x424d5e,'bbf00a0000'),(0x424d63,'bf0080ffff'),(0x424d79,'66899882c85200')])
calls([(0x424d32,0x406cd0),(0x424d4d,0x406cd0),(0x424d17,0x405cf0),(0x424fcb,0x405d20),(0x42511b,0x405d20)])
assert read(0x424d0c,2)==bytes.fromhex('6a17') and read(0x424d14,2)==bytes.fromhex('6a01')
assert list(struct.unpack('<13i',read(0x4f2f58,52)))==[-1,8,8,7,7,8,9,10,7,11,8,11,7]
print('PASS: level 9 boss hit-count, minion-pair table, phase and spawn/removal signatures.')
# Level 15 boss: per-actor callbacks (spawn switch 40700d/406faa/407044) defeat at
# health < 10 while the fight flag is 2 and advance 530064; tick 3 -> 4 -> 5.
assert struct.unpack('<I',read(0x40700d+6,4))[0]==0x42f310 and struct.unpack('<I',read(0x406faa+6,4))[0]==0x42f530 and struct.unpack('<I',read(0x407044+6,4))[0]==0x42f7b0
code([(0x42f337,'890d28005300'),(0x42f4ad,'66837e7e0a'),(0x42f4f1,'c7050800530003000000'),(0x42f4fe,'a364005300'),(0x42f505,'891df4ff5200'),
      (0x42f552,'0fbf4e7e'),(0x42f742,'66837e7e0a'),(0x42f74a,'833d0400530002'),(0x42f78d,'a364005300'),(0x42f7d8,'890d2c005300'),(0x42fa0f,'66837e7e0a'),(0x42fa59,'a364005300'),
      (0x4301e0,'a308005300'),(0x4301e5,'a30c005300'),(0x4301ea,'a304005300'),(0x4305bf,'c7056400530004000000'),(0x430662,'893564005300')])
print('PASS: level 15 boss callback defeat, fight-flag and defeat-step signatures.')
# ColliderSync.h: moving collider groups. Collider entries 728720+i*0x34 (+0x30 geometry,
# +0x0c rotation, +0x14 velocity, +0x24 rotation delta, +0x32 flags), geometry 7295a8+g*0x34;
# 488510 places a collider; standing collider 559e80; level 4/5/13 group globals.
code([(0x48852a,'8b4c2410'),(0x488533,'8bb8a8957200'),(0x488565,'8988a8957200'),(0x488570,'8988ac957200'),(0x488576,'8990b0957200'),
      (0x41c73e,'8b0df8f75200'),(0x41c760,'a3f8f75200'),(0x41c765,'a1809e5500'),(0x41c7c1,'8b15fcf75200'),(0x41c8b5,'8b0d08f85200'),(0x41c8d2,'a308f85200'),
      (0x41c2e2,'c70574f85200a41b4f00'),(0x41c2fe,'c70570f85200c81b4f00'),(0x41c31a,'c7057cf85200f41b4f00'),(0x41c336,'c70578f852001c1c4f00'),(0x41c42e,'b8641c4f00'),
      (0x41cb97,'833d1cb8520024'),(0x41cba4,'c705e0f7520001000000'),(0x41e8c7,'a1809e5500'),(0x41ea1c,'8b0d88f95200'),(0x41ea50,'a388f95200'),(0x41ea5a,'a33cf95200'),
      (0x42ca77,'a120ff5200'),(0x42ca9b,'a320ff5200'),(0x42cac9,'a324ff5200')])
calls([(0x41c75b,0x49ec00),(0x41c7d8,0x49ec00),(0x41c851,0x49ec00),(0x41c8cd,0x49ec00),(0x41c78a,0x48acc0),(0x41c805,0x48acc0),(0x41c87f,0x48acc0),(0x41c8fc,0x48acc0),
       (0x41c946,0x48acc0),(0x41c95e,0x48acc0),(0x41c976,0x48acc0),(0x41c991,0x48acc0),(0x41e8e4,0x41e150),(0x41e8f7,0x41e150),(0x41e936,0x41e150),(0x41e94a,0x41e150),
       (0x41e98b,0x41e150),(0x41e9a1,0x41e150),(0x41e9e6,0x41e150),(0x41e9fb,0x41e150),(0x41ea34,0x49ec00),(0x41ea55,0x49ec00),(0x42ca92,0x49ec00),(0x42cac1,0x49ec00)])
print('PASS: moving collider group signatures for levels 4, 5 and 13.')
# Levels 7, 8, 10, 11 mechanism groups and one-way switch variables.
code([(0x421457,'c70524fa52008c0a0000'),(0x4214f2,'c705e4f9520001000000'),(0x421bea,'c7054cfa5200ffffffff'),(0x421c95,'c705e8f9520020000000'),
      (0x422360,'c70510fa520000400000'),(0x4236b6,'c705e4fa52003c000000'),(0x423764,'ff248d64434200'),(0x427a74,'f60514fc520001'),
      (0x427ab3,'893514fc5200'),(0x427b09,'893514fc5200'),(0x42ab98,'893554fd5200'),(0x42abb0,'893538fd5200'),(0x42a6cc,'83f801'),(0x42a767,'83f801'),
      (0x427a8d,'6880010000'),(0x427ae0,'6880010000'),(0x427a92,'6a04')])
calls([(0x421424,0x48b640),(0x421441,0x488aa0),(0x421e80,0x48acc0),(0x421e98,0x48acc0),(0x421eb0,0x48acc0),(0x421f99,0x48acc0),(0x421fbd,0x48acc0),(0x421fe1,0x48acc0),
       (0x4273b9,0x48acc0),(0x427785,0x48acc0),(0x427a7f,0x48b640),(0x427a94,0x488aa0),(0x427ad4,0x48b640),(0x427ae9,0x488aa0),(0x42a6da,0x488aa0),(0x42a775,0x488aa0),
       (0x42177f,0x4878a0),(0x4217aa,0x4878d0)])
assert read(0x4878c0,9)==bytes.fromhex('66c70495d495720008')
print('PASS: level 7, 8, 10 and 11 mechanism group and switch signatures.')
# Level 5 button bit and level 13 platform records (0x20-byte stride, collider at +0x10).
code([(0x41eb39,'f60538f9520001'),(0x41eb7a,'890d38f95200'),(0x41eb94,'05100e0000'),(0x42c3f0,'8bf7'),(0x42c3f2,'c1e605'),(0x42c470,'8b9658fe5200'),(0x42c449,'899e48fe5200')])
calls([(0x41eba6,0x488510),(0x41ebbc,0x4ccb20),(0x41ebd2,0x4ccb20),(0x42ce3c,0x42c3e0),(0x42ce45,0x42c3e0),(0x42ce4e,0x42c3e0),(0x42ce5b,0x42c3e0),(0x42ce64,0x42c3e0),(0x42c47c,0x488510)])
print('PASS: level 5 button and level 13 platform record signatures.')
# Level 14 lamp puzzle: 52ffd0 lamps/target nibbles, 52ff70 moves, 52ffb4 first-visit scramble,
# landing on surfaces 0x20..0x23 presses 42d710; lamp redraw helpers 42d8b0/42d9c0; door 52ff7c.
code([(0x42e7e5,'391db4ff5200'),(0x42e7f8,'c705b4ff520001000000'),(0x42e806,'83e20f'),(0x42e809,'25f0000000'),(0x42e99f,'833d4cc65300d8'),
      (0x42e9b2,'83f920'),(0x42e9bb,'83f923'),(0x42e9c4,'48'),(0x42e9ca,'a370ff5200'),(0x42d750,'a3d0ff5200'),(0x42dc4e,'c70570ff520003000000'),
      (0x42dc63,'8915d0ff5200'),(0x42e877,'813d7cff520080bb0000'),(0x42d8b0,'a0d0ff5200')])
calls([(0x42e7ed,0x42db10),(0x42ea3b,0x42d710),(0x42ea43,0x42d8b0),(0x42e849,0x42d8b0),(0x42e84e,0x42d9c0),(0x42dc69,0x42d8b0),(0x42dc6e,0x42d9c0)])
print('PASS: level 14 lamp puzzle signatures.')
# Level 11 paint can: colour 52fd14 via 428890 (blink bit 0x800 in 428ba0), level buttons 52fd3c via
# 4292c0 and target 52fd30 = -1600*n (table 4f3fcc), fill 52fdfc, pour switches (table 4f3fb4, 429910).
code([(0x42a2df,'892d74fd5200'),(0x42a36a,'891530fd5200'),(0x429af7,'c7073c000000'),(0x4288ed,'891d14fd5200'),(0x428bc6,'80cd08'),(0x42a29e,'83f8d8'),(0x42a202,'a3fcfd5200'),
      (0x42a355,'8bc1f7d8c1e0022bc18d1480')])
calls([(0x42a317,0x428890),(0x42a34a,0x4292c0),(0x42a37e,0x429910),(0x42a383,0x428ba0),(0x42a28c,0x428c80),(0x429aea,0x488510)])
assert read(0x4f3fcc,16)==bytes.fromhex('07100003082014000640280109804802')
assert read(0x4f3fb4,24)==bytes.fromhex('020b3009113830071239530805053004 0b073006 0a063005'.replace(' ',''))
print('PASS: level 11 paint can, level buttons and pour switch signatures.')
# Level 4 crumbling platforms 41bc20 (x3): local proximity 0x480, collider on/off, mesh POS, fall to 0xc800.
calls([(0x41cbdd,0x41bc20),(0x41cbfa,0x41bc20),(0x41cc17,0x41bc20),(0x41bcaf,0x4878a0),(0x41bcc1,0x4878d0),(0x41bc9b,0x4cce30)])
code([(0x41bc33,'6880040000'),(0x41bc72,'83f832'),(0x41bd47,'3d00c80000'),(0x41cbce,'68acf75200'),(0x41cbd3,'68b8f75200'),(0x41cbd8,'68a4f75200')])
print('PASS: level 4 crumbling platform signatures.')
# Campaign audit 2026.11.7.13: level 5 bubble rides (phase 52f8f8, states 52f91c/52f92c, carry 53c668 -> local
# position), level 7 press velocity 52f9bc, level 8 crane prize 52fb08/52fb74/52fb18 (pickup +0xd |= 0x28), level 10
# floor switches (surface read, counters 52fc50/52fc30/52fcd8 -> 52fc14 bit 4, collider 0x13 off, mesh helper 425680),
# level 11 route toy 428e70 (node 52fd4c, collider 0x1b on), floating platforms 429fb0 x4, pour countdown -1 + 429800 +
# turret bits 52fd48, local bits 0x400 (52fd14) / 0x200 (52fd3c), level 14 carousel 42dcb0 (collider 0 rotation, 52ff84).
code([(0x41ed81,'a3f8f85200'),(0x41eda1,'892d1cf95200'),(0x41edda,'892d2cf95200'),(0x41f011,'a310f95200'),(0x41f101,'891500f35200'),(0x41f0c2,'391d68c65300'),
      (0x421bbb,'890dbcf95200'),(0x42366c,'c70518fb520001000000'),(0x4239bf,'a374fb5200'),(0x423916,'a308fb5200'),(0x423b56,'80cb28'),(0x426f83,'a11cb85200'),
      (0x427047,'8b2d14fc5200'),(0x42704f,'83cd04'),(0x42704d,'6a13'),(0x4256a8,'a130fc5200'),(0x42701c,'8b0d50fc5200'),(0x4272ac,'f60514fc520004'),(0x4292b0,'6a1b'),
      (0x42900c,'890d4cfd5200'),(0x42a878,'89ae58fd5200'),(0x42a85a,'83cdff'),(0x42a896,'a348fd5200'),(0x429293,'80cc04'),(0x42943b,'80cd02'),(0x42dceb,'892d84ff5200')])
calls([(0x421bcf,0x487900),(0x423c47,0x4cce30),(0x427058,0x4878d0),(0x4272a7,0x425680),(0x42a4ff,0x428e70),(0x42a4af,0x429fb0),(0x42a4c7,0x429fb0),(0x42a4df,0x429fb0),
       (0x42a4f7,0x429fb0),(0x4292b2,0x4878a0),(0x42a87e,0x429800),(0x42e7a1,0x42dcb0),(0x42dcd6,0x487970),(0x42dcbe,0x487a20)])
print('PASS: campaign audit signatures (levels 5, 7, 8, 10, 11, 14).')
# World sync 2026.11.7.14: token take-back 4a0e60 (descriptor active 0, slot Y empty) called by the timed
# challenges of levels 7/10/14; pickup award skip resumes at the native removal 4a1669 -> 4a166d; native
# respawn of dead actors with a delay (4086f0: 0x7a < 0x1388, off-camera 448f00, 406cd0(actor,0)); mode 2
# keeps the model while +0x7a != 0; level 7 slot 10/11 removals; respawn reads descriptor +0x11.
code([(0x4a0e6d,'c780d00c830000000000'),(0x4a0e7d,'c70100000080'),(0x4a1669,'896c2410'),(0x4a166d,'8b442414'),
      (0x408886,'663d8813'),(0x420587,'a100f35200'),(0x42058c,'a39cc85200'),(0x420591,'a108f35200'),(0x420596,'a3a4c85200'),(0x40884d,'668b463a'),(0x406139,'66395e7a'),(0x40613f,'66895e14'),(0x406d4c,'660fb65111'),(0x406d51,'6689507a')])
calls([(0x408879,0x406cd0),(0x408864,0x448f00),(0x42248f,0x4a0e60),(0x428151,0x4a0e60),(0x42ecf4,0x4a0e60),(0x42223d,0x405d20),(0x42228c,0x405d20)])
print('PASS: world sync signatures (token take-back, personal pickup removal, actor respawn, level 7 removals).')
