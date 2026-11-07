"""Copy original Pizza Planet art into unused gameplay HUD atlas space, staging only."""
from pathlib import Path
import re, struct, sys

root = Path(sys.argv[1]).resolve()
assert root != Path(__file__).resolve().parents[1], 'Never patch the installed game'

def bmp(data, name):
    found = list(re.finditer(name + b'BM', data))
    assert len(found) == 1, (name, len(found))
    off = found[0].end() - 2
    size = struct.unpack_from('<I', data, off+2)[0]
    result = bytearray(data[off:off+size])
    assert struct.unpack_from('<ii', result, 18) == (256,256)
    assert struct.unpack_from('<H', result, 28)[0] == 24
    assert struct.unpack_from('<I', result, 30)[0] == 0
    return off, result

def pixel_offset(image, x, y):
    return struct.unpack_from('<I', image, 10)[0] + (255-y)*768 + x*3

_, art = bmp((root/'data/level00/levelt1.ngn').read_bytes(), b'tex00')
count = 0
for path in (root/'data').glob('level*/*.ngn'):
    if path.parent.name == 'level00': continue
    data = bytearray(path.read_bytes())
    if b'tex31BM' not in data: continue
    off, atlas = bmp(data, b'tex31')
    for y in range(16):
        for x in range(24):
            src = pixel_offset(art, x*2, y*2)
            dest = pixel_offset(atlas, 160+x, 128+y)
            old = atlas[dest:dest+3]
            assert old in (bytearray(b'\x00\xff\x00'),art[src:src+3]), (str(path),x,y,old)
            atlas[dest:dest+3] = art[src:src+3]
    data[off:off+len(atlas)] = atlas
    path.write_bytes(data)
    assert len(data) == path.stat().st_size
    count += 1
assert count >= 15, count
print('PASS: original Pizza Planet pixels installed into', count, 'staged gameplay HUD atlases; no occupied pixels overwritten.')

