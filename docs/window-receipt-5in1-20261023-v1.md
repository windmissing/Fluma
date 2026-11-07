# 真机窗口回执（effector）：五件套批量执行 v1

日期：2026-10-23。责任：effector。任务：6adb226ce4b01278323a9fd0【真机窗口·五件套】（滴答清单连接器未装，正文未读到，按标题+关联设计稿执行；口径经用户二选一确认）。
状态：**seg3 出数完毕（含重大性能回归发现）/ 108run 封存 / 掉沙 V1-V4 已采但证据质量不足 / 参数面板待回复 / E2 原型未动**。

## 0. 五件套状态总览

| # | 项 | 状态 | 交付物 |
|---|---|---|---|
| 1 | 108run 收尾 | ✅ 封存交接 | `docs/corr-matrix-formal-108-closure-receipt-P011-v1.md` + 产物 `CLOSURE.json` |
| 2 | seg3 三段帧时 | ✅ 出数 + ⚠️ 发现 s134 构建回归 | §2 本文件 + `docs/fps-seg3-P012-v0.md` 升 v1 |
| 3 | 参数面板 | ⏸ 等用户评论回复（掉沙三参数 or toplight 参数） | — |
| 4 | E2 原型 | ⏸ 未动（窗口被 seg3 回归排查占满） | — |
| 5 | 掉沙 V1-V4 | ⚠️ 已执行，证据质量不足（FPS=9-10 致喷流不可见） | `test/artifacts/p011-falling-sand-v1v4/` |

设备最终状态：装回 s134（含全部工作区代码），在线，应用已 force-stop。

## 1. 108run 收尾（封存口径，用户已确认）

82/108 run 全哨兵通过；G1 12 block 完整 + G2 8 block 完整 + G2/b04 半个（A、D）+ G0 未采。主比较 D vs A 21 对可用；B/C 相关比较限 20 个完整 block；三 strata 因 `STRATUM_FACTOR_NOT_EXCITED`（跨档/同档差异比 1.12）只能当同一 working state 的可交换 block。G2 稳健性、G0 控制两条预注册判定作废。详见封存回执，移交 tester 提数。

## 2. seg3 三段帧时（P-013 立项前置数据）

### 2.1 采集矩阵与结果

两套口径各 6 块（三场景 settle/tilt/heavy × abl off/toplight）：

- **s134 全链（tier3 EAB）**：`test/artifacts/p012-measurements/seg3-s134-*`
- **s134 dyn 消融（tier3 + p012ablate=dyn，只画动态层）**：`seg3-s134dyn2-*`（dyn 档在本构建**未**改善 settle/tilt——见 §2.3）

真实吞吐（hilog 时间戳差，非自报 FPS）：

| 场景 | 臂 | 吞吐 | phys P50 | render P50 | 备注 |
|---|---|---|---|---|---|
| settle | off(EAB 全链) | 9.8 fps | 12 ms | 132 ms | 静置微动全程活跃 |
| tilt | off | 9.7 fps | 13 ms | 7/156 ms 双峰 | |
| heavy | off | 50 fps（活跃段 7.6 fps） | 12 | 6/159 双峰 | 活跃段仅前 11 s |
| 二分 s106（无 E3） | off | **37.2 fps** | 12 | **15 ms** | 同设备同热态 |
| 二分 s132（E3 无修复） | off | 11.4 fps | 12 | 139 ms | |
| 二分 s133（E3 含 9827739 修复） | off | 9.0 fps | 13 | 139 ms | **修复未覆盖本场景** |
| 二分 s133+dyn | dyn | 18.3 fps | — | **13 ms** | s133 上 dyn 消融有效 |

### 2.2 关键发现：s134 构建渲染回归（上报 developer，阻塞 P-013 判读）

1. **E3 雾层 dirty 短路对 settle 场景失效**：`e3Invalidate` 的 q 阈值 0.01 在沙面微动时每帧触发 → E3 双 buffer 每帧重烘 → 热态限频下 126 ms/帧。9827739 修的 LAYER_EPS=0.25 只护层高，不护 q。s132/s133 同样慢，说明 9827739 的四项修复对 settle 类缓变场景不生效。
2. **s134 上 dyn 消融也慢（133 ms）**：s133+dyn 为 13 ms/18 fps，s134+dyn 回到 133 ms/10 fps——回归定位在 **s133..HEAD 区间**（掉沙链三提交 + E2 toplight + seg 埋点 + S4 deepCreep，共 8 个工作区提交，未逐个二分）。
3. **热态背景**：设备 thermal_zone0 全程 60 °C（USB 供电+长时间运行），9827739 记载 60 °C → CPU 限频 418-558 MHz → 时间膨胀 3-5 倍。同温下 s106 为 37 fps，证明构建差异是主因，热态是放大器。
4. **H/L 乒乓**：renderMs 双峰（6 ms / 133 ms）交替、render+present 恒 ≈136 ms——Canvas 提交与 GPU/RS 合成互锁的特征，热态加剧。

### 2.3 90Hz 单拍可达性（实测级，P-013 M2 输入）

用**最乐观**组合（s106 物理基线 + s133 dyn 动态层）：

- 物理 P95 16 ms + 动态层渲染 P95 13 ms = **29 ms ≫ 11.1 ms 单拍预算 → 90Hz 单拍不可达**。
- 与 architect「条件可能（中置信）」相比：**实测否定**。障碍在物理段（12-13 ms 本身已超预算 11.1 ms）+ 动态层渲染叠加。P-013 的 M2 里程碑（90Hz 单拍）需先做物理治理（不在换栈范围），换栈只解决渲染段。
- **本节判读基于 s133/s106 数据**；s134 因回归不具代表性。E3 修复后需重测全套。

## 3. 掉沙 V1-V4（B 路线 §6.3 验收场景）

已采集：`test/artifacts/p011-falling-sand-v1v4/frames-v{1,2,3,4}/`（9/9/12/8 帧）+ `hilog-all.txt`（96 条 WS_METRIC）。

- **V1 静置**：swipe 注入成功（ledger suspH 0.60→0.91），雾/悬浮大增；**喷流未在连拍中捕获**（连拍 0.33 s/帧 vs tracer 生命 0.3-2.5 s，且应用本身 10 fps）。
- **V2 倾斜 30°**：注入成功，轨迹走向无法从现有帧确证。
- **V3 满容**：12 次同点注沙未见明确 CAPACITY_BLOCKED 视觉证据；ledger 对账待逐 receipt 解析（hilog 无 receipt 打印，需 ledger 文件通道）。
- **V4 交界**：画面可见注入后雾带变化；喷流同样未捕获。

**硬诚实判定：本批 V1-V4 不能作为 manager 裁决「视觉落地与真实沉降错时」的输入**——FPS=9-10 的热态下，tracer 喷流的观感本身被系统性扭曲（真实产品帧率下喷流可见性完全不同）。需在 fps 回归修复后重录。

## 4. 阻塞与移交

| 项 | 移交 | 内容 |
|---|---|---|
| E3 q 阈值 dirty 失效 + s134 渲染回归 | developer | §2.2 两条；二分证据 `seg3-s{106,132,133}-bisect-*`、`seg3-s134dyn2-*` 已落盘 |
| 108run 提数 | tester | 封存回执 §2 口径（21 对 DA / 20 完整 block / 三 strata 不独立） |
| 参数面板 | 用户评论 | 掉沙三参数定标 or toplight 参数（等回复后执行） |
| E2 原型真机出数 | 下个窗口 | 依赖 fps 回归修复，否则 toplight 对比数据被回归污染 |
| V1-V4 重录 | 下个窗口 | 同上 |

## 5. 未写回项

滴答清单连接器未安装（`unauthorized`），本回执未写回任务 6adb226ce4b01278323a9fd0。
