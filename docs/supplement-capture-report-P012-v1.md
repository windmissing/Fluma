# P-012 C 补采报告 v1（层厚增减 mask 裁剪 + 几何重算/DPI 替代口径）

- **日期**：2026-10-22；责任：effector。对应任务【P-012·C补采收尾】。
- **被测构建**：补采一 = `entry-default-s107-signed.hap`（9d20488，SHA256 `c1514721…ea2a3`）；补采二 = HEAD `a46fcb5` 系构建（含 epoch 修复 0ba4441 + E3 第一批）。
- **真机**：VYG-AL00（62T0225B10010020）。
- **补采一数据**：`test/artifacts/p012-c-recheck1-mask-20261022/`（44 帧 + `collect-manifest.json` + 分析 JSON ×3）。

## 1. 补采一：层厚增减 mask 裁剪

### 1.1 场景与扰动机制

无 inject() want 通道（确认未暴露），采用 **thick 预设 tilt→shake 耦合**（SandCanvas L1592：`setShakeInput(|tilt|/90)`）作为层厚扰动源：慢倾 → 悬浮扬起（带减相）→ 回正 → 悬浮沉降（带增相）。OFF 臂 `p012ablate=strat`，ON 臂默认，均 `p012tier=3`。

### 1.2 采集事故与数据完整性

- **设备外部接管**：pair1-on increase-2 起屏幕被切至桌面（12:04 快照确证，同 v3pilot 历史事件）；pair2-off 全臂 11 帧污染。污染帧以饱和度检测（>80 饱和像素占比 >0.1）逐一标出，**不进入任何统计**。
- 有效帧 30/44：pair1-off ×11、pair2-on ×11、pair1-on ×8（static 3 + decrease 3 + increase 0/1）。
- P-5 哨兵：pair1/pair2 static-0 两臂差分 746k / 970k px（>3 灰度级）， treatments 差异真实。
- pair2-off 缺失 → pair2 只保留 ON 臂自相分析，不用于配对结论。

### 1.3 验证点判定

| 验证点（任务/设计稿 §1） | 判定 | 证据 |
|---|---|---|
| **条纹在真实 mask 内增减，非整层重缩放** | **成立（配对内成立）** | ① 层纹周期 spacing 中位数 static=12.0px vs post-settle=6.5px：若"整层重缩放"周期应随带厚单调伸缩；实测重排为局部相位重分布而非整体缩放（周期分布重排但峰谷结构逐帧连续演化）。② 带内周期数 settled 帧稳定 10–11（static 10 / inc-1 11，带厚 +25px 时仅 +1 周期——比例远小于厚度增幅 2%，符合"λ 不随层高归一化"红线）。③ 动态帧周期数波动（dec-0=14）源于雾带边界进入检测窗，非条纹自身缩放 |
| **新增长度只揭示稳定纹理更多部分** | **成立（方向性证据）** | static[1195–1295] 与 inc-1[1195–1295] 同屏幕窗剖面最优位移相关 0.729 @ shift+3px——材料净搬运后大部分层纹内容保持连续，仅局部相位重分布 |
| **层纹跟随材料而非钉屏幕** | **成立** | 固定屏幕窗（1250–1500）内条纹峰位 static=112px → dec-2/inc-1=120px（+8px 材料位移方向一致）；dec-2 与 inc-1 剖面相关 1.0（同峰位）——静止后回到同一材料相位，确定性成立 |
| **倾斜/回正附着材料区（C 三查③复核）** | 成立 | 静止后条纹与带界相对位置稳定（dec-2/inc-1 逐位一致）；无逐帧滚动/贴纸病征 |
| **不增加库存/几何（红线）** | 遵守 | 零代码改动；扰动全部来自物理自身 shake 通道 |

### 1.4 登记项

- ①配对覆盖不完整：pair2-off 因设备接管缺失，「增/减相」配对判定实际基于 pair1 单对 + pair2-on 旁证；若需 3 对全量（设计稿 §2.2 第 1 条），待设备独占窗口补采 pair2-off。
- ②判定为「方向性成立」而非正式 PASS：`N_C`/`layerExtrema_C_v1` 口径仍未冻结，本报告不宣布 VALIDATED。
- ③绿带（重沙区）因 B 颗粒纹理主导（oscStd 3.4 vs C 增量 ~0.03），C 裁剪行为在该带不可分辨，登记适用性限制。

## 2. 补采二：几何重算（DPI 替代口径）

### 2.1 口径变更（按用户裁决）

DPI 真机不可构造（settings 命令缺失、无 density 通道），改验 **epoch 修复（0ba4441）后的几何重算触发链**：`initField` box 四值实际变化 → `p012GeometryEpoch++`（hilog `[P012] geometryEpoch -> N`）→ renderer `stratBakedGeometryEpoch !== geometryEpoch` → C 位图按新 pxPerMaterial 重烘焙。

### 2.2 触发路径可行性（代码勘查结论）

`initField` 的 box 尺寸源自 `canvasW/H = ctx.width/height`（Canvas onReady 一次定型，无 onAreaChange 监听），**真机常驻运行期 box 不变**：
- 面板 reset → 同尺寸 → 幂等不递增（developer 已验证）
- 任务原文设想的"testMode 下 want 传两组不同几何参数触发 initField 重建"**不可行**：want 只能改 cols/capacity/sandTopMax（物理列数），box 几何不受 want 控制；cellPx（liquidCellPx）不参与 box 计算

### 2.3 判定

- **Node 通道证据（developer，0ba4441 提交内）**：`p012_c_geometryEpoch_rebake`（boxH 900→1200 → λpx 85.33→128.00 = 256/2）+ `p012_c_geometryEpoch_idempotent`（同几何不重烘）两单测覆盖重烘触发与幂等性，51/51 绿。
- **真机证据（developer，s107 双证据）**：首启 `geometryEpoch -> 1 (box 0x0→263x311@51,215)` + `strat: bitmaps H=ok L=ok M=ok`；面板 reset 无新 epoch 行。
- **effector 真机复验**：未执行——常驻运行期无法在不改代码/不换显示模式的前提下构造第二次 box 变化；唯一剩余路径（分屏/窗口模式变更）在当前 hdc 工具面不可注入。**登记 BLOCKED（真机侧），替代证据以 Node 单测 + developer 首启真机日志为准。**

### 2.4 移交

若 manager 认为真机级 epoch 重烘证据必要，可选路径：①developer 加 want 参数强制 box 覆盖（改产品代码，需评审）；②真机分屏测试（需 uitest 分屏注入方案，超出 effector 工具面）。当前不建议为补采二单独立项。

## 3. 结论

1. 补采一**验证点方向性全部成立**：层纹在真实 mask 内增减、周期不随层高归一化（+25px 厚度仅 +1 周期）、材料相位跟随（非钉屏幕）、静止后确定性回归；受设备接管事故限制，配对证据为 1 完整对 + 1 单臂旁证。
2. 补采二按替代口径以 **Node 单测 + 首启真机日志** 收口；真机级二次重烘登记 BLOCKED（无构造路径），不另行立项。
3. 两项均不做 VALIDATED 判定（锚点未冻结口径维持）。

## 附：产物

- 批次：`test/artifacts/p012-c-recheck1-mask-20261022/`（44 帧、collect-manifest.json、analysis-mask-final.json 等分析 3 份、污染检测标注）
- 脚本：`tools/run_p012_c_recheck1_mask.ps1`
- 上游报告：`docs/effect-report-P012-c-verify-v1.md`（首轮验证）、`docs/pipeline-rehearsal-P012-c-verify-v1.md`（管线 SOP）
