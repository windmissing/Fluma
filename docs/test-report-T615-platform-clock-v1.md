# T615 v1 · 目标 9 真实事件时钟平台证据取证

日期：2026-11-18，tester，真机（011124264R000337，MLN-AL00，OS 7.1.0.566
C00E23R2P1DEVDUlog）+ SDK 源链核对。
证据：`test/reports/t615/platform-evidence.json`。

## 已采事实（权威源证据）

### 1. SDK 契约（BaseEvent.timestamp 语义）
`OpenHarmony 26.0.0 SDK ets/component/common.d.ts`
（sha256=c4e673557876fecbe51603b4d76a39c78def8cf0983352199c133793a8b2c804）：
> "Timestamp of the event. It is the interval between the time when the event is
> triggered and the time when the **system starts**. **Unit: ns**"（@since 8）

即 SDK 权威声明 = **ns since system start**（单调语义域，非 wall epoch）。
另一相关：`@ohos.multimodalInput.touchEvent.d.ts` Touch.pressedTime
"microseconds since the system starts"（@since 9，μs 粒度）。

### 2. 实测一致性（一致，非证明）
- `/proc/uptime` = 204155.02s → 2.04155×10¹⁴ ns since boot
- T593 v1 `inputStages.rows[].nativeNs ≈ 1.667×10¹⁴`、T510 v3 timeline ns
  ≈1.663×10¹⁴——同一次 boot 内更早时刻，量级与 ns-since-boot 一致 ✓（实测一致，
  不构成 producer/转换链完整证明）

### 3. 部署二进制锚
- T599-delivery-final-signed.hap：02bbd0cf940d99df…；包内 libgrsm1.so：
  facae236bbf21d79…（本地签包同 SHA 域；实际进程级加载映射未做 dlopen 句柄核）

## 准确缺口（不伪完整 proof）

| 缺口 | 需要的输入 | 接手角色 |
|---|---|---|
| producer（MultimodalInput→ArkUI 事件分发）实现源 | 厂商/OS 源码树对应 MLN-AL00 7.1.0.566 build | 平台/系统角色 |
| 重采样/转换链（μs pressedTime→ns timestamp→CLOCK_MONOTONIC）单位与 epoch 换算 | 同上 + 系统时间子系统源 | 平台/系统角色 |
| 部署二进制与源码映射 | vendor build 符号/调试信息/源清单 | 平台/系统角色 |
| LoadQualifiedPlatformClockProof 输入 | 以上三项齐备后的 proof 打包 | 平台实现者 |

SDK "since boot" 一句与上游源码本身按合同**不足以伪完整 proof**；本轮保
INPUT_CLOCK_UNVERIFIED 现状不宣 REAL_TOUCH 资格。
