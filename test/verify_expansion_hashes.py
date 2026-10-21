import hashlib, re

s = open('tools/run_p011_v3expansion.ps1', encoding='utf-8').read()


def h(p):
    return hashlib.sha256(open(p, 'rb').read()).hexdigest()


real_hap = h('D:/hmprojects/hw-v3pilot-c869cda/entry/build/default/outputs/default/entry-default-s106-signed.hap')
real_sched = h('calibration/schedule-obs3v3-A18.7-rate0.5.csv')
real_rand = h('calibration/randomization-manifest-obs3v3-expansion-v1.json')

m = re.search(r"ExpectedHapSha256 = '([0-9a-f]+)'", s)
print('HAP match:', m.group(1) == real_hap)
if m.group(1) != real_hap:
    print('  runner:', m.group(1))
    print('  real  :', real_hap)

m2 = re.search(r"ScheduleSha256 = '([0-9a-f]+)'", s)
print('SCHED match:', m2.group(1) == real_sched)
if m2.group(1) != real_sched:
    print('  runner:', m2.group(1))
    print('  real  :', real_sched)

# randomization manifest 内的 hapSha256 也要校验
import json
rm = json.load(open('calibration/randomization-manifest-obs3v3-expansion-v1.json', encoding='utf-8'))
print('RAND manifest hapSha match:', rm['build']['hapSha256'] == real_hap)
if rm['build']['hapSha256'] != real_hap:
    print('  manifest:', rm['build']['hapSha256'])
    print('  real    :', real_hap)
