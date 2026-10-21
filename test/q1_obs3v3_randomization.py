import json, random, hashlib, os

# Expansion randomization manifest (protocol §6, seed=20261019).
# 34 runs = 24 fit + 10 holdout; Fisher-Yates on immutable plannedRunId order.

run_ids = ['p011-v3expansion-run-%03d' % i for i in range(1, 35)]
rng = random.Random(20261019)
perm = run_ids.copy()
for i in range(len(perm) - 1, 0, -1):
    j = rng.randrange(i + 1)
    perm[i], perm[j] = perm[j], perm[i]
fit = perm[:24]
holdout = perm[24:]

manifest = {
    'protocolId': 'p011-observation3-v3',
    'protocolRevision': 'preregistration-v1',
    'analysisRole': 'expansion',
    'randomization': {
        'seed': 20261019,
        'method': 'Fisher-Yates on immutable plannedRunId order, fixed implementation',
        'generatorVersion': 'python random.Random(20261019), Fisher-Yates descending',
        'plannedRunIds': run_ids,
        'permutation': perm,
        'fit': fit,
        'holdout': holdout,
        'pilotSlot': 'p011-run-1792575546108-3 (permanently excluded per protocol §6)'
    },
    'candidate': {
        'thresholdCalibrationId': 'hysteresis-calib-v1',
        'thetaStartDeg': 26, 'thetaStopDeg': 20,
        'peakADeg': 18.7, 'rateDegPerSec': 0.5
    },
    'build': {
        'gitCommit': 'c869cda741f17def77da923e43fcafece05bb892',
        'hap': 'entry-default-s106-signed.hap',
        'hapSha256': 'e9f3f928cbe6fe0ec586a781cd71ce86e6cd57728dfa7d580142f0e316deb6a9'
    },
    'schedule': {
        'scheduleSha256': '7b59b2e4c4f4d87e9390cfc85cc7fb7c4c94a9233d70389cc8fadcba37b6bd46',
        'csvPath': 'calibration/schedule-obs3v3-A18.7-rate0.5.csv'
    },
    'checkerVersions': {
        'v3': 'p011_obs3_v3_protocol_check.py (post pre-expansion-ruling b2e00a8)',
        'rampAttribution': 'pre-expansion-ruling-P011-obs3-v1 裁决1',
        'postScheduleTruncation': 'pre-expansion-ruling-P011-obs3-v1 裁决2',
        'checkerTrapNote': 'expansion runner MUST pass --manifest (check-manifest.json) else protocolMetadataErrors false-INVALID'
    }
}

os.makedirs('calibration', exist_ok=True)
path = 'calibration/randomization-manifest-obs3v3-expansion-v1.json'
with open(path, 'w', encoding='utf-8') as f:
    json.dump(manifest, f, indent=1, ensure_ascii=False)
digest = hashlib.sha256(open(path, 'rb').read()).hexdigest()
print('randomization manifest written:', path)
print('manifestSha256:', digest)
print('fit count:', len(fit), 'holdout count:', len(holdout))
print('fit (first 6):', fit[:6])
print('holdout (first 4):', holdout[:4])
