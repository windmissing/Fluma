# T847 v1 · PC 指针 SOURCE 与首个 QUANTUM 端到端验证——M1 窗口模式稳定崩溃（如实）

日期：2026-11-23，effector，Windows PC（无手机）。
被测：held-release PE **4ab9ff59**（卡面指定，SHA 核验）+ 三份 SPIR-V
（1912af9e/b8f0e327/caf0614c，回执匹配）。证据：
`test/reports/t847/`（run1/run2/run3.jsonl 各 22 条、diag.jsonl、v1 判定）。

## 诊断模式（无 --m1-window）：PASS

PE 本体健康——BOOT 16 阶段完整、窗口存活、像素 0xff00ff00。

## M1 窗口模式：稳定崩溃（3/3 复现）

三次运行（相对路径/绝对路径/重复）全部在**同一崩溃点**：

```
BOOT 15 阶段 → M1_SOURCE_MODULE/PIPELINE
→ M1_QUANTUM_CANDIDATE_PIPELINE → M1_QUANTUM_COMMIT_PIPELINE
→ M1_SOURCE_BUFFERS → **M1_QUANTUM_COLD_ANGLES（RUNNING）→ SEGFAULT**
```

22 条 JSONL 原字每次完整保留到崩溃点。

## 崩溃点定位

`m1_quantum.inc:91` Checkpoint("M1_QUANTUM_COLD_ANGLES")——在描述符集
更新后、角度表上传序列（vkResetCommandPool→vkCreateFence→
CmdUpdateBuffer 循环）附近。诊断模式通过证明 Vulkan/窗口/spv 加载全正
常——崩溃特定于 M1 QUANTUM 冷角度路径。

## 端到端范围（按卡面）——NOT REACHED（如实）

Down、同代 SOURCE→QUANTUM→Present、实际 shader 像素、Up 与退役——
**崩溃在 M1_POINTER_READY 之前**，端到端全部未达。M1PCOLD1 原字未产生。

## 回执

- PE 4ab9ff59 + 三 spv 身份核验 ✔
- 诊断模式 PASS（设施正常）
- **M1 窗口模式稳定 SEGFAULT（3/3，崩溃点 M1_QUANTUM_COLD_ANGLES）**
- 归 developer：修 m1_quantum.inc:91 附近崩溃（角度表上传序列）
