# T522 V4 实施记录（进行中，不是交付回执）

2026-11-16，developer，不使用真机。依据 T497/T508/T498 v2。完整任务尚未完成，状态保持“进行中”，旧 M1/M2 默认入口不切换。

## 当前可复现检查

- `python test/t522/generate_contract.py`、`build_contracts.cmd`：四档表、分配对齐和语义 mask；生成 quota256 字段也消费 T508 manifest。
- `build_control.cmd`：独立 V3 控制、B/BC 整数守恒、BLOCKED/再入、旧尾、SOURCE-only、溢出、版本 reader；有界 FIFO 的实际入队/意图原子提交、重复 key、满队背压和冻结 sourceBoundary。
- `solver_cases.py` + `build_solver.cmd`：实际离线 ANGLE ES3.1 λ/η及 waterLevel 11 场景。
- `seed_shader.py` + `build_seed.cmd`：实际 GL 共享 origin、升序 seed、连续 ID、尾包。
- `compile_source_shader.py` + `build_source.cmd`：同产品模板 SOURCE72 的普通沙源、DIRECT/FLIGHT 水源、湿向量和两 trial 回滚；SPARE 独立 trial/root 重放、ledger/raw dedup 全字验证、publication 审计和 V4 completion seal。篡改 trial 行拒绝在 promotion 之前。
- `compile_occupied_shader.py` 后运行 `t522-source.exe quantum`：105 个单 scope 核心槽（未含 scope metadata/finalize）实际 GL 运行及独立 SPARE replay 比较通过；**不能称完整五角色 QUANTUM 通过**。
- `.codex/local-test/t522-native-source72`：DevEco ArkTS/native `assembleHap --no-daemon` BUILD SUCCESSFUL，33 tasks（25 executed/8 up-to-date）。新 SOURCE 资产和 V3 native 模块由 CMake 编译。隔离工程用 unsigned HAP 构建；最终交付另走既有 T257 签名/verifyApp，当前尚未做最终签包。

ANGLE 仅用于离线开发检查，不接产品回退。以上不提供真机性能或 250ms 设备达标结论。完整 native 调度、五角色接受、scene/lease、冷协议与正常页面接线仍须完成后再做最终编译/签包和回执。

## 已发现的参考 oracle 差异

T499 `solve_lambda` 没有覆盖 T497 §4.5 的保守触顶拒绝。N=128、T=0、W=[5,5,0…]、cap=[5,CQ…]、r=[0,4,0…]、λ=65536，oracle 返回 OK/eHi=1，但 eta0[0]+eHi>cap[0]，正式合同要求 ETA_TRUNCATION_RETRY。新增 `truncation_contract` 负控按正式合同判断；未修改 tester oracle。已在原卡评论同步精确复现，后续由 architect/tester 处理参考资产差异。

## 剩余实施范围

1. REQUESTED/CLAMPED/OLD/AUTHORITATIVE/CONSISTENCY 完整 GPU 调度、control128、权威事件锁存和三态/全流比较。
2. 独立 native root owner/transport、四档固定表、250ms 原期限/双 GAP、同锁退休和 native 接受、完整 SOURCE/QUANTUM 串接。
3. scene/lease 与旧 reader 拒版本；V4 producer/controller 页面与 XComponent/shader 接线。
4. T507 冷协议新 schema/payload/容量适配、BQ01–BQ21 全夹具和完整根实际 GL 自测；最终正常应用编译、签名核验、独立提交与原卡完成回执。
