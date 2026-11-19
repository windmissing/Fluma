# T630 v1 · 实际 NAPI provider 长度语义独立取证

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T630-probe3/4-signed.hap——T582-g9-final 测试树 + tester probe 页
（T630Probe.ets，`--ps t630probe true` 直达路由），**真实生产绑定**
native.m3Configure / native.g9ResearchConfig64（libgrsm1.so）。
证据：`test/reports/t630/typed-length-unit.json` +
`test/reports/t491/t630c/`（probe 页回复快照）。

## 方法

tester 设施 probe 页向真实 m3Configure/g9ResearchConfig64 绑定发送不同元素类型
（Uint8/Uint16/Uint32/Float32）与不同长度（1/16/32/64/64B），读取 GPU_NAPI_ERROR_V1
回复的 firstFailedStage，区分 ELEMENTS vs BYTES 单位语义。

## 实测结果

| 输入 | 元素 | 字节 | stage | 判读 |
|---|---|---|---|---|
| Uint8Array(64) | 64 | 64 | ELEMENT_TYPE | kind≠uint32 先拒（长度检查未达） |
| Uint16Array(32) | 32 | 64 | ELEMENT_TYPE | 同上 |
| **Uint32Array(16)** | 16 | 64 | **ELEMENT_COUNT** | **决定性**：若 ELEMENTS 语义 raw=16==N 应通过；被拒只有 BYTES（raw=64≠16）可解释 |
| Uint32Array(64) | 64 | 256 | ELEMENT_COUNT | 两单位下都拒（弱信号，与 u32(16) 构成自洽对） |

## 结论

**当前打包 provider 的 napi_get_typedarray_info length 返回字节数（BYTES）**：
- 决定性单点：Uint32Array(16) 在 ELEMENT_COUNT 被拒——ELEMENTS 语义下 raw=16==N
  必过，唯 BYTES（raw=64≠16）自洽
- 交叉验证：T590 v1 原件记录 native elementLength=64（ArkTS Uint32Array(16)）
  ——与 BYTES 精确吻合
- T621 合同的"华为官方接口区分标准/native 接口元素数与导出接口字节长度"假设
  **实测成立**

## 对 T629 的直接输入

产品 reader 适配 **profile lengthUnit=BYTES + checked /4 规范化**（整除
sizeof(uint32) 校验后 normalizedCount=raw/4），按 architecture-T621-v1.md §产
品工程修订执行；T590 续测在适配+资格两项齐后进行。

## 未测域（如实）

- Float32Array opticsConfig 路径未实测（历史 M1 轮从未发过 optics 调用——此前
  "M1 optics 正常"为错误假设，已纠正：optics len==16 判据从未被真机验证）
- Sendable 跨域 typed 未 probe（无构造入口）
- elementKind 数值仅在内层 snapshot（probe 外层 reply 不含）——如需 kind 原值
  需扩展 probe 解析 gpuInputDiagnosticsV1
