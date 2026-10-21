import json

with open('test/artifacts/p011-v3pilot3-20261021/smoke2-161246/ledger.ndjson', encoding='utf-8', errors='replace') as f:
    lines = f.readlines()
good = lines[:-1]
json.loads(good[-1])
with open('test/artifacts/p011-v3pilot3-20261021/smoke2-161246/ledger-trimmed.ndjson', 'w', encoding='utf-8') as f:
    f.writelines(good)
print('trimmed to', len(good), 'records')
