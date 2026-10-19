# P-011 bottom-creep-v2.7 objective test report

Date: 2026-10-16
Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
Route: `bottom-creep-v2.7`
Formal device-log result: **BLOCKED(missing_hook)**
Checker self-test result: **PASS**

## Formal device log

No v2.7 device log was supplied, so formal acceptance could not be executed. Empty-input evidence records the missing hooks:

* `P011_EVENT input`
* `P011_METRIC`
* `P011_BOTTOM`
* `WS_EVENT init.capacity`

No acceptance metric is relaxed and no FAIL is converted to PASS.

## Checker self-test

| Check | Result |
|---|---:|
| Direction accuracy (`P011_METRIC`) | 1.0 (200+ samples) |
| `P011_BOTTOM` donor/receiver direction | 207/207 PASS |
| Direction sign switches | 1 |
| Bottom budget violations | 0 |
| Total motion budget violations | 0 |
| High-pressure total-clip runs | 3 |
| Full-load independent-limit degradation | 0/3 |
| P95 T90 | 0 ms (<=100 ms) |
| TIERED minimum | 8/10 (>=6) |

## Evidence

* Checker: `test/p011_check.py`
* Self-test JSON: `test/artifacts/p011_selfcheck_v2.json`
* Self-test Markdown: `test/artifacts/p011_selfcheck_v2.md`
* Formal blocked JSON: `test/artifacts/p011_blocked_v2.json`
* Formal blocked Markdown: `test/artifacts/p011_blocked_v2.md`

## Gap

Capture a fixed-seed v2.7 run with `rendermode=4 && testmode=true`, then rerun `test/p011_check.py` with that log. It must include at least 200 non-dead-zone direction samples, sign switching, and at least 3 high-pressure total-clip cases.

## 2026-10-16 device rerun

- Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Device: `62T0225B10010020`
- HAP: `entry/build/default/outputs/default/entry-default-signed.hap`
- Start parameters: `rendermode=4`, `testmode=true`, `wssnapshot=true`, test-only `wscols=10`; an additional `forceinvert=true` attempt was made for drive coverage
- Device-log result: **BLOCKED(missing_hook)**
- Evidence:
  - Raw UTF-16 hilog: `test/artifacts/p011-forceinvert-raw.txt`
  - UTF-8 hilog: `test/artifacts/p011-forceinvert-utf8.txt`
  - Checker JSON: `test/reports/p011_check_20261016.json`
  - Checker Markdown: `test/reports/p011_check_20261016.md`

### Measured completeness

| Item | Measured | Judgment |
|---|---:|---|
| `P011_METRIC` | 184 | Present |
| `P011_BOTTOM` | 184 | Present, full JSON parsed |
| `P011_EVENT input` | 0 | Missing; formal run blocked |
| `WS_EVENT init.capacity` | 0 | Missing; formal run blocked |
| Parse errors | 0 | PASS |
| Timestamp order errors | 0 | PASS |
| Route mismatch | false | PASS |

### Objective snapshot

| Check | Measured | Judgment |
|---|---:|---|
| `maxSubstepBh` | 0.00009224537174290504 | PASS |
| `maxSubstepBl` | 0.000045492866876019106 | PASS |
| `maxUncommandedTiltStep` | 0 | PASS |
| Water drift | 0.0000000000000003248195363474743 | PASS |
| Heavy drift | 0.0000000000000009473903143468004 | PASS |
| Light drift | 0.0000000000000005329070518200755 | PASS |
| Non-finite count | 0 | PASS |
| Out-of-range count | 0 | PASS |
| Model errors | 0 | PASS |
| Crash count | 0 | PASS |
| Direction accuracy | N/A | Missing input events |
| Direction sign switches | 0 | Missing input events |
| High-pressure evidence | 0 | Missing input events |

### Failure analysis

The device build did not crash and preserved the hard invariants, but the test drive did not produce the required scripted input edge. `uitest uiInput swipe/drag` did not create `P011_EVENT input`, so `P011_METRIC/P011_BOTTOM` stayed near the no-command state and the direction, sign-switch, budget-isolation and high-pressure acceptance cases cannot be accepted. No standard was relaxed and no FAIL was converted to PASS.

## 2026-10-16 wstilt rerun after reviewer clearance

- Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Device: `62T0225B10010020`
- HAP: `entry/build/default/outputs/default/entry-default-signed.hap`
- HAP SHA256: `9D9574F3EEB8E4AD75079758348ED3F19A8DB7258B7DF42F893ED953837D56D6`
- Start parameters: `rendermode=4`, `testmode=true`, `wssnapshot=true`, test-only `wspreset=thick`, `wscols=10`, `wscapacity=1000`
- Sequence: `0掳` for 3 s; then `+88掳`, `0掳`, `-88掳`, `0掳`, `+88掳`, `0掳`, `-88掳`, `0掳`, `+88掳`, `0掳`, `-88掳`, `0掳`, each for 700 ms; final rest 2 s
- Device-log result: **FAIL**
- Evidence:
  - Raw UTF-8 hilog: `test/artifacts/p011-formal-wstilt-runs-cap1000-raw.txt`
  - Checker JSON: `test/reports/p011_check_20261016.json`
  - Checker Markdown: `test/reports/p011_check_20261016.md`

### Completeness

| Item | Measured | Judgment |
|---|---:|---|
| `P011_EVENT input` | 13 | Present |
| `P011_METRIC` | 1690 | Present |
| `P011_BOTTOM` | 1690 | Present, aligned |
| Parse errors | 0 | PASS |
| Timestamp order errors | 0 | PASS |
| Route mismatch | false | PASS |

### Formal acceptance snapshot

| Check | Measured | Threshold | Judgment |
|---|---:|---:|---|
| Direction accuracy | 1.0 | all non-dead-zone samples correct | PASS |
| Non-dead-zone direction samples | 616 | >= 200 | PASS |
| `directionError` | 29 | 0 | FAIL |
| Bottom direction error | 427/616 | 0 | FAIL |
| Bottom direction sign switches | 0 | > 0 | FAIL |
| Heavy-bottom Pearson | 0.48168260498997467 | >= 0.70 for score 6 | FAIL |
| Light-bottom Pearson | 0.0810382044076828 | >= 0.70 for score 6 | FAIL |
| T90 P95 | 0 ms | <= 100 ms | PASS |
| `maxUncommandedTiltStep` | 11.52掳 | <= 2掳 | FAIL |
| Zero residual | 0掳 | <= 1掳 | PASS |
| Reverse direction error | 0 | 0 | PASS |
| Bottom budget violations | 0 | 0 | PASS |
| Total motion budget violations | 0 | 0 | PASS |
| Full-load independent-limit degradation | 0/699 | 0 | PASS |
| High-pressure total-clip runs | 11 | >= 3 | PASS |
| Water drift | 1.495879443705474e-15 | < 0.001 | PASS |
| Heavy drift | 3.1579677144893343e-16 | < 0.001 | PASS |
| Light drift | 6.315935428978669e-16 | < 0.001 | PASS |
| Non-finite values | 0 | 0 | PASS |
| Model errors | 0 | 0 | PASS |
| Crash/ANR | 0 | 0 | PASS |
| TIERED minimum | 0/10 | >= 6 | FAIL |

### Failure analysis

1. Negative `wstilt=-88` was accepted by the input event, but the model consumed it as `effectiveDeg=89`; the expected negative effective value did not occur. The same observation appears in the captured `WS_EVENT tilt` payload, so sign switching cannot pass.
2. High-pressure total-clip evidence was restored without capacity overflow: 11 contiguous clip runs, no model error, and full-load independent-limit degradation remained 0.
3. The remaining `maxUncommandedTiltStep=11.52掳` and 427 bottom-direction errors are model-level acceptance failures. They are recorded as-is and no threshold was relaxed.

## 2026-10-16 signed-HAP rerun after negative-angle mapping fix

- Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7`
- Device: `62T0225B10010020`
- Start parameters: `rendermode=4`, `testmode=true`, `wspreset=thick`, `wssnapshot=true`, test-only `wscols=10`, `wscapacity=1000`
- Requested sequence: `0`, `+88`, `-88`, `0`, each with full Want parameters; final rest 5 s

### Signing failure reproduction

1. First build attempt failed before signature validation with `00308018 Unknown Error / spawn java ENOENT` because `java.exe` was absent from `PATH`.
2. With DevEco JBR (`C:\Program Files\Huawei\DevEco Studio\jbr`) in `JAVA_HOME` and `PATH`, `SignHap` reproduced the real configuration error:
   - Error code: `00303116 Configuration Error`
   - Message: `The length of the storePassword or keyPassword field in the signature configuration is less than 32`
   - Location: `D:\hmprojects\HelloWorld\build-profile.json5`
   - Full log: `test/artifacts/p011-sign-repro-java.log`

### Signed HAP generation without modifying signing material

- The existing unsigned HAP was signed with the existing key/certificate/Profile using `hap-sign-tool.jar`; `build-profile.json5` and all signing material were not modified.
- Certificate used: `p001-sign-cert.cer`; its leaf signature matches the Profile certificate. The previous `app-cert-chain.pem` first certificate is self-signed (`Subject == Issuer`) and does not match the key.
- Signed HAP: `entry/build/default/outputs/default/entry-default-signed.hap`
- HAP SHA256: `CFE26427D1767D13B2009A3D0C1ED5004E3017A844608795B93862A39A006BD1`
- Install result: `install bundle successfully.`

Exact signing command:

```powershell
$env:JAVA_HOME='C:\Program Files\Huawei\DevEco Studio\jbr'
& $env:JAVA_HOME\bin\java.exe -jar 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\lib\hap-sign-tool.jar' sign-app -mode localSign -keyAlias 'openharmony application release' -appCertFile 'D:\hmprojects\HelloWorld\p001-sign-cert.cer' -profileFile 'D:\hmprojects\HelloWorld\entry-default.p7b' -inFile 'D:\hmprojects\HelloWorld\entry\build\default\outputs\default\entry-default-unsigned.hap' -signAlg SHA256withECDSA -keystoreFile 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\lib\OpenHarmony.p12' -keystorePwd 123456 -keyPwd 123456 -outFile 'D:\hmprojects\HelloWorld\entry\build\default\outputs\default\entry-default-signed.hap' -compatibleVersion 12 -signCode 1
```

### Device capture

- Raw UTF-8 hilog: `test/artifacts/p011-formal-signfix-raw.txt`
- Checker JSON: `test/reports/p011_check_signfix_20261016.json`
- Checker Markdown: `test/reports/p011_check_signfix_20261016.md`
- Measured hooks: `P011_EVENT input=3`, `P011_METRIC=563`, `P011_BOTTOM=557`
- Parse errors: 0; timestamp order errors: 0; route mismatch: false
- Sequence note: the final duplicate `0` command returned `start ability successfully` but did not emit an input event, so the device produced 3/4 requested edges. This is recorded without altering the requested sequence or checker standards.

### Objective results

Checker overall: **BLOCKED(missing_hook)** (`P011_METRIC/P011_BOTTOM.physicsTime alignment`).

| Check | Measured | Threshold | Judgment | Score |
|---|---:|---:|---|---:|
| Direction accuracy | 1.0 | all non-dead-zone samples correct | PASS | 10 |
| `directionError` | 0 | 0 | PASS | |
| Bottom direction error | 41/43 | 0 | FAIL | |
| Bottom direction sign switches | 1 | > 0 | PASS | |
| Heavy-bottom Pearson | 0.7681016501 | >= 0.70 | PASS | |
| Light-bottom Pearson | 0.6615982813 | >= 0.70 | FAIL | |
| T90 P95 | 0 ms | <= 100 ms | PASS | |
| `maxSubstepBh` | 0.2427546973 | < 5 | PASS | |
| `maxSubstepBl` | 3.9133703598 | < 5 | PASS | |
| `maxUncommandedTiltStep` | 11.52掳 | <= 2掳 | FAIL | |
| Zero residual | 0掳 | <= 1掳 | PASS | |
| Reverse direction error | 0 | 0 | PASS | |
| Bottom budget violations | 0 | 0 | PASS | |
| Total motion budget violations | 0 | 0 | PASS | |
| High-pressure total-clip runs | 2 | >= 3 | FAIL | |
| Water drift | 1.0471156105938318e-15 | < 0.001 | PASS | |
| Heavy drift | 3.1579677144893343e-16 | < 0.001 | PASS | |
| Light drift | 6.3159354289786686e-16 | < 0.001 | PASS | |
| Non-finite values | 0 | 0 | PASS | |
| Out-of-range values | 0 | 0 | PASS | |
| Model errors | 0 | 0 | PASS | |
| Crash/ANR | 0 | 0 | PASS | |
| TIERED minimum | 2/10 | >= 6 | FAIL | |

### Failure analysis

1. The mapping fix is visible in the captured edges: `targetTilt=+88` and `targetTilt=-88` were accepted, and positive direction accuracy was 1.0 with zero reverse-direction error.
2. Acceptance still fails because only 2 of 43 bottom-direction samples matched, `maxUncommandedTiltStep=11.52掳`, light-bottom Pearson was below the tiered pass line, and high-pressure evidence was 2 instead of 3.
3. The missing final duplicate-zero edge and metric/bottom timestamp alignment blocked full formal acceptance. No threshold was relaxed and no FAIL was converted to PASS.

## 2026-10-16 formal suite rerun

Source commit: `bb854c21a260e7ace92edc99084fbdac736d81d7` (git CLI unavailable; commit recorded from the prior report)
Device: `62T0225B10010020`
Artifacts: `test/artifacts/p011-formal-prep-20261016/`
Overall: **FAIL**

### Test-script corrections before acceptance

1. `p011_tc02_formal_suite.ps1 --Mode All` now invokes TC02; the original `All` path omitted `Invoke-Tc02`.
2. TC02 now drives the device with an LF-terminated device shell script, performs `aa force-stop`, writes a 16 MB app hilog buffer, and preserves the 100 ms minimum accepted-time gap requirement.
3. `p011_check.py` no longer treats system `ANRManager` log lines as application crashes; FATAL/Fatal/app-freeze markers remain. This is a checker false-positive correction and does not relax any functional threshold.

### TC-P011-02 continuous linear drive

- Driver: 531/531 script events, expected 531; minimum accepted-time gap 254 ms; schedule validation PASS.
- TC01: score 10/10, direction error 0, bottom direction error 0.
- TC02: score 0/10; Pearson H/L were null because the TC02 pressure profile has no full-load clip evidence.
- TC03: score 10/10, max substep BH/BL 0.0098605146/0.0248137689950, max uncommanded tilt step 0.
- MUST-PASS failures in TC02 context: `independentBudgetIsolation=false`, `highPressureEvidenceCount=false`; these are pressure-evidence requirements not fulfilled by the TC02-only log. Crash count was 0.

### Pressure-segment checker decisions

| Log | Overall | Full-load samples | Total-clip segments | Segment count >= 3 | Hard-gate failures |
|---|---|---:|---:|---|---|
| `p011-full-load.log` | FAIL | 1995 | 2 | NO | maxSubstepBelow5, bottomDirectionCorrectAll, highPressureEvidenceCount |
| `p011-clip-segment-a-positive.log` | FAIL | 2023 | 2 | NO | maxSubstepBelow5, maxUncommandedTiltStepBelow2, bottomDirectionCorrectAll, highPressureEvidenceCount |
| `p011-clip-segment-b-negative.log` | FAIL | 2033 | 3 | YES | maxSubstepBelow5 |
| `p011-clip-segment-c-positive-repeat.log` | FAIL | 2034 | 2 | NO | maxSubstepBelow5, highPressureEvidenceCount |

Across the suite, 9 high-pressure total-clip segments entered the final checker. The negative-pressure repeat log alone met the segment-count acceptance with 3 segments. All logs had 0 non-finite values, 0 out-of-range values, 0 model errors, 0 crash markers after the checker correction, 0 bottom-budget violations, and 0 total-motion-budget violations. Drift remained below 0.1%.

The largest measured full-load substep values were BH=70.0552093461 and BL=70.6772417879. Segment A also produced max uncommanded tilt step 81.988 degrees. Segment B and C produced max substep BL 41.2114885190 and 46.6662184307 respectively. These are hard-gate failures and cannot be tiered as acceptable.

### Overall routing

- Overall suite: **FAIL** because at least one hard gate fails in every pressure-segment log.
- Responsibility route: **designer**, starting with full-load and repeat-segment max-substep magnitude, then segment A's uncommanded tilt step and the two bottom-direction failures.
- No checker threshold was widened; the only checker change removed a system-log false positive.

