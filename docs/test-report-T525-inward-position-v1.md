# T525 · 单沙向内位置实际页面独立差分 tester 报告 v1

- 卡：`6afa6b4be4b010f4894f7773`（tester，P4，不使用真机）
- 日期：2026-11-16
- 依据：architecture-T509-inward-raster-position-v2.md；被测=T516（a38636c/cf15da7，HAP `6f49d06d…`）
  实际生成 RasterGeometry/PrepareReadiness/页面
- oracle：test/t525/inward_position_oracle.py（独立推期望，不信任 developer 44 断言）
- 产物：test/reports/t525/t525-inward-position-oracle.json

## 结果 25 PASS / 1 NOT_RUN / 0 FAIL

### A. 精确样本独立复算（核心）

window 物理 px 域 `ceil(x/y)、floor(x+w/y+h)` 独立复算父框 (51.20001220703125, 990, 1177.5999755859375, 1716)：

```
x=ceil(51.2)=52, y=990, w=floor(1228.8)−52=1176, h=floor(2706)−990=1716
→ (52,990,1176,1716) 与卡面精确样本逐字 MATCH
```

- 旧 51/1177 拒（51<ceil(51.2) 且 1177≠floor 派生宽）；52/1177 拒（1177≠1176）
- T516 pages.json 44/0 独立读

### B. 相邻浮点/半像素（ceil 语义逐 float，2.625/3.5 两 scale）

| 值 | ×3.5 → ceil | ×2.625 → ceil |
|---|---|---|
| 9.999999999999998（−ε） | 35（同格） | 27（同格） |
| 10.0（恰好） | 35 | 27 |
| 10.000000000000002（+ε） | **36（进位）** | 27 |
| 10.5（半像素） | 37 | 28 |

关键语义差异实证：scale3.5 下 +ε 进位（35→36）而 −ε 同格——**归属检查必须逐 float 判**，
2.625 下 ±ε 恰好同格（乘积不越界）。T516 8 项相邻用例覆盖两 scale × 4 值。

### C. floor 边界（12 项）

0.999..8→0、1→1、+ε→1（不动）；1177.5/±ε→1177、1178→1178；0/−1/NaN/Inf/2^31 由页面严格门拒。

### D/E/F. 平台绑定与换代语义

transform 不支持、窗口原点不匹配、触摸层==实际节点、EGL −1、旧 packet 宽、surfaceEpoch/
contextEpoch 换代旧 packet 拒、键盘严格门——8 项拒绝语义用例全在 T516 名单。
两帧门（两帧+整数尺寸+双坐标域）、同 floor 移动使 permit 失效且 reserve 零副作用、
同 floor scale 变化使 open hold 失效保首因（**ceil 不变仍换代**——卡面要求）、旧观察不消费新计划。
draw packet 用分配整数尺寸拒 EGL 不匹配（packet ROI 全 tuple）。

### DEVICE_DOMAIN

真机 paint/ROI 实测 NOT_RUN（本卡不使用真机）；T408 独立接 T516 新签包 `6f49d06d…`
重标定（**新 ROI 宽 1176，旧 1177 不得套用**），结果回 architect。

## 判定

- 语义基线（独立推期望 vs T516 生成代码/44 用例判据）：**PASS**
- 设备 paint：NOT_RUN（T408 独立）
