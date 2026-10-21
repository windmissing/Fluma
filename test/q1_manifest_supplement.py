import json, hashlib, os
from PIL import Image

d = json.load(open('test/artifacts/p012-measurements/q1-manifest-media.json', encoding='utf-8'))
items = d if isinstance(d, list) else list(d.values())
have = {(it.get('path') or it.get('file')): it for it in items}
base = 'test/artifacts/p012-measurements/'

missing = ['dyn-s96-static-%d.jpeg' % i for i in range(8)] + [
    'eab-s99w-pair%d-%s-w%d.jpeg' % (pr, src, w)
    for pr in (1, 2, 3) for src in ('off', 'eab') for w in (0, 2)]


def quant_summary(img):
    q = img.quantization
    flat = []
    for k in sorted(q.keys()):
        flat.extend(q[k])
    return hashlib.sha256(json.dumps(flat, sort_keys=True).encode()).hexdigest()[:16]


added = 0
for fn in missing:
    p = base + fn
    if not os.path.exists(p):
        print('MISSING FILE:', fn)
        continue
    data = open(p, 'rb').read()
    img = Image.open(p)
    entry = {
        'path': p,
        'sha256': hashlib.sha256(data).hexdigest(),
        'sizeBytes': len(data),
        'width': img.width, 'height': img.height,
        'format': img.format,
        'quantTableSha16': quant_summary(img),
        'addedIn': 'v1.0-freeze-supplement (item 5 closure)'
    }
    items.append(entry)
    added += 1

with open('test/artifacts/p012-measurements/q1-manifest-media.json', 'w', encoding='utf-8') as f:
    json.dump(items, f, indent=1, ensure_ascii=False)
print('added', added, 'entries; total now', len(items))
s99w = [it for it in items if 's99w' in (it.get('path') or '')]
print('s99w quant tables:', set(it.get('quantTableSha16') for it in s99w))
s96 = [it for it in items if 'dyn-s96' in (it.get('path') or '')]
print('s96 quant tables:', set(it.get('quantTableSha16') for it in s96))
