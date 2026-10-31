# T337 fence 修复真机验证（K1-K5）——BLOCKED（T336 实现遗漏 ArkTS 校验同步，导出路径 100% 被拒）

2026-10-31，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：T336 实施 commit `8e2662a`。

## 0. 包身份（T151 三件套，已完成）

- HEAD `d547e6c`（工作区 cpp/ets 零未提交改动）hvigor 重建（BuildNativeWithNinja 2.9s + PackageHap 刷新，SignHap 0031016 已知独立登记）→ Python 重打包 → hap-sign-tool localSign + verify-app **PASS**。
- **T337-signed.hap SHA256 `1b5656e5c109f4928c5ffa6153c2ab9b1f96a4f13c1c092d63658eead9336392`**，页面 TextInput 外部输入核对一致，导出 manifest `identity.packageSha256` 回读同值。

## 1. 阻塞根因（卡面红线触发：停下反馈，不强行判 PASS）

**K1-K5 全部无法出证**：两次冷启导出（run1/run2）均 `INCOMPLETE Error: INVALID_PAGE`（manifest `m1.json`/`m2.json` 归档，rows=0、stageRevision 回退 T295_STAGE23_V1、files 空）。

**根因（源码级定位）**：T336 按 Amendment 2 把 native timeline 环容量 1024→4096（`bridge_timeline.h:27` `static constexpr uint32_t capacity=4096`，本包 libgrsm1.so 已含），但 **ArkTS 侧分页校验器未同步**——`entry/src/main/ets/physics/GpuBridgeTimelineCold.ets:24` 硬编码：

```
p.capacity!==1024 || ... || p.count>1024  →  throw new Error('INVALID_PAGE')
```

native 产出的每页 `capacity=4096` 全部被 ArkTS 校验拒收 → 导出流程 0 行落盘 → K1（fence 停顿）、K2（submitPumpWallNs）、K3（accepted/双源链）、K4（六项无回归的 41 阶段遥测出证）、K5（三 run COMPLETE）无一可判。

**定性**：T336 实施遗漏（Amendment 2 §2.3 环扩容的配套校验同步未做；T336 回执五套非机回归未覆盖 ArkTS 导出校验路径——非机替身不经过 `GpuBridgeTimelineCold.ets`）。fail-closed 语义正确（坏页不落盘），但本卡验证目标结构性不可达。

## 2. 已完成与已采集证据

- 签包三件套归档 `.codex/local-test/t318-sign/T337-*`。
- 两次 INCOMPLETE manifest（`test/reports/t337/m1.json`、`m2.json`，含 statusBefore：PAUSED_CLOCK/timelineCount=121/dropped=0——native 环 4096 工作正常，卡点纯在 ArkTS 校验）。
- 布局快照 `layout0/1/2.json`（SHA 输入、导出按钮坐标、INCOMPLETE 状态）。
- hilog GRS_TIMELINE_COLD 两条 INCOMPLETE 记录（8390 pid，sha256 逐位一致）。

## 3. 修复建议（交 manager/developer，一行级）

`GpuBridgeTimelineCold.ets:24` 校验同步：`p.capacity!==1024` → `p.capacity!==4096`（或放宽为 `p.capacity<1024||p.capacity>4096`）、`p.count>1024` → `p.count>4096`。建议随修复卡同步补一条 ArkTS 校验器单测（V1 1024 页与 V2 4096 页各一），防再次漂移。

## 4. 结论

- **T337 BLOCKED**：K1-K5 无法判定（导出路径被 T336 遗漏的 ArkTS 校验堵死），不强行判 PASS 也不判 FAIL——判定推迟到修复卡交付后重测。
- 包身份三件套与阻塞证据链完整落盘；设备无异常（无崩溃，fail-closed 正确）。

证据：`test/reports/t337/`（m1/m2.json、layout0-2.json）+ `.codex/local-test/t318-sign/T337-signed.hap`。
