# T135 支持纯核实施 V2

2026-10-27，developer，原卡 6adf48d1e4b03273ad86acdb。不使用真机。
接续 4c4d59b 草稿，按 T127/T148 实现隔离的阶段二、三；未接生产 strict route。

- 新增 `P011WriterRNGraph`，128 个冷定容节点。每个节点记录真实 RN 值、输入槽、ideal 区间、传播 error 区间及来自当前运算范围的 η；冻结的 depth/dH/dL/M 用新常量节点明确区分。系数与截距、CanonicalS 三次 RN 和 δ0 均向外包络，settle A 与 top b 分列。重复乘法复用同一节点，未引入生产 BigInt。
- settle、TOP/MIX 与 CONTACT_WAIT partial LAND 在认证域内解上下 affine 约束；系数正/负/零分支使用保守向内端点，再用短表达式精确比较核对上下端点，选择该保守域上端。所有约束沿同一入口 residual/H 或旧 residual bound；最多 RAW、INWARD 两次非零构图。TOP 后两分支和 full LAND 只有 RAW/ZERO。包络与实际不一致直接返回 `ENVELOPE_CERTIFICATE_MISMATCH`；库存/质量审计错误保留 `AUDIT_FAILED`。
- `P011KernelFrozen` 是隔离测试 owner：列材料、水、source/landed、列在途 cache、local/total、已 debit ticket、pool slot 的质量/ID/state/phase/source/运动字段、free-list/nextId、事件与 lineage。显式 before/after 供测试读取；private seal 不接受公共输出篡改。先冻结 changed 列及邻居、池槽/票据身份，preview 不写 owner；commit 验证所有参与 before/版本/容量后一次写入。token 必须显式传入，下一 preview 与重复消费失效。
- full LAND 仅释放原槽一次，partial 保留原 ID/source/已有等待位置速度（T148 支持域），列 cache/床/事件/lineage 同笔提交。退款只以原 debit−qControl 计算，各 local 端退实扣、total 一次；ZERO 不执行材料、水、cache、pool 或成功 LAND 写入，只结算原有 reservation 的拒绝退款。AIRBORNE/CONTACT_HOLD 和未登记 writer 仍 UNSUPPORTED。

验证：`node test/run_t135_writer_kernels_units.cjs` 14/14 PASS，包括 T134 的 A≠1/.175 夹具、MIX 偏置/后两分支、三种 INWARD、partial ULP、full retire、来源/预算/水、旧 token/重复 commit/公共输出篡改、提交前各参与方改变、无写 ZERO。测试端独立 BigInt 有理数逐节点重构 settle/source、top、LAND 的精确理想表达式，核对 ideal/error 范围；此开发自测不替代 tester 独立验证。T133 基础单测 PASS。

hvigor CompileArkTS、Native、PackageHap 通过，完整 assembleHap 仍在既有 SignHap 密码少于 32 位的配置处失败；日志 `test/reports/t135_build_v2.txt`。未更改除法负号、H、epsilon、预算/守恒判据；未声明设备 code5 或 step73 已修复。

原 `developer-T135-partial-kernels.md` 保留为上一阶段历史，现支持边界以本回执为准。
