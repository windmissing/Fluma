# T300 GPU M1 场景独立验证（离线，结束待验收）

2026-10-30，tester 离线（PC SwiftShader，非真机——GL 身份如实标注 `PC_SWIFTSHADER`）。被测：T281 交付 `GRS_GPU_SCENE_Q20_M1_V2`（c127322），组合 SHA `6c476dcf…`、scene SHA `64a37ed0…`。

## 一、独立自持像素 oracle（tester 全新实现，不调开发 check_t281_pixels）

从 `scene.exe` 独立重跑再生 140 张 RGBA 原图（四档 K192/512 × Kw64/128），解析原图逐项重算：

| 判据 | 实现 | 结果 |
|---|---|---|
| 深水水体 sRGB 混合 | 独立推导 path=31.5（水面 y=64、像素中心 32.5）、alpha=1-exp(-0.03×31.5)，linear premult→sRGB 全程自算 | 4/4 档 got=exp=(46,81,95,255)，≤2 LSB PASS |
| 干列不受邻列雾化 | deep(x=127)==empty(x=127) 逐字节 | 4/4 PASS |
| 空场景水体/水面/退包层一致 | empty 层 2==3==4==6 | 4/4 PASS |
| ARRIVE 退包不双显 | arrival 层4 与层6 在 J 区域 (132..136,55..75) 逐字节一致 | 4/4 PASS |
| 飞行包高 J 可见 | flight 层4≠层6 | 4/4 PASS |
| 活动沙/床被绘制 | sand 层2 与层6 均异于 empty 对应层 | 4/4 PASS |

**6 判据 × 4 档 = 24/24 独立 PASS**。oracle 记录：`test/reports/t300/independent_oracle.json`。

## 二、196/0 独立复跑（非复用 json）

`scene.exe` 四档直接执行（ANGLE via CokePLUS libEGL/libGLESv2；GL 软件身份 PC SwiftShader 如实）：

- K192/W64 49/0、K192/W128 49/0、K512/W64 49/0、K512/W128 49/0，合计 **196/0**，execution=PC_SWIFTSHADER、device=NOT_RUN。
- 再生图 35 SHA × 4 档与开发 `pixels-*.json` 记录**逐字节一致（0 mismatch/140）**——开发报告与可复现产物同源可信。
- 场景专项 fixture kappa=0（隔离 ARRIVE/退包像素）与生产 kappa=24 分开，未混淆；未冒默认物理/设备收益。

## 三、范围与边界（如实）

- token 完整性/组合 SHA/lease/fence/旧代拒绝/失败 swap 不提交 LastGood：由 scene.exe 内 49 项断言覆盖（含错 context/旧 publication/失败 swap 后 LastGood 不变），本轮独立复跑通过；**tester 未对这些边语义逐项自建 oracle**（开发 196/0 复跑不冒独立，此部分记为"独立复跑一致、语义 oracle 部分依赖开发断言"）。
- 真机设备收益 NOT_EVALUATED（另卡 T301，本卡不作前置）。

## 四、结论

T300 **结束待验收**：独立像素 oracle 24/24 PASS + 四档 196/0 独立复跑 + SHA 同源一致。无 FAIL、无需回 architect。

证据：`test/reports/t300/`（独立 oracle json + 四档复跑输出 rc_*.txt）。
