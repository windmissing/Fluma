# T671 离线设施与交接报告

2026-11-19；tester；不使用真机。构建/离线设施交付，不表示设备、物理或性能 PASS。开发编译基线 8fe7c2e65565350b2013dd466bb64dce189b2351；本卡新增工作区按随附源/对象 SHA 收据绑定，提交时一并入库。

完成 T659 正确 owner 产物复用与独立签包，未使用旧 owner 包。unsigned SHA 必须匹配 f3a0beb3377b4a00df61170b7e748389fe6d1219b5f54d07703b47c6bfa9b42e；T671 verifyApp PASS，签包路径/SHA/ABC/ELF BuildId、599项源及对象闭包见 test/reports/t671/handoff.json，签名收据 test/reports/t257/T671.json。

离线检查：实际 PrepareReadinessV4 模板十字段比较、不可变快照、等值新DTO、safe integer/native非零、READY无TTL、票据六项篡改/重复消费/就绪下降均拒绝；check_owner.py 13项正反控。复用未修改 T413 严格采集工具：strict PNG/HOLD/clock/原RGBA+SHA/旧PNG拒绝等16项通过；独立进程runner四场景通过；T415既有oracle自测通过。原始输出见本卡报告目录。没有放宽 ROI、时间包络、HOLD或PNG门。

可复跑：node test/t671/owner_units.cjs；python test/t671/check_owner.py --selftest；python test/t413/units.py；python test/t413/runner_units.py。python test/t671/verify_capture.py prepare-cold.json reserve-witness.json capture/spec.json 离线联核 owner/ticket 与完整HOLD/PNG链；单独 owner PASS 不代表截图/物理 PASS。

交接T592 effector：使用本卡新签名包；真实固定系统键盘→prepare→RESERVED→HOLD，按 test/t413/capture_hold.py 的设备配置运行。taps和deviceId必须由实际设备布局给出，不预填另一设备旧坐标。捕获原prepare冷DTO及ReserveAttemptWitnessV1 first/last 64word原件（core14 ownerSerial/core15 retired），private ticket不导出不伪造、HOLD前后status、原PNG及clock样本；只用lossless RGB/RGBA PNG，不能拿旧图/屏幕预览代原件。离线用上述verify_capture和原screen.py阈值判定。构建复用工具 tools/singlesand/prepare_owner_build.py 保固定T598基线及T659 overlay；无需重构现有正确unsigned。真机 NOT_RUN，不作本卡关闭门。
