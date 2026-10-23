# P-012 Q1/E3 mask 标注 + 含端点规则确认 + collect-manifest 落地回执 v1

- **日期**：2026-10-22；责任：effector。对应任务【P-012·Q1标注】。
- **性质**：候选标注（tester 复核后冻结）；不重算任何已冻结数值；离线工作，无新采集。
- **产物根**：`test/artifacts/p012-measurements/q1-mask-annot-v1/`

## 1. 雾带/清水斑驳 mask 标注（E3 稿 §2.1 effector 侧）

标注方法：4x LANCZOS 放大叠图目测迭代（v1→v6），容器内缘角点与沙线（沙粒可辨上缘）逐次校正；多边形顶点以 1x 原生像素坐标登记。

### 1.1 参考帧（主参考 f0 静态 / f5 动态）

| 帧 | 雾带 mask（1x 多边形，侵蚀前候选） | 清水斑驳 mask |
|---|---|---|
| `video1_frame0.jpg`（260×330） | `[(87,27),(171,85),(167,112),(38,148)]` — 上缘沿容器顶内缘、下缘沿沙粒可辨线（左低右高）、右到壁 | **EMPTY**（f0 雾带下直接接沙顶过渡，无可辨独立清水斑驳区） |
| `video1_frame5.jpg`（260×330） | `[(17,53),(186,62),(186,136),(175,132.5),(87.5,111.3),(60,116.3),(17.5,127.5)]` — 下缘含中央沙丘隆起轮廓（丘顶 y≈111） | **EMPTY**（同上） |

排除项已按 E3 稿执行：容器壁/亚克力反光、手/花瓣遮挡、字幕、绿沙体（沙内层纹不进雾带标注）。

### 1.2 基线图（s99w OFF 臂）

`eab-s99w-pair1-off-w1.jpeg`（1280×2832）：雾带=行 941–1160 淡青带（含界面过渡，排除白框 173–177/1099–1104 列带）；清水斑驳=同行域灰蓝矩形块区（与雾带同行域但独立 mask 语义：块状结构 vs 带状过渡）。

### 1.3 覆盖与遗留

- **f0/f2/f5/f6 四主参考全部标注完毕**（f2/f6 于同日补齐，方法同 f0/f5：4x 叠图目测迭代）——f2 为大幅倾斜视角（左上手+花遮挡面积大，雾带右上窄带下缘沿沙线弧 `[(152,116),(248,52),(240,170),(200,150),(157,132)]`）；f6 为近正面视角（沙线中央隆起，`[(20,58),(203,53),(204,175),(140,163),(87,157),(55,168),(17,175)]`；左手指+右蓝牙图标在容器外/遮挡区排除）。
- 沙线为多边形直线近似（真实沙线为弧线）；tester 冻结时可改逐列 onset 精化——登记为算子自由度，不影响候选边界。
- 支持量：f0 雾带 ≈133×85px≈400+ 分析像素（原生 1:1），满足 ≥400 像素门槛；f5 面积更大。清水斑驳 mask 为 EMPTY——P64 清水口径在参考帧上无支持量，登记 N/A 预期。

## 2. 含端点规则确认（796 vs 795 列勘误）

**事实链**：
- 冻结附表 v1 ROI 元组 `(240, y0, 1035, y1)` 与实现 `tools/measure_render_p012.py` PIL `crop()` 绑定：PIL 语义右/下端**不含** → 实际列域 240..1034 = **795 列**（x=1035 不参与任何历史计算）
- Q1 修订 v2（R3）把「mask rect 含端点」登记为设备 display 侧口径（x=240..1035 含 = 796 列）——**两种约定并存且各自标注**（R3 原文）

**effector 确认结论**：
1. **原冻结意图 = 不含右端**。唯一证据是 v1 附表数值全部由 PIL crop 生成（795 列语义）；附表文本本身未注明端点约定（缺口即 review §2 发现的横向前端点缺口），但从"文本与实现从未分歧"可判定冻结数值的意图域是 795 列。
2. **勘误登记**：v1 附表 `(…,1035,…)` 记法按 PIL 语义读作"右端开区间"——建议 tester 在冻结附表 revision 时显式补注「native v1 ROI 右/下端不含」；display 侧 796 列含端点口径继续由 Q1 修订 v2 R3 独立标注，两者不合并、不互换。
3. **影响评估**：x=1035 单列处于容器内壁反光带边缘（1099–1104 白框左过渡区），历史 OFF/EAB 数值不受"少算一列"以外的任何影响；无需重算已冻结数值（红线遵守）。display 796 列若未来出数，与 native 795 列差异预期 ≤1 列亮度贡献，两口径各自成带不互比。

## 3. collect-manifest.json 落地

- **新批次已带**：p012-c-recheck1-mask-20261022、p012-e3-first-20261022（采集脚本内置生成，rehearsal §4.2 字段表：tier/want/seed/build SHA/timestamp/arms）。
- **历史批次回填**（本次补齐）：p012-c-verify-20261021（有效批）、p012-c-verify-20261022-smoke、p012-c-verify-20261021-invalid-noTier（含 `invalidReason` 字段）——来源为采集脚本参数 + s107 HAP SHA256 硬校验值。
- **验证**：tester 分析器（`analyze_p012_c_verify.py` P-6 provenance 消费）对回填后的有效批重跑：`manifestWarning=NONE`，provenance 块完整读入 tier/seed/arms/build（42/42 帧零错误、哨兵不触发，与归档一致）。
- **登记**：回填 manifest 的 timestamp 为采集时刻的近似值（脚本运行日志推断），精确到分钟级；后续新批次仍以采集脚本实时生成为准，回填件仅用于历史 provenance 补链。

## 4. 红线遵守

- 已冻结数值（G1 JSON、v1 附表、Q1 修订等价回执）零改动、零重算。
- 全部 mask 标注为**候选**（`masks-summary-final.json` 顶部声明），tester 复核冻结前不用于任何 PASS 判定。
- 无真机采集（本任务"真机仅确认"项无需求——标注对象均为存档图，无新数据依赖）。

## 附：产物清单

- `test/artifacts/p012-measurements/q1-mask-annot-v1/`：masks-summary-final.json、f0/f5 polygons JSON（v4/v5/final 迭代留痕）、f0-fogband-annot-final.png、f5-fogband-annot-final.png、4x 放大工作图
- 回填 manifest：三个 C 批次目录内 collect-manifest.json + 有效批 analysis-postmanifest.json（provenance 验证）
- 引用文档：docs/algorithm-P012-E3-grid-removal-v1.md §2.1、docs/receipt-P012-q1-revision-v2.md §R3、docs/measurement-frozen-appendix-P012-v1.md
