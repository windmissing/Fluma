# P011 dual-input-v1 developer delivery — 2026-10-18

Scope: architecture-P011 §15; physical route remains bottom-creep-v2.8-heavy-interface. No physical mechanism, material initialization, budget or acceptance threshold changes.

## Changed files and reasons

- `entry/src/main/ets/physics/P011InputMode.ets`: pure fixed-size mode/input state machine used by the actual page; source exclusion, sequence watermark, held script and fresh sensor gate.
- `entry/src/main/ets/physics/LiquidCAWant.ets`: optional current tilt Want sequence on the existing mode envelope.
- `entry/src/main/ets/entryability/EntryAbility.ets`: parse current Want before publication; mode envelope carries its valid tilt sequence; publish tilt last.
- `entry/src/main/ets/pages/SandCanvas.ets`: shared mode transition for startup/Want/panel/long press; mode-first frame order; callback/consumer/Pan gates; input-only cleanup; two explicit UI names; boundary and first-real-callback diagnostics.
- `entry/src/main/ets/physics/WaterSandField.ets`: only added O(1) `resetInputState`; no step/material traversal/reset/run change.
- `entry/src/test/P011InputModeTest.ets`, `entry/src/test/List.test.ets`, `entry/src/test/WaterSandFieldTest.ets`: seven mode behavior tests, registration, material/run-preserving input reset test.
- `test/run_p011_input_units.cjs`: execute actual product .ets and the same Hypium tests using bundled TypeScript and a small assertion adapter, without a copied input/physical model.

Historical dirty changes in shared product files are preserved. `source-manifest.json` hashes the complete main source/resource tree and build metadata plus relevant tests, and links the signed HAP. Reviewer must retain existing P001 dependencies when committing.

## Verification

1. Tests preceded implementation: real helper imports initially failed `MODULE_NOT_FOUND` for the not-yet-created P011InputMode. After implementation, `units.json` records 39 PASS / 0 FAIL: seven mode cases, one reset preservation case, 31 existing WaterSand regression cases. Coverage includes 100 sensor callback interferences per scripted command, ten round trips, cold/hot first envelope, same-mode commands, invalid pending watermark, fresh sensor recovery, and exact equivalent-input physical trajectories.
2. `hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`: BUILD SUCCESSFUL; ArkTS compiled and HAP packaged (`build-package.log`). `default@UnitTestArkTS`: BUILD SUCCESSFUL (`unit-arkts-build.log`). Full `assembleHap` reaches existing SignHap configuration failure 00303116 because repository signing passwords are under 32 characters; no signing config or material was changed. The existing documented `hap-sign-tool.jar` local-sign procedure produced the new signed HAP successfully (`sign.log`).
3. New signed HAP `entry-default-dual-input-v1-signed.hap` SHA256: `C89FC37C309412B5C010F52CC0F5C682CAC711C1F13D1F1127380FBC233B27AB`. Installed this exact new package on real device `62T0225B10010020` with `install -r`; `install.log` records `install bundle successfully`.
4. Runtime `runtime-hilog.txt`: cold EXPERIENCE starts PID 16010 and receives an actual ACCELEROMETER callback with finite x/y and system callbackTimestamp. Hot EXPERIENCE→TEST Want with first +15 tilt retains sequence 1, consumes it once, then -15/0 are sequences 2/3 within the same run. EXPERIENCE request with forceinvert=true/wstilt=70 rejects both controls, clears input caches and receives its next true sensor callback. Cold TEST starts PID 16300 and retains/consumes first +15 sequence 1. Both processes were checked alive and page content loaded. This is developer runtime smoke evidence, not TC02/formal quality acceptance.
5. UI layout/screenshot artifacts: `experience-layout.json`, `experience-panel.jpeg`, `test-layout.json`, `test-panel.jpeg` show the exact current-mode text. `panel-hilog.txt`, `panel-to-experience-layout.json`, `panel-to-test-layout.json` record actual panel clicks TEST→EXPERIENCE→TEST; first real callback follows the experience boundary in 86 ms; no old script replay. Final PID 16300 remained alive. No manual handset motion was requested or used.

Device lease RELEASED after runtime smoke; hilog capture processes stopped. Actual callback evidence proves static-device sensor wiring, not dynamic sensor calibration/noise/handheld experience acceptance. Canonical 531-row audit, complete unchanged physical suite, green-ROI and subjective quality remain tester/effector work after review.
